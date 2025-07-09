#include "wif_utils.h"
#include "base58/libbase58.h"
#include "hash/sha256.h"
#include "secp256k1/SECP256k1.h"
#include <vector>
#include <string.h>
#include <omp.h>

extern Secp256K1 *secp;

static bool double_sha256(const unsigned char *data, size_t len, unsigned char *digest) {
    unsigned char tmp[32];
    sha256((unsigned char *)data, len, tmp);
    sha256(tmp, 32, digest);
    return true;
}

bool wif_decode(const std::string &wif, Int &key, bool &compressed) {
    size_t binsz = 0;
    std::vector<unsigned char> bin(wif.size());
    if(!b58tobin(bin.data(), &binsz, wif.c_str(), wif.size()))
        return false;
    if(binsz != 37 && binsz != 38)
        return false;
    unsigned char hash[32];
    if(!double_sha256(bin.data(), binsz - 4, hash))
        return false;
    if(memcmp(hash, bin.data() + binsz - 4, 4) != 0)
        return false;
    if(bin[0] != 0x80 && bin[0] != 0xEF)
        return false;
    if(binsz == 38) {
        if(bin[33] != 0x01)
            return false;
        compressed = true;
    } else {
        compressed = false;
    }
    key.Set32Bytes(bin.data() + 1);
    return true;
}

bool wif_public_key(const std::string &wif, Point &pubkey, bool &compressed) {
    Int priv;
    if(!wif_decode(wif, priv, compressed))
        return false;
    pubkey = secp->ComputePublicKey(&priv);
    return true;
}

void wif_public_keys_batch(const std::vector<std::string> &wifs,
                           std::vector<Point> &pubkeys,
                           bool &compressed) {
    size_t n = wifs.size();
    pubkeys.resize(n);
    std::vector<Int> keys(n);
    std::vector<int> valid(n, 0);
    std::vector<int> compf(n, 0);

    #pragma omp parallel for schedule(dynamic,8)
    for(size_t i = 0; i < n; ++i) {
        bool c = false;
        if(wif_decode(wifs[i], keys[i], c)) {
            valid[i] = 1;
            compf[i] = c ? 1 : 0;
        } else {
            valid[i] = 0;
        }
    }

    bool comp = true;
    for(size_t i=0;i<n;i++) {
        if(valid[i]) { comp = compf[i]; break; }
    }

    std::vector<Int> valid_keys;
    std::vector<size_t> idx;
    for(size_t i = 0; i < n; ++i) {
        if(valid[i]) {
            valid_keys.push_back(keys[i]);
            idx.push_back(i);
        }
    }
    std::vector<Point> tmp(valid_keys.size());
    secp->ComputePublicKeysPippenger(valid_keys, tmp);
    for(size_t j = 0; j < idx.size(); ++j)
        pubkeys[idx[j]] = tmp[j];

    compressed = comp;
}

bool wif_encode(const Int &key, bool compressed, std::string &wif) {
    unsigned char data[34];
    size_t len = 33;
    data[0] = 0x80; /* mainnet */
    Int tmp((Int*)&key);
    tmp.Get32Bytes(data + 1);
    if(compressed) {
        data[33] = 0x01;
        len = 34;
    }
    unsigned char hash[32];
    double_sha256(data, len, hash);
    unsigned char buf[40];
    memcpy(buf, data, len);
    memcpy(buf + len, hash, 4);
    char out[128];
    size_t out_sz = sizeof(out);
    if(!b58enc(out, &out_sz, buf, len + 4))
        return false;
    wif.assign(out);
    return true;
}

bool load_wifs_csv(const char *fname, std::vector<Int> &keys, bool &compressed) {
    FILE *fp = fopen(fname, "r");
    if(!fp) return false;
    char line[256];
    if(fgets(line, sizeof(line), fp)) {
        /* skip header */
    }
    bool comp = true;
    while(fgets(line, sizeof(line), fp)) {
        char *token = strtok(line, ",\n\r");
        if(!token) continue;
        Int k; bool ctmp = false;
        if(wif_decode(token, k, ctmp)) {
            keys.push_back(k);
            comp = ctmp;
        }
    }
    fclose(fp);
    compressed = comp;
    return !keys.empty();
}

bool analyze_wif_sequence(const std::vector<Int> &keys, Int &start_key, Int &step) {
    if(keys.size() < 2) return false;
    Int diff;
    diff.Set((Int*)&keys[1]);
    diff.Sub((Int*)&keys[0]);
    step.Set(&diff);
    for(size_t i=2;i<keys.size();i++) {
        diff.Set((Int*)&keys[i]);
        diff.Sub((Int*)&keys[i-1]);
        if(!diff.IsEqual(&step)) {
            step.GCD(&diff);
        }
    }
    start_key.Set((Int*)&keys.back());
    return true;
}

void predict_wifs_range(const char *csv, const Int &range_start, const Int &range_end) {
    std::vector<Int> keys;
    bool compressed = true;
    if(!load_wifs_csv(csv, keys, compressed)) {
        fprintf(stderr, "[E] Cannot load WIF list %s\n", csv);
        return;
    }
    Int last, step;
    if(!analyze_wif_sequence(keys, last, step)) {
        fprintf(stderr, "[E] Not enough data to analyse WIF sequence\n");
        return;
    }
    Int current(last); current.Add(&step);
    while(current.IsLowerOrEqual((Int*)&range_end)) {
        if(current.IsGreaterOrEqual((Int*)&range_start)) {
            std::string wif;
            if(wif_encode(current, compressed, wif))
                printf("%s\n", wif.c_str());
        }
        current.Add(&step);
    }
}


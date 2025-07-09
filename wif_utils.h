#ifndef WIF_UTILS_H
#define WIF_UTILS_H

#include <string>
#include "secp256k1/Int.h"
#include "secp256k1/Point.h"
#include <vector>

bool wif_decode(const std::string &wif, Int &key, bool &compressed);
bool wif_public_key(const std::string &wif, Point &pubkey, bool &compressed);
bool wif_encode(const Int &key, bool compressed, std::string &wif);
bool load_wifs_csv(const char *fname, std::vector<Int> &keys, bool &compressed);
bool analyze_wif_sequence(const std::vector<Int> &keys, Int &start_key, Int &step);
void predict_wifs_range(const char *csv, const Int &range_start, const Int &range_end);

#endif

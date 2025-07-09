#ifdef __AVX2__
#include "sha256.h"
#include <immintrin.h>

/*
 * Basic AVX2 based SHA-256 implementation processing eight messages in
 * parallel.  This implementation follows the structure of the scalar
 * reference code but uses 256-bit SIMD registers to operate on eight
 * lanes simultaneously.
 */

static inline __m256i _ror(__m256i x, int n) {
    return _mm256_or_si256(_mm256_srli_epi32(x, n),
                           _mm256_slli_epi32(x, 32 - n));
}

static inline __m256i _shr(__m256i x, int n) {
    return _mm256_srli_epi32(x, n);
}

#define SIGMA0(x) (_mm256_xor_si256(_mm256_xor_si256(_ror((x), 2), _ror((x), 13)), _ror((x), 22)))
#define SIGMA1(x) (_mm256_xor_si256(_mm256_xor_si256(_ror((x), 6), _ror((x), 11)), _ror((x), 25)))
#define sigma0(x) (_mm256_xor_si256(_mm256_xor_si256(_ror((x), 7), _ror((x), 18)), _shr((x), 3)))
#define sigma1(x) (_mm256_xor_si256(_mm256_xor_si256(_ror((x), 17), _ror((x), 19)), _shr((x), 10)))

#define Ch(x,y,z)  _mm256_xor_si256(_mm256_and_si256(x, y), _mm256_andnot_si256(x, z))
#define Maj(x,y,z) _mm256_or_si256(_mm256_and_si256(x, y), _mm256_and_si256(z, _mm256_or_si256(x, y)))

static const uint32_t K256[64] = {
    0x428a2f98ul,0x71374491ul,0xb5c0fbcful,0xe9b5dba5ul,
    0x3956c25bul,0x59f111f1ul,0x923f82a4ul,0xab1c5ed5ul,
    0xd807aa98ul,0x12835b01ul,0x243185beul,0x550c7dc3ul,
    0x72be5d74ul,0x80deb1feul,0x9bdc06a7ul,0xc19bf174ul,
    0xe49b69c1ul,0xefbe4786ul,0x0fc19dc6ul,0x240ca1ccul,
    0x2de92c6ful,0x4a7484aaul,0x5cb0a9dcul,0x76f988daul,
    0x983e5152ul,0xa831c66dul,0xb00327c8ul,0xbf597fc7ul,
    0xc6e00bf3ul,0xd5a79147ul,0x06ca6351ul,0x14292967ul,
    0x27b70a85ul,0x2e1b2138ul,0x4d2c6dfcul,0x53380d13ul,
    0x650a7354ul,0x766a0abbul,0x81c2c92eul,0x92722c85ul,
    0xa2bfe8a1ul,0xa81a664bul,0xc24b8b70ul,0xc76c51a3ul,
    0xd192e819ul,0xd6990624ul,0xf40e3585ul,0x106aa070ul,
    0x19a4c116ul,0x1e376c08ul,0x2748774cul,0x34b0bcb5ul,
    0x391c0cb3ul,0x4ed8aa4aul,0x5b9cca4ful,0x682e6ff3ul,
    0x748f82eeul,0x78a5636ful,0x84c87814ul,0x8cc70208ul,
    0x90befffaul,0xa4506cebul,0xbef9a3f7ul,0xc67178f2ul
};

void sha256_avx2_8(uint32_t *i0, uint32_t *i1, uint32_t *i2, uint32_t *i3,
                   uint32_t *i4, uint32_t *i5, uint32_t *i6, uint32_t *i7,
                   uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t *d3,
                   uint8_t *d4, uint8_t *d5, uint8_t *d6, uint8_t *d7) {

    __m256i a = _mm256_set1_epi32(0x6a09e667ul);
    __m256i b = _mm256_set1_epi32(0xbb67ae85ul);
    __m256i c = _mm256_set1_epi32(0x3c6ef372ul);
    __m256i d = _mm256_set1_epi32(0xa54ff53aul);
    __m256i e = _mm256_set1_epi32(0x510e527ful);
    __m256i f = _mm256_set1_epi32(0x9b05688cul);
    __m256i g = _mm256_set1_epi32(0x1f83d9abul);
    __m256i h = _mm256_set1_epi32(0x5be0cd19ul);

    __m256i W[64];

    for (int i = 0; i < 16; ++i) {
        W[i] = _mm256_set_epi32(i7[i], i6[i], i5[i], i4[i], i3[i], i2[i], i1[i], i0[i]);
    }
    for (int i = 16; i < 64; ++i) {
        __m256i s0 = sigma0(W[i-15]);
        __m256i s1 = sigma1(W[i-2]);
        W[i] = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(W[i-16], s0), W[i-7]), s1);
    }

    for (int i = 0; i < 64; ++i) {
        __m256i T1 = _mm256_add_epi32(h, SIGMA1(e));
        T1 = _mm256_add_epi32(T1, Ch(e, f, g));
        T1 = _mm256_add_epi32(T1, _mm256_set1_epi32(K256[i]));
        T1 = _mm256_add_epi32(T1, W[i]);
        __m256i T2 = _mm256_add_epi32(SIGMA0(a), Maj(a, b, c));

        h = g;
        g = f;
        f = e;
        e = _mm256_add_epi32(d, T1);
        d = c;
        c = b;
        b = a;
        a = _mm256_add_epi32(T1, T2);
    }

    a = _mm256_add_epi32(a, _mm256_set1_epi32(0x6a09e667ul));
    b = _mm256_add_epi32(b, _mm256_set1_epi32(0xbb67ae85ul));
    c = _mm256_add_epi32(c, _mm256_set1_epi32(0x3c6ef372ul));
    d = _mm256_add_epi32(d, _mm256_set1_epi32(0xa54ff53aul));
    e = _mm256_add_epi32(e, _mm256_set1_epi32(0x510e527ful));
    f = _mm256_add_epi32(f, _mm256_set1_epi32(0x9b05688cul));
    g = _mm256_add_epi32(g, _mm256_set1_epi32(0x1f83d9abul));
    h = _mm256_add_epi32(h, _mm256_set1_epi32(0x5be0cd19ul));

    alignas(32) uint32_t outA[8], outB[8], outC[8], outD[8];
    alignas(32) uint32_t outE[8], outF[8], outG[8], outH[8];

    _mm256_store_si256((__m256i*)outA, a);
    _mm256_store_si256((__m256i*)outB, b);
    _mm256_store_si256((__m256i*)outC, c);
    _mm256_store_si256((__m256i*)outD, d);
    _mm256_store_si256((__m256i*)outE, e);
    _mm256_store_si256((__m256i*)outF, f);
    _mm256_store_si256((__m256i*)outG, g);
    _mm256_store_si256((__m256i*)outH, h);

    uint32_t* outs[8] = {outA,outB,outC,outD,outE,outF,outG,outH};
    uint8_t* digests[8] = {d0,d1,d2,d3,d4,d5,d6,d7};

    for (int lane = 0; lane < 8; ++lane) {
        uint8_t* dst = digests[lane];
        ((uint32_t*)dst)[0] = __builtin_bswap32(outA[lane]);
        ((uint32_t*)dst)[1] = __builtin_bswap32(outB[lane]);
        ((uint32_t*)dst)[2] = __builtin_bswap32(outC[lane]);
        ((uint32_t*)dst)[3] = __builtin_bswap32(outD[lane]);
        ((uint32_t*)dst)[4] = __builtin_bswap32(outE[lane]);
        ((uint32_t*)dst)[5] = __builtin_bswap32(outF[lane]);
        ((uint32_t*)dst)[6] = __builtin_bswap32(outG[lane]);
        ((uint32_t*)dst)[7] = __builtin_bswap32(outH[lane]);
    }
}
#endif
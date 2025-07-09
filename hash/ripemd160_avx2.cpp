#ifdef __AVX2__
#include "ripemd160.h"

void ripemd160_avx2_8(uint8_t *i0, uint8_t *i1, uint8_t *i2, uint8_t *i3,
                       uint8_t *i4, uint8_t *i5, uint8_t *i6, uint8_t *i7,
                       uint8_t *d0, uint8_t *d1, uint8_t *d2, uint8_t *d3,
                       uint8_t *d4, uint8_t *d5, uint8_t *d6, uint8_t *d7) {
    ripemd160sse_32(i0, i1, i2, i3, d0, d1, d2, d3);
    ripemd160sse_32(i4, i5, i6, i7, d4, d5, d6, d7);
}
#endif
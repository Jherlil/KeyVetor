#include "Random.h"
#include <sys/random.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif
#include <stdint.h>
#include <time.h>

static inline uint64_t rotl64(uint64_t x,int k){
    return (x<<k)|(x>>(64-k));
}
static inline uint64_t splitmix64(uint64_t &x){
    uint64_t z=(x+=0x9e3779b97f4a7c15ULL);
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
    z=(z^(z>>27))*0x94d049bb133111ebULL;
    return z^(z>>31);
}
static uint64_t s[4];
void rseed(unsigned long seed){
    uint64_t st=seed;
    for(int i=0;i<4;i++)
        s[i]=splitmix64(st);
}
static inline uint64_t xoshiro256pp(){
    uint64_t result=rotl64(s[0]+s[3],23)+s[0];
    uint64_t t=s[1]<<17;
    s[2]^=s[0];
    s[3]^=s[1];
    s[1]^=s[2];
    s[0]^=s[3];
    s[2]^=t;
    s[3]=rotl64(s[3],45);
    return result;
}

#ifdef __AVX2__
struct xoshiro256pp8{
    __m256i s0,s1,s2,s3;
    void seed(uint64_t base){
        uint64_t st=base;
        alignas(32) uint64_t buf[32];
        for(int i=0;i<8;i++){
            buf[i]=splitmix64(st);
            buf[i+8]=splitmix64(st);
            buf[i+16]=splitmix64(st);
            buf[i+24]=splitmix64(st);
        }
        s0=_mm256_load_si256((__m256i*)&buf[0]);
        s1=_mm256_load_si256((__m256i*)&buf[8]);
        s2=_mm256_load_si256((__m256i*)&buf[16]);
        s3=_mm256_load_si256((__m256i*)&buf[24]);
    }
    static inline __m256i rotl(__m256i x,int k){
        return _mm256_or_si256(_mm256_slli_epi64(x,k),_mm256_srli_epi64(x,64-k));
    }
    void next8(uint64_t out[8]){
        __m256i r=_mm256_add_epi64(rotl(_mm256_add_epi64(s0,s3),23),s0);
        __m256i t=_mm256_slli_epi64(s1,17);
        s2=_mm256_xor_si256(s2,s0);
        s3=_mm256_xor_si256(s3,s1);
        s1=_mm256_xor_si256(s1,s2);
        s0=_mm256_xor_si256(s0,s3);
        s2=_mm256_xor_si256(s2,t);
        s3=rotl(s3,45);
        _mm256_store_si256((__m256i*)out,r);
    }
};
#endif

unsigned long rndl(){
    unsigned long r;
    if(getrandom(&r,sizeof(r),GRND_NONBLOCK)==sizeof(r))
        return r;
    return (unsigned long)xoshiro256pp();
}

double rnd(){
    const double norm=1.0/(double)UINT64_MAX;
    return (double)xoshiro256pp()*norm;
}

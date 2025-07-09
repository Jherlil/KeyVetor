/* RMD160.H - header file for RMD160.C
 */
#ifndef _RMD160_BSGS_H_
#define _RMD160_BSGS_H_

#include <stdint.h>
#include <sys/types.h>
#include <stdalign.h>

#define RMD160_BLOCKBYTES 64
#define RMD160_BLOCKWORDS 16

#define RMD160_HASHBYTES 20
#define RMD160_HASHWORDS 5

/* For compatibility */
#define RIPEMD160_BLOCKBYTES 64
#define RIPEMD160_BLOCKWORDS 16

#define RIPEMD160_HASHBYTES 20
#define RIPEMD160_HASHWORDS 5

/* RIPEMD160 context. */
typedef struct RMD160Context {
  alignas(32) uint32_t key[RIPEMD160_BLOCKWORDS];
  alignas(32) uint32_t iv[RIPEMD160_HASHWORDS];
  uint32_t bytesHi, bytesLo;
} RMD160_CTX;

#define RIPEMD160Context RMD160Context

#ifdef _WIN64
#else
#include <sys/cdefs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void   RMD160Init(RMD160_CTX *);
void   RMD160Update(RMD160_CTX *, const unsigned char *, unsigned int);
void   RMD160Final(unsigned char [RMD160_HASHBYTES], RMD160_CTX *);
char * RMD160End(RMD160_CTX *, char *);
char * RMD160File(const char *, char *);
void RMD160Data(const unsigned char *, unsigned int, char *);

#ifdef __cplusplus
}
#endif

#endif /* _RMD160_BSGS_H_ */
#ifndef OCL_ENGINE_H
#define OCL_ENGINE_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int ocl_init(int requested_shaders);
void ocl_cleanup();
size_t ocl_max_shaders();
int ocl_sha256_33(const uint8_t *input, uint8_t *digest);
int ocl_sha256_batch_33(const uint8_t *inputs, size_t n, uint8_t *digests);
int ocl_ripemd160_batch_32(const uint8_t *inputs, size_t n, uint8_t *digests);

#ifdef __cplusplus
}
#endif

#endif

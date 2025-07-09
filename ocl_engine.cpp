#include "ocl_engine.h"
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static cl_context ctx = NULL;
static cl_command_queue queue = NULL;
static cl_program program = NULL;
static cl_kernel sha_kernel = NULL;
static cl_kernel rmd_kernel = NULL;
static pthread_mutex_t ocl_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t shader_count = 0;

static char *load_source(const char *file, size_t *len) {
    FILE *f = fopen(file, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    size_t l = ftell(f);
    rewind(f);
    char *src = (char*)malloc(l + 1);
    if(fread(src,1,l,f)!=l){fclose(f); free(src); return NULL;}
    fclose(f);
    src[l]='\0';
    if(len) *len = l;
    return src;
}

int ocl_init(int requested) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_uint num;
    err = clGetPlatformIDs(1, &platform, &num);
    if(err != CL_SUCCESS || num==0) {
        fprintf(stderr, "[E] No OpenCL platform found\n");
        return 0;
    }
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &num);
    if(err != CL_SUCCESS) {
        fprintf(stderr, "[E] No OpenCL GPU device found\n");
        return 0;
    }
    cl_uint cu;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, NULL);
    shader_count = (size_t)cu * 64;
    if(requested>0 && (size_t)requested < shader_count) shader_count=requested;
    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if(!ctx || err!=CL_SUCCESS) {
        fprintf(stderr, "[E] Failed to create OpenCL context\n");
        return 0;
    }
    queue = clCreateCommandQueue(ctx, device, 0, &err);
    if(!queue || err!=CL_SUCCESS) {
        fprintf(stderr, "[E] Failed to create command queue\n");
        ocl_cleanup();
        return 0;
    }
    size_t src_len;
    char *src = load_source("ocl_kernel.cl", &src_len);
    if(!src) {
        fprintf(stderr, "[E] Cannot load ocl_kernel.cl\n");
        ocl_cleanup();
        return 0;
    }
    program = clCreateProgramWithSource(ctx, 1, (const char**)&src, &src_len, &err);
    free(src);
    if(err!=CL_SUCCESS) {
        fprintf(stderr, "[E] Failed to create program\n");
        ocl_cleanup();
        return 0;
    }
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err!=CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size+1);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        log[log_size]=0;
        fprintf(stderr, "[E] Build error:\n%s\n", log);
        free(log);
        ocl_cleanup();
        return 0;
    }
    sha_kernel = clCreateKernel(program, "sha256_33_kernel", &err);
    if(err!=CL_SUCCESS) {
        fprintf(stderr, "[E] Failed to create sha256 kernel\n");
        ocl_cleanup();
        return 0;
    }
    rmd_kernel = clCreateKernel(program, "ripemd160_32_kernel", &err);
    if(err!=CL_SUCCESS) {
        fprintf(stderr, "[E] Failed to create ripemd160 kernel\n");
        ocl_cleanup();
        return 0;
    }
    return 1;
}

void ocl_cleanup() {
    if(sha_kernel) clReleaseKernel(sha_kernel); sha_kernel = NULL;
    if(rmd_kernel) clReleaseKernel(rmd_kernel); rmd_kernel = NULL;
    if(program) clReleaseProgram(program); program = NULL;
    if(queue) clReleaseCommandQueue(queue); queue = NULL;
    if(ctx) clReleaseContext(ctx); ctx = NULL;
    shader_count = 0;
}

size_t ocl_max_shaders() { return shader_count; }

int ocl_sha256_33(const uint8_t *input, uint8_t *digest) {
    if(!sha_kernel) return 0;
    cl_int err;
    uint8_t block[64];
    static const uint8_t pad[23] = {0x80};
    static const uint8_t sizedesc[8] = {0,0,0,0,0,0,1,8};
    memcpy(block, input, 33);
    memcpy(block+33, pad, 23);
    memcpy(block+56, sizedesc, 8);
    cl_mem inbuf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 64, block, &err);
    if(err!=CL_SUCCESS) return 0;
    cl_mem outbuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 32, NULL, &err);
    if(err!=CL_SUCCESS) { clReleaseMemObject(inbuf); return 0; }
    err = clSetKernelArg(sha_kernel, 0, sizeof(cl_mem), &inbuf);
    err |= clSetKernelArg(sha_kernel, 1, sizeof(cl_mem), &outbuf);
    if(err!=CL_SUCCESS) { clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); return 0; }
    size_t gsize = 1;
    err = clEnqueueNDRangeKernel(queue, sha_kernel, 1, NULL, &gsize, NULL, 0, NULL, NULL);
    if(err!=CL_SUCCESS) { clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); return 0; }
    clEnqueueReadBuffer(queue, outbuf, CL_TRUE, 0, 32, digest, 0, NULL, NULL);
    clReleaseMemObject(inbuf);
    clReleaseMemObject(outbuf);
    return 1;
}

int ocl_sha256_batch_33(const uint8_t *inputs, size_t n, uint8_t *digests) {
    if(!sha_kernel) return 0;
    cl_int err;
    const uint8_t pad[23] = {0x80};
    const uint8_t sizedesc[8] = {0,0,0,0,0,0,1,8};
    uint8_t *blocks = (uint8_t*)malloc(64*n);
    if(!blocks) return 0;
    for(size_t i=0;i<n;++i){
        memcpy(blocks + i*64, inputs + i*33, 33);
        memcpy(blocks + i*64 + 33, pad, 23);
        memcpy(blocks + i*64 + 56, sizedesc, 8);
    }
    pthread_mutex_lock(&ocl_mutex);
    cl_mem inbuf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 64*n, blocks, &err);
    if(err!=CL_SUCCESS){ pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    cl_mem outbuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 32*n, NULL, &err);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    err = clSetKernelArg(sha_kernel, 0, sizeof(cl_mem), &inbuf);
    err |= clSetKernelArg(sha_kernel, 1, sizeof(cl_mem), &outbuf);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    size_t gsize = n;
    err = clEnqueueNDRangeKernel(queue, sha_kernel, 1, NULL, &gsize, NULL, 0, NULL, NULL);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    clEnqueueReadBuffer(queue, outbuf, CL_TRUE, 0, 32*n, digests, 0, NULL, NULL);
    clReleaseMemObject(inbuf); clReleaseMemObject(outbuf);
    pthread_mutex_unlock(&ocl_mutex);
    free(blocks);
    return 1;
}

int ocl_ripemd160_batch_32(const uint8_t *inputs, size_t n, uint8_t *digests) {
    if(!rmd_kernel) return 0;
    cl_int err;
    const uint8_t pad[24] = {0x80};
    const uint8_t sizedesc[8] = {0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
    uint8_t *blocks = (uint8_t*)malloc(64*n);
    if(!blocks) return 0;
    for(size_t i=0;i<n;++i){
        memcpy(blocks + i*64, inputs + i*32, 32);
        memcpy(blocks + i*64 + 32, pad, 24);
        memcpy(blocks + i*64 + 56, sizedesc, 8);
    }
    pthread_mutex_lock(&ocl_mutex);
    cl_mem inbuf = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,64*n, blocks, &err);
    if(err!=CL_SUCCESS){ pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    cl_mem outbuf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 20*n, NULL, &err);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    err = clSetKernelArg(rmd_kernel, 0, sizeof(cl_mem), &inbuf);
    err |= clSetKernelArg(rmd_kernel, 1, sizeof(cl_mem), &outbuf);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    size_t gsize = n;
    err = clEnqueueNDRangeKernel(queue, rmd_kernel, 1, NULL, &gsize, NULL, 0, NULL, NULL);
    if(err!=CL_SUCCESS){ clReleaseMemObject(inbuf); clReleaseMemObject(outbuf); pthread_mutex_unlock(&ocl_mutex); free(blocks); return 0; }
    clEnqueueReadBuffer(queue, outbuf, CL_TRUE, 0, 20*n, digests, 0, NULL, NULL);
    clReleaseMemObject(inbuf); clReleaseMemObject(outbuf);
    pthread_mutex_unlock(&ocl_mutex);
    free(blocks);
    return 1;
}

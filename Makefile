CXX ?= g++
CC  ?= gcc

CXXFLAGS += -std=c++11 -O3 -march=native -mavx2 -maes -msha -mpclmul \
            -funroll-loops -fomit-frame-pointer -pipe -fopenmp \
            -fcf-protection=none
CFLAGS   += -O3 -march=native -mavx2 -maes -msha -mpclmul  \
            -funroll-loops -fomit-frame-pointer -pipe -fopenmp \
            -fcf-protection=none
LDFLAGS  += -lm -lpthread -lOpenCL
ifeq ($(OS),Windows_NT)
LDFLAGS  += -lws2_32
endif

OBJS = \
    oldbloom/bloom.o \
    bloom/bloom.o \
    base58/base58.o \
    rmd160/rmd160_bsgs.o \
    sha3/sha3.o sha3/keccak.o \
    xxhash/xxhash.o \
    util.o \
    secp256k1/Int.o secp256k1/Point.o secp256k1/SECP256K1.o \
    secp256k1/IntMod.o secp256k1/Random.o secp256k1/IntGroup.o \
    hash/ripemd160.o hash/sha256.o hash/ripemd160_sse.o hash/sha256_sse.o \
    distributed.o skiprange.o ocl_engine.o

all: keyhunt

keyhunt: $(OBJS) keyhunt.cpp
	$(CXX) $(CXXFLAGS) -o $@ keyhunt.cpp $(OBJS) $(LDFLAGS)

%.o: %.cpp
		$(CXX) $(CXXFLAGS) -c $< -o $@

util.o: util.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) keyhunt
.PHONY: all clean

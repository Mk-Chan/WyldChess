#ifndef RANDOM_H
#define RANDOM_H

#include "defs.h"

extern void init_genrand64(u64 seed);
extern u64 genrand64_int64(void);
extern void init_zobrist_keys();

extern HashKey psq_keys[2][8][64];
extern HashKey castle_keys[16];
extern HashKey stm_key;

#define rand_u64(void) (genrand64_int64())
#define rand_u128(void) (((unsigned __int128) rand_u64()) << 64 | rand_u64())

#ifdef HASH_128_BIT
#define rng(void) (rand_u128())
#else
#define rng(void) (rand_u64())
#endif

#endif

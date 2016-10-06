#ifndef RANDOM_H
#define RANDOM_H

#include "defs.h"

extern void init_genrand64(u64 seed);
extern u64 genrand64_int64(void);
extern void init_zobrist_keys();

extern HashKey psq_keys[2][8][64];
extern HashKey castle_keys[16];
extern HashKey stm_key;

#define rng(void) (genrand64_int64())

#endif

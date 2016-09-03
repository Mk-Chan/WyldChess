#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"

extern u64 p_atks[2][64];
extern u64 n_atks[64];
extern u64 k_atks[64];
extern u64 b_pseudo_atks[64];
extern u64 r_pseudo_atks[64];
extern u64 q_pseudo_atks[64];
extern u64 dirn_sqs[64][64];
extern u64 intervening_sqs[64][64];

extern u64 rank_mask[8];
extern u64 file_mask[8];

extern void init_atks();
extern void init_intervening_sqs();
extern void print_bb(u64 bb);

#endif

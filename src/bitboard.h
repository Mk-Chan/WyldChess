#ifndef BITBOARD_H
#define BITBOARD_H

/*
 * WyldChess, a free UCI/Xboard compatible chess engine
 * Copyright (C) 2016  Manik Charan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "defs.h"

extern u64 p_atks_bb[2][64];
extern u64 n_atks_bb[64];
extern u64 k_atks_bb[64];
extern u64 b_pseudo_atks_bb[64];
extern u64 r_pseudo_atks_bb[64];
extern u64 q_pseudo_atks_bb[64];
extern u64 dirn_sqs_bb[64][64];
extern u64 intervening_sqs_bb[64][64];

extern u64 passed_pawn_mask[2][64];
extern u64 file_forward_mask[2][64];
extern u64 adjacent_files_mask[8];
extern u64 adjacent_ranks_mask[8];
extern u64 adjacent_sqs_mask[64];
extern u64 color_sq_mask[2];
extern u64 adjacent_forward_mask[2][64];
extern u64 backwards_pawn_restrictors_mask[2][64];
extern u64 outpost_ranks_mask[2];

extern u32 distance[64][64];
extern u32 sq_color[64];
extern u32 rank_lookup[2][8];

static u64 rank_mask[8] = {
	0xffULL,
	0xffULL << 8,
	0xffULL << 16,
	0xffULL << 24,
	0xffULL << 32,
	0xffULL << 40,
	0xffULL << 48,
	0xffULL << 56
};

static u64 file_mask[8] = {
	0x0101010101010101ULL,
	0x0202020202020202ULL,
	0x0404040404040404ULL,
	0x0808080808080808ULL,
	0x1010101010101010ULL,
	0x2020202020202020ULL,
	0x4040404040404040ULL,
	0x8080808080808080ULL
};

extern void init_lookups();
extern void print_bb(u64 bb);

#endif

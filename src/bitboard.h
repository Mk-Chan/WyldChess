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

extern u64 passed_pawn_mask[2][64];
extern u64 file_forward_mask[2][64];
extern u64 adjacent_files_mask[8];
extern u64 adjacent_ranks_mask[8];
extern u64 color_sq_mask[2];
extern u32 sq_color[64];

extern void init_masks();
extern void init_atks();
extern void init_intervening_sqs();
extern void print_bb(u64 bb);

#endif

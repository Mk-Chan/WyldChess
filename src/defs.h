#ifndef DEF_H
#define DEF_H

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#undef INFINITY

#define ENGINE_NAME (("WyldChess"))
#define AUTHOR_NAME (("Manik Charan"))
#define INITIAL_POSITION (("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))

#define MAX_MOVES    (2048)
#define MAX_PLY      (128)
#define BB(x)        (1ULL << (x))
#define INFINITY     (30000)
#define MAX_MATE_VAL (INFINITY - MAX_PLY)
#define S(mg, eg)    (((mg) + (((unsigned int)eg) << 16)))
#define MAX_PHASE    (256)
#define INVALID      (-INFINITY - 1)

#define MOVE_TYPE_SHIFT (12)
#define PROM_TYPE_SHIFT (15)
#define CAP_TYPE_SHIFT  (18)
#define ORDER_SHIFT     (21)

#define MOVE_TYPE_MASK (7 << MOVE_TYPE_SHIFT)
#define PROM_TYPE_MASK (7 << PROM_TYPE_SHIFT)
#define CAP_TYPE_MASK  (7 << CAP_TYPE_SHIFT)

typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef unsigned long long  HashKey;

enum Colors {
	ALL = 0,
	WHITE = 0,
	BLACK
};

enum BitboardConstants {
	PAWN = 2,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	FULL
};

enum PieceWithColors {
	WP = 2, WN, WB, WR, WQ, WK,
	BP = 10, BN, BB, BR, BQ, BK
};

enum Squares {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};

enum Files {
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
};

enum Ranks {
	RANK_1,
	RANK_2,
	RANK_3,
	RANK_4,
	RANK_5,
	RANK_6,
	RANK_7,
	RANK_8
};

enum CastlingRights {
	WKC = 1,
	WQC = 2,
	BKC = 4,
	BQC = 8
};

enum MoveTypes {
	NORMAL,
	CASTLE      = 1 << MOVE_TYPE_SHIFT,
	ENPASSANT   = 2 << MOVE_TYPE_SHIFT,
	PROMOTION   = 3 << MOVE_TYPE_SHIFT,
	DOUBLE_PUSH = 4 << MOVE_TYPE_SHIFT
};

enum PromotionTypes {
	TO_KNIGHT = KNIGHT << PROM_TYPE_SHIFT,
	TO_BISHOP = BISHOP << PROM_TYPE_SHIFT,
	TO_ROOK   = ROOK   << PROM_TYPE_SHIFT,
	TO_QUEEN  = QUEEN  << PROM_TYPE_SHIFT
};

enum NodeTypes {
	PV_NODE,
	CUT_NODE,
	ALL_NODE
};

static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

static int const is_prom_sq[64] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1
};

#define get_sq(r, f) (((r << 3) | f))
#define rank_of(sq)  (sq >> 3)
#define file_of(sq)  (sq & 7)

#define from_sq(m)   (m & 0x3f)
#define to_sq(m)     ((m >> 6) & 0x3f)
#define move_type(m) (m & MOVE_TYPE_MASK)
#define prom_type(m) ((m & PROM_TYPE_MASK) >> PROM_TYPE_SHIFT)
#define cap_type(m)  ((m & CAP_TYPE_MASK) >> CAP_TYPE_SHIFT)
#define order(m)     (((m) >> ORDER_SHIFT))

#define popcnt(bb)  (__builtin_popcountll(bb))
#define bitscan(bb) (__builtin_ctzll(bb))

#define get_move(data)                     (((data) & 0x1fffff))
#define move_normal(from, to)              (from | (to << 6) | NORMAL)
#define move_cap(from, to, cap)            (from | (to << 6) | NORMAL | (cap << CAP_TYPE_SHIFT))
#define move_double_push(from, to)         (from | (to << 6) | DOUBLE_PUSH)
#define move_castle(from, to)              (from | (to << 6) | CASTLE)
#define move_ep(from, to)                  (from | (to << 6) | ENPASSANT)
#define move_prom(from, to, prom)          (from | (to << 6) | PROMOTION | prom)
#define move_prom_cap(from, to, prom, cap) (from | (to << 6) | PROMOTION | prom | (cap << CAP_TYPE_SHIFT))
#define move(from, to, mt, prom, cap)      (from | (to << 6) | mt | prom | (cap << CAP_TYPE_SHIFT))
#define encode_cap(m, pt)                  (m    |= (pt << CAP_TYPE_SHIFT))
#define encode_order(m, order)             (m    |= ((order) << ORDER_SHIFT))

#define mg_val(val) ((int)((short)val))
#define eg_val(val) ((int)((val + 0x8000) >> 16))

#define phased_val(val, phase) ((((mg_val(val) * phase) + (eg_val(val) * (MAX_PHASE - phase))) / MAX_PHASE))

#endif

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

#ifdef STATS_BUILD
#define STATS(x) x
#else
#define STATS(x)
#endif

#define ENGINE_NAME (("WyldChess"))
#define AUTHOR_NAME (("Manik Charan"))
#define INITIAL_POSITION (("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))

#define MAX_MOVES_PER_GAME (2048)
#define MAX_MOVES_PER_POS  (218)
#define MAX_PLY            (127)
#define BB(x)              (1ULL << (x))
#define INFINITY           (30000)
#define MATE               (29000)
#define WINNING_SCORE      (8000)
#define MAX_MATE_VAL       (MATE - MAX_PLY)
#define TB_MATE_VAL        (28000)
#define TB_CURSED_MATE_VAL (1)
#define MAX_PHASE          (256)

#define MOVE_TYPE_SHIFT (12)
#define PROM_TYPE_SHIFT (15)
#define CAP_TYPE_SHIFT  (18)

#define MOVE_TYPE_MASK (7 << MOVE_TYPE_SHIFT)
#define PROM_TYPE_MASK (7 << PROM_TYPE_SHIFT)
#define CAP_TYPE_MASK  (7 << CAP_TYPE_SHIFT)

typedef unsigned int       u32;
typedef unsigned long long u64;

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

enum OpenFiles {
	ON_KING,
	NEAR_KING
};

enum PassedPawnType {
	CANT_PUSH,
	UNSAFE_PUSH,
	SAFE_PUSH
};

enum MoveOrder {
	HASH_MOVE = 3000000,
	GOOD_CAP  = 2900000,
	KILLER    = 2800000,
	COUNTER   = 2700000,
	NGOOD_CAP = 2600000,
};

static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

static int const tb_values[5] = { -TB_MATE_VAL, -TB_CURSED_MATE_VAL, 0, TB_CURSED_MATE_VAL, TB_MATE_VAL };

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

#define from_sq(m)   ((int)(m & 0x3f))
#define to_sq(m)     ((int)(m >> 6) & 0x3f)
#define move_type(m) ((int)(m & MOVE_TYPE_MASK))
#define prom_type(m) ((int)(m & PROM_TYPE_MASK) >> PROM_TYPE_SHIFT)
#define cap_type(m)  ((int)(m & CAP_TYPE_MASK) >> CAP_TYPE_SHIFT)

#define popcnt(bb)  (__builtin_popcountll(bb))
#define bitscan(bb) (__builtin_ctzll(bb))

#define get_move(data)                     (((data) & 0x1fffff))
#define move_normal(from, to)              ((from) | ((to)<< 6) | NORMAL)
#define move_cap(from, to, cap)            ((from) | ((to)<< 6) | NORMAL | (cap << CAP_TYPE_SHIFT))
#define move_double_push(from, to)         ((from) | ((to)<< 6) | DOUBLE_PUSH)
#define move_castle(from, to)              ((from) | ((to)<< 6) | CASTLE)
#define move_ep(from, to)                  ((from) | ((to)<< 6) | ENPASSANT)
#define move_prom(from, to, prom)          ((from) | ((to)<< 6) | PROMOTION | prom)
#define move_prom_cap(from, to, prom, cap) ((from) | ((to)<< 6) | PROMOTION | prom | (cap << CAP_TYPE_SHIFT))
#define move(from, to, mt, prom, cap)      ((from) | ((to)<< 6) | mt | prom | (cap << CAP_TYPE_SHIFT))

#define S(mg, eg) ((int) (mg + (((unsigned int) eg) << 16)))

// Taken from stockfish to clean out my old narrowing solution
static inline int eg_val(int val)
{
	union {
		unsigned short u;
		short s;
	} eg = {
		(unsigned short)((unsigned)(val + 0x8000) >> 16)
	};
	return eg.s;
}

static inline int mg_val(int val)
{
	union {
		unsigned short u;
		short s;
	} mg = {
		(unsigned short)((unsigned)(val))
	};
	return mg.s;
}

static inline int phased_val(int val, int phase)
{
	return ((mg_val(val) * phase) + (eg_val(val) * (MAX_PHASE - phase))) / MAX_PHASE;
}

#endif

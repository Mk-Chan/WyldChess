#ifndef POSITION_H
#define POSITION_H

/*
 * WyldChess, a free Xboard/Winboard compatible chess engine
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
#include "random.h"
#include "bitboard.h"
#include "magicmoves.h"

typedef struct Stats_s {

	u64 iid_cutoffs;
	u64 iid_tries;
	u64 futility_cutoffs;
	u64 futility_tries;
	u64 null_cutoffs;
	u64 null_tries;
	u64 first_beta_cutoffs;
	u64 beta_cutoffs;
	u64 hash_probes;
	u64 hash_hits;
	u64 total_nodes;

} Stats;

typedef u64 Move;

typedef struct Movelist_s {

	Move  moves[218];
	Move* end;

} Movelist;

typedef struct State_s {

	Move    move;
	u64     pinned_bb;
	u64     checkers_bb;
	u64     ep_sq_bb;
	HashKey pos_key;
	u32     castling_rights;
	u32     fifty_moves;
	u32     full_moves;
	int     phase;
	int     piece_psq_eval[2];

} State;

typedef struct Position_s {

	u64    bb[9];
	u32    hist_size;
	u32    stm;
	u32    king_sq[2];
	u32    board[64];
	State* state;
	State  hist[MAX_MOVES + MAX_PLY];

#ifdef STATS
	Stats    stats;
#endif

} Position;

extern int piece_val[7];
extern int psq_val[8][64];
extern int phase[7];
extern void print_board(Position* pos);

extern void performance_test(Position* const pos, u32 max_depth);

extern void init_pos(Position* pos);
extern int set_pos(Position* pos, char* fen);

extern void do_null_move(Position* const pos);
extern void undo_null_move(Position* const pos);
extern void undo_move(Position* const pos);
extern u32  do_move(Position* const pos, Move const m);
extern u32  do_usermove(Position* const pos, Move const m);

extern void gen_quiets(Position* pos, Movelist* list);
extern void gen_captures(Position* pos, Movelist* list);

extern void gen_check_evasions(Position* pos, Movelist* list);

extern int evaluate(Position* const pos);

static inline void move_piece_no_key(Position* pos, u32 from, u32 to, u32 pt, u32 c)
{
	u64 from_to       = BB(from) ^ BB(to);
	pos->bb[FULL]    ^= from_to;
	pos->bb[c]       ^= from_to;
	pos->bb[pt]      ^= from_to;
	pos->board[to]    = pos->board[from];
	pos->board[from]  = 0;
}

static inline void put_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 set         = BB(sq);
	pos->bb[FULL]  |= set;
	pos->bb[c]     |= set;
	pos->bb[pt]    |= set;
	pos->board[sq]  = make_piece(pt, c);
}

static inline void remove_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 clr         = BB(sq);
	pos->bb[FULL]  ^= clr;
	pos->bb[c]     ^= clr;
	pos->bb[pt]    ^= clr;
	pos->board[sq]  = 0;
}

static inline void move_piece(Position* pos, u32 from, u32 to, u32 pt, u32 c)
{
	u64 from_to          = BB(from) ^ BB(to);
	pos->bb[FULL]       ^= from_to;
	pos->bb[c]          ^= from_to;
	pos->bb[pt]         ^= from_to;
	pos->board[to]       = pos->board[from];
	pos->board[from]     = 0;
	pos->state->pos_key ^= psq_keys[c][pt][from] ^ psq_keys[c][pt][to];
	if (c == WHITE)
		pos->state->piece_psq_eval[WHITE] += psq_val[pt][to] - psq_val[pt][from];
	else
		pos->state->piece_psq_eval[BLACK] += psq_val[pt][to ^ 56] - psq_val[pt][from ^ 56];
}

static inline void put_piece(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 set              = BB(sq);
	pos->bb[FULL]       |= set;
	pos->bb[c]          |= set;
	pos->bb[pt]         |= set;
	pos->board[sq]       = make_piece(pt, c);
	pos->state->pos_key ^= psq_keys[c][pt][sq];
	pos->state->phase   += phase[pt];
	if (c == WHITE)
		pos->state->piece_psq_eval[WHITE] += piece_val[pt] + psq_val[pt][sq];
	else
		pos->state->piece_psq_eval[BLACK] += piece_val[pt] + psq_val[pt][sq ^ 56];
}

static inline void remove_piece(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 clr              = BB(sq);
	pos->bb[FULL]       ^= clr;
	pos->bb[c]          ^= clr;
	pos->bb[pt]         ^= clr;
	pos->board[sq]       = 0;
	pos->state->pos_key ^= psq_keys[c][pt][sq];
	pos->state->phase   -= phase[pt];
	if (c == WHITE)
		pos->state->piece_psq_eval[WHITE] -= piece_val[pt] + psq_val[pt][sq];
	else
		pos->state->piece_psq_eval[BLACK] -= piece_val[pt] + psq_val[pt][sq ^ 56];
}

static inline u64 pawn_shift(u64 bb, u32 c)
{
	return (c == WHITE ? bb << 8 : bb >> 8);
}

static inline u64 pawn_double_shift(u64 bb, u32 c)
{
	return (c == WHITE ? bb << 16 : bb >> 16);
}

static inline u64 get_atks(u32 sq, u32 pt, u64 occupancy)
{
	switch (pt) {
	case KNIGHT: return n_atks[sq];
	case BISHOP: return Bmagic(sq, occupancy);
	case ROOK:   return Rmagic(sq, occupancy);
	case QUEEN:  return Qmagic(sq, occupancy);
	case KING:   return k_atks[sq];
	default:     return -1;
	}
}

static inline u64 atkers_to_sq(Position const * const pos, u32 sq, u32 by_color, u64 occupancy)
{
	return (  ( pos->bb[KNIGHT]                   & pos->bb[by_color] & n_atks[sq])
		| ( pos->bb[PAWN]                     & pos->bb[by_color] & p_atks[!by_color][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & pos->bb[by_color] & Rmagic(sq, occupancy))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & pos->bb[by_color] & Bmagic(sq, occupancy))
		| ( pos->bb[KING]                     & pos->bb[by_color] & k_atks[sq]));
}

static inline u64 all_atkers_to_sq(Position const * const pos, u32 sq, u64 occupancy)
{
	return (  ( pos->bb[KNIGHT]                   & n_atks[sq])
		| ( pos->bb[PAWN]                     & pos->bb[WHITE] & p_atks[BLACK][sq])
		| ( pos->bb[PAWN]                     & pos->bb[BLACK] & p_atks[WHITE][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & Rmagic(sq, occupancy))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & Bmagic(sq, occupancy))
		| ( pos->bb[KING]                     & k_atks[sq]));
}

static inline u64 checkers(Position const * const pos, u32 by_color)
{
	const u32 sq = pos->king_sq[!by_color];
	return (  ( pos->bb[KNIGHT]                   & pos->bb[by_color] & n_atks[sq])
		| ( pos->bb[PAWN]                     & pos->bb[by_color] & p_atks[!by_color][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & pos->bb[by_color] & Rmagic(sq, pos->bb[FULL]))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & pos->bb[by_color] & Bmagic(sq, pos->bb[FULL]))
		| ( pos->bb[KING]                     & pos->bb[by_color] & k_atks[sq]));
}

static inline void set_pinned(Position* const pos)
{
	u32 const  to_color  = pos->stm,
	           ksq       = pos->king_sq[to_color];
	u64* const pinned_bb = &pos->state->pinned_bb;
	          *pinned_bb = 0ULL;

	u32 sq;
	u64 bb,
	    pinners_bb = ((pos->bb[ROOK] | pos->bb[QUEEN])
			  & pos->bb[!to_color]
			  & r_pseudo_atks[ksq])
		    | ((pos->bb[BISHOP] | pos->bb[QUEEN])
		       & pos->bb[!to_color]
		       & b_pseudo_atks[ksq]);
	while (pinners_bb) {
		sq          = bitscan(pinners_bb);
		pinners_bb &= pinners_bb - 1;
		bb          = intervening_sqs[sq][ksq] & pos->bb[FULL];
		if(!(bb & (bb - 1)))
			*pinned_bb ^= bb & pos->bb[to_color];
	}
}

static inline void set_checkers(Position* pos)
{
	pos->state->checkers_bb = checkers(pos, !pos->stm);
}

static inline void move_str(Move move, char str[6])
{
	u32 from = from_sq(move),
	    to   = to_sq(move);
	str[0]    = file_of(from) + 'a';
	str[1]    = rank_of(from) + '1';
	str[2]    = file_of(to)   + 'a';
	str[3]    = rank_of(to)   + '1';
	if (move_type(move) == PROMOTION) {
		const u32 prom = prom_type(move);
		switch (prom) {
		case QUEEN:
			str[4] = 'q';
			break;
		case KNIGHT:
			str[4] = 'n';
			break;
		case BISHOP:
			str[4] = 'b';
			break;
		case ROOK:
			str[4] = 'r';
			break;
		}
	}
	else {
		str[4] = '\0';
	}
	str[5] = '\0';
}

static inline int parse_move(Position* pos, char* str)
{
	u32 from  = (str[0] - 'a') + ((str[1] - '1') << 3),
	    to    = (str[2] - 'a') + ((str[3] - '1') << 3);
	char prom = str[4];
	Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, &list);
	} else {
		gen_quiets(pos, &list);
		gen_captures(pos, &list);
	}
	for(Move* move = list.moves; move != list.end; ++move) {
		if (   from_sq(*move) == from
		    && to_sq(*move) == to) {
			if (    piece_type(pos->board[from]) == PAWN
			    && (to > 56 || to < 8)) {
				switch (prom_type(*move)) {
				case QUEEN:
					if (prom == 'q')
						return *move;
					break;
				case KNIGHT:
					if (prom == 'n')
						return *move;
					break;
				case BISHOP:
					if (prom == 'b')
						return *move;
					break;
				case ROOK:
					if (prom == 'r')
						return *move;
					break;
				}
			} else {
				return *move;
			}
		}
	}
	return 0;
}

#endif

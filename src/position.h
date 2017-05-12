#ifndef POSITION_H
#define POSITION_H

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
#include "misc.h"
#include "bitboard.h"
#include "magicmoves.h"

STATS(
	struct Stats
	{
		u64 correct_nt_guess;
		u64 iid_cutoffs;
		u64 iid_tries;
		u64 null_cutoffs;
		u64 null_tries;
		u64 first_beta_cutoffs;
		u64 beta_cutoffs;
		u64 hash_probes;
		u64 hash_hits;
		u64 total_nodes;
	};
)

struct Movelist
{
	u32  moves[MAX_MOVES_PER_POS];
	u32* end;
};

struct State
{
	u64  pinned_bb;
	u64  checkers_bb;
	u64  ep_sq_bb;
	u64  pos_key;
	u32  castling_rights;
	u32  fifty_moves;
	u32  full_moves;
	u32  move;
};

struct Position
{
	u64 bb[9];
	int stm;
	int king_sq[2];
	int board[64];
	int phase;
	int piece_psq_eval[2];
	State* state;
	State* hist;
	STATS(Stats stats;)
};

extern int piece_val[8];
extern int psqt[2][8][64];
extern int phase[8];

extern void init_eval_terms();
extern void print_board(Position* pos);

extern void performance_test(Position* const pos, u32 max_depth);

extern void init_pos(Position* pos, State* state);
extern int set_pos(Position* pos, std::string fen);
extern Position get_position_copy(Position const * const pos);

extern void do_null_move(Position* const pos);
extern void undo_null_move(Position* const pos);
extern void undo_move(Position* const pos);
extern void do_move(Position* const pos, u32 const m);

extern void gen_pseudo_legal_moves(Position* pos, Movelist* list);
extern void gen_captures(Position* pos, Movelist* list);
extern void gen_quiesce_moves(Position* pos, Movelist* list);
extern void gen_legal_moves(Position* pos, Movelist* list);
extern void gen_check_evasions(Position* pos, Movelist* list);

extern int evaluate(Position* const pos);
extern void tune();

inline int king_sq(Position const * const pos, int c)
{
	return pos->king_sq[c];
}

inline void move_piece_no_key(Position* pos, u32 from, u32 to, u32 pt, u32 c)
{
	u64 from_to             = BB(from) ^ BB(to);
	pos->bb[FULL]          ^= from_to;
	pos->bb[c]             ^= from_to;
	pos->bb[pt]            ^= from_to;
	pos->board[to]          = pos->board[from];
	pos->board[from]        = 0;
	pos->piece_psq_eval[c] += psqt[c][pt][to] - psqt[c][pt][from];
}

inline void put_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 set                 = BB(sq);
	pos->bb[FULL]          |= set;
	pos->bb[c]             |= set;
	pos->bb[pt]            |= set;
	pos->board[sq]          = pt;
	pos->phase             += phase[pt];
	pos->piece_psq_eval[c] += piece_val[pt] + psqt[c][pt][sq];
}

inline void remove_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 clr                 = BB(sq);
	pos->bb[FULL]          ^= clr;
	pos->bb[c]             ^= clr;
	pos->bb[pt]            ^= clr;
	pos->board[sq]          = 0;
	pos->phase             -= phase[pt];
	pos->piece_psq_eval[c] -= piece_val[pt] + psqt[c][pt][sq];
}

inline void move_piece(Position* pos, u32 from, u32 to, u32 pt, u32 c)
{
	u64 from_to             = BB(from) ^ BB(to);
	pos->bb[FULL]          ^= from_to;
	pos->bb[c]             ^= from_to;
	pos->bb[pt]            ^= from_to;
	pos->board[to]          = pos->board[from];
	pos->board[from]        = 0;
	pos->state->pos_key    ^= psq_keys[c][pt][from] ^ psq_keys[c][pt][to];
	pos->piece_psq_eval[c] += psqt[c][pt][to] - psqt[c][pt][from];
}

inline void put_piece(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 set                 = BB(sq);
	pos->bb[FULL]          |= set;
	pos->bb[c]             |= set;
	pos->bb[pt]            |= set;
	pos->board[sq]          = pt;
	pos->state->pos_key    ^= psq_keys[c][pt][sq];
	pos->phase             += phase[pt];
	pos->piece_psq_eval[c] += piece_val[pt] + psqt[c][pt][sq];
}

inline void remove_piece(Position* pos, u32 sq, u32 pt, u32 c)
{
	u64 clr                 = BB(sq);
	pos->bb[FULL]          ^= clr;
	pos->bb[c]             ^= clr;
	pos->bb[pt]            ^= clr;
	pos->board[sq]          = 0;
	pos->state->pos_key    ^= psq_keys[c][pt][sq];
	pos->phase             -= phase[pt];
	pos->piece_psq_eval[c] -= piece_val[pt] + psqt[c][pt][sq];
}

inline u64 pawn_push(int from, u32 c)
{
	return (c == WHITE ? from + 8 : from - 8);
}

inline u64 pawn_shift(u64 bb, u32 c)
{
	return (c == WHITE ? bb << 8 : bb >> 8);
}

inline u64 pawn_double_shift(u64 bb, u32 c)
{
	return (c == WHITE ? bb << 16 : bb >> 16);
}

inline u64 get_atks(u32 sq, u32 pt, u64 occupancy)
{
	switch (pt) {
	case KNIGHT: return n_atks_bb[sq];
	case BISHOP: return Bmagic(sq, occupancy);
	case ROOK:   return Rmagic(sq, occupancy);
	case QUEEN:  return Qmagic(sq, occupancy);
	case KING:   return k_atks_bb[sq];
	default:     return -1;
	}
}

inline u64 get_all_atks(u32 sq, u32 pt, u32 c, u64 occupancy)
{
	switch (pt) {
	case PAWN:   return p_atks_bb[c][sq];
	case KNIGHT: return n_atks_bb[sq];
	case BISHOP: return Bmagic(sq, occupancy);
	case ROOK:   return Rmagic(sq, occupancy);
	case QUEEN:  return Qmagic(sq, occupancy);
	case KING:   return k_atks_bb[sq];
	default:     return -1;
	}
}

inline u64 atkers_to_sq(Position const * const pos, u32 sq, u32 by_color, u64 occupancy)
{
	return (  ( pos->bb[KNIGHT]                   & n_atks_bb[sq])
		| ( pos->bb[PAWN]                     & p_atks_bb[!by_color][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & Rmagic(sq, occupancy))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & Bmagic(sq, occupancy))
		| ( pos->bb[KING]                     & k_atks_bb[sq]))
		& pos->bb[by_color];
}

inline u64 all_atkers_to_sq(Position const * const pos, u32 sq, u64 occupancy)
{
	return (  ( pos->bb[KNIGHT]                   & n_atks_bb[sq])
		| ( pos->bb[PAWN]                     & pos->bb[WHITE] & p_atks_bb[BLACK][sq])
		| ( pos->bb[PAWN]                     & pos->bb[BLACK] & p_atks_bb[WHITE][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & Rmagic(sq, occupancy))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & Bmagic(sq, occupancy))
		| ( pos->bb[KING]                     & k_atks_bb[sq]));
}

inline u64 checkers(Position const * const pos, u32 by_color)
{
	const u32 sq = king_sq(pos, !by_color);
	return (  ( pos->bb[KNIGHT]                   & n_atks_bb[sq])
		| ( pos->bb[PAWN]                     & p_atks_bb[!by_color][sq])
		| ((pos->bb[ROOK]   | pos->bb[QUEEN]) & Rmagic(sq, pos->bb[FULL]))
		| ((pos->bb[BISHOP] | pos->bb[QUEEN]) & Bmagic(sq, pos->bb[FULL]))
		| ( pos->bb[KING]                     & k_atks_bb[sq]))
		& pos->bb[by_color];
}

inline u64 get_pinned(Position* const pos, int to_color)
{
	u32 const ksq = king_sq(pos, to_color);
	u32 sq;
	u64 bb;
	u64 pinned_bb  = 0ULL;
	u64 pinners_bb = ( (pos->bb[ROOK] | pos->bb[QUEEN])
			  & pos->bb[!to_color]
			  & r_pseudo_atks_bb[ksq])
		    | ( (pos->bb[BISHOP] | pos->bb[QUEEN])
		       & pos->bb[!to_color]
		       & b_pseudo_atks_bb[ksq]);
	while (pinners_bb) {
		sq          = bitscan(pinners_bb);
		pinners_bb &= pinners_bb - 1;
		bb          = intervening_sqs_bb[sq][ksq] & pos->bb[FULL];
		if(!(bb & (bb - 1)))
			pinned_bb ^= bb & pos->bb[to_color];
	}
	return pinned_bb;
}

inline void set_pinned(Position* const pos)
{
	pos->state->pinned_bb = get_pinned(pos, pos->stm);
}

inline void set_checkers(Position* pos)
{
	pos->state->checkers_bb = checkers(pos, !pos->stm);
}

inline int insufficient_material(Position* const pos)
{
	if (popcnt(pos->bb[FULL]) > 4)
		return 0;
	u64 const * const bb = pos->bb;
	if (bb[PAWN] | bb[QUEEN] | bb[ROOK])
		return 0;
	if (popcnt(bb[KNIGHT]) > 1)
		return 0;
	if (   popcnt((bb[BISHOP] | bb[KNIGHT]) & bb[WHITE]) > 1
	    || popcnt((bb[BISHOP] | bb[KNIGHT]) & bb[BLACK]) > 1)
	       return 0;
	if (   popcnt(bb[BISHOP] & bb[WHITE]) == 1
	    && popcnt(bb[KNIGHT] & bb[BLACK]) == 1)
		return 0;
	if (   popcnt(bb[BISHOP] & bb[BLACK]) == 1
	    && popcnt(bb[KNIGHT] & bb[WHITE]) == 1)
		return 0;
	if (   (bb[BISHOP] & bb[WHITE])
	    && (bb[BISHOP] & bb[BLACK])
	    &&  sq_color[bitscan((bb[BISHOP] & bb[WHITE]))] != sq_color[bitscan((bb[BISHOP] & bb[BLACK]))])
		return 0;
	return 1;
}

// Idea from Stockfish
inline int legal_move(Position* const pos, u32 move)
{
	u32 c    = pos->stm;
	u32 from = from_sq(move);
	u32 ksq  = king_sq(pos, c);
	if (move_type(move) == ENPASSANT) {
		u64 to_bb  = pos->state->ep_sq_bb;
		u64 cap_bb = pawn_shift(to_bb, !c);
		u64 pieces = (pos->bb[FULL] ^ BB(from) ^ cap_bb) | to_bb;

		return     !(Rmagic(ksq, pieces) & ((pos->bb[QUEEN] | pos->bb[ROOK]) & pos->bb[!c]))
			&& !(Bmagic(ksq, pieces) & ((pos->bb[QUEEN] | pos->bb[BISHOP]) & pos->bb[!c]));
	} else if (from == ksq) {
		return      move_type(move) == CASTLE
			|| !atkers_to_sq(pos, to_sq(move), !c, pos->bb[FULL]);
	} else {
		return    !(pos->state->pinned_bb & BB(from))
			|| (BB(to_sq(move)) & dirn_sqs_bb[from][ksq]);
	}
}

// Idea taken from Stockfish
inline int gives_check(Position const * const pos, u32 move)
{
	int from = from_sq(move),
	    pt   = pos->board[from],
	    to   = to_sq(move),
	    c    = pos->stm,
	    ksq  = king_sq(pos, c ^ 1);
	u64 occ_tmp_bb,
	    to_bb        = BB(to),
	    occupancy_bb = (pos->bb[FULL] ^ BB(from)) | to_bb; // Move the piece
	u64 const * bb = pos->bb;

	// Check if after piece moves, opponent is in check by piece
	if (get_all_atks(ksq, pt, c ^ 1, occupancy_bb) & to_bb)
		return 1;

	// Check if after piece moves, opponent is in check by sliders behind piece
	if (   (Bmagic(ksq, occupancy_bb) & (bb[c] & (bb[BISHOP] | bb[QUEEN])))
	    || (Rmagic(ksq, occupancy_bb) & (bb[c] & (bb[ROOK] | bb[QUEEN]))))
		return 1;

	// Handle different move types
	int mt = move_type(move);
	switch (mt) {
	case NORMAL:
		return 0;
	case DOUBLE_PUSH:
		return 0;
	case ENPASSANT:
		// Direct check handled and discovery handled
		// Remove opponent's enpassant-ed pawn and recheck for discovery
		occ_tmp_bb = occupancy_bb ^ pawn_shift(pos->state->ep_sq_bb, c ^ 1);
		if (   (Bmagic(ksq, occ_tmp_bb) & (bb[c] & (bb[BISHOP] | bb[QUEEN])))
		    || (Rmagic(ksq, occ_tmp_bb) & (bb[c] & (bb[ROOK] | bb[QUEEN]))))
			return 1;
		return 0;
	case CASTLE:
		// Relocate the rook on the occupancy bitboard
		// Check for direct check from the castled rook
		occ_tmp_bb = (to == G1 ? BB(H1) ^ BB(F1)
			               : to == C1 ? BB(A1) ^ BB(D1)
						  : to == G8 ? BB(H8) ^ BB(F8)
							     : BB(A8) ^ BB(D8));
		occupancy_bb ^= occ_tmp_bb;
		if (Rmagic(ksq, occupancy_bb) & occ_tmp_bb)
			return 1;
		return 0;
	case PROMOTION:
		return (get_atks(ksq, prom_type(move), occupancy_bb) & to_bb) > 0ULL;
	default:
		printf("Error!\n");
		return 0;
	}
}

inline void move_str(u32 move, char str[6])
{
	u32 from = from_sq(move),
	    to   = to_sq(move);
	str[0]   = file_of(from) + 'a';
	str[1]   = rank_of(from) + '1';
	str[2]   = file_of(to)   + 'a';
	str[3]   = rank_of(to)   + '1';
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

inline u32 parse_move(Position* pos, char* str)
{
	u32 from  = (str[0] - 'a') + ((str[1] - '1') << 3),
	    to    = (str[2] - 'a') + ((str[3] - '1') << 3);
	char prom = str[4];
	int pr_t;
	Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, &list);
	u32* move;
	for(move = list.moves; move != list.end; ++move) {
		if (   from_sq(*move) == from
		    && to_sq(*move) == to) {
			if (   pos->board[from] == PAWN
			    && is_prom_sq[to]) {
				pr_t = prom_type(*move);
				if (prom == 'q' && pr_t == QUEEN)
					return *move;
				if (prom == 'n' && pr_t == KNIGHT)
					return *move;
				if (prom == 'b' && pr_t == BISHOP)
					return *move;
				if (prom == 'r' && pr_t == ROOK)
					return *move;
			} else {
				return *move;
			}
		}
	}
	return 0;
}

inline int is_passed_pawn(Position* const pos, int sq, int c)
{
	return (    (pos->board[sq] == PAWN)
		&& !(passed_pawn_mask[c][sq] & pos->bb[PAWN] & pos->bb[!c])
		&& !(file_forward_mask[c][sq] & pos->bb[PAWN] & pos->bb[c]));
}

#endif

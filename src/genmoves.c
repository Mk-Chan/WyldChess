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
#include "position.h"
#include "magicmoves.h"

static inline void add_move(u32 m, Movelist* list)
{
	*list->end = m;
	++list->end;
}

static void extract_moves(u32 from, u64 atks, Movelist* list)
{
	u32 to;
	while (atks) {
		to    = bitscan(atks);
		atks &= atks - 1;
		add_move(move_normal(from, to), list);
	}
}

static void extract_caps(Position* const pos, u32 from, u64 atks, Movelist* list)
{
	u32 to;
	while (atks) {
		to    = bitscan(atks);
		atks &= atks - 1;
		add_move(move_cap(from, to, piece_type(pos->board[to])), list);
	}
}

static void gen_check_blocks(Position* pos, u64 blocking_poss_bb, Movelist* list)
{
	u64 blockers_poss, pawn_block_poss;
	u32 blocking_sq, blocker;
	u32 const c              = pos->stm;
	u64       pawns_bb       = pos->bb[PAWN] & pos->bb[c];
	u64 const inlcusion_mask = ~(pawns_bb | pos->bb[KING] | pos->state->pinned_bb),
	          full_bb        = pos->bb[FULL],
	          vacancy_mask   = ~full_bb;
	while (blocking_poss_bb) {
		blocking_sq       = bitscan(blocking_poss_bb);
		blocking_poss_bb &= blocking_poss_bb - 1;
		pawn_block_poss   = pawn_shift(BB(blocking_sq), !c);
		if (pawn_block_poss & pawns_bb) {
			blocker = bitscan(pawn_block_poss);
			if (blocking_sq < 8 || blocking_sq > 55) {
				add_move(move_prom(blocker, blocking_sq, TO_QUEEN), list);
				add_move(move_prom(blocker, blocking_sq, TO_KNIGHT), list);
				add_move(move_prom(blocker, blocking_sq, TO_ROOK), list);
				add_move(move_prom(blocker, blocking_sq, TO_BISHOP), list);
			} else {
				add_move(move_normal(blocker, blocking_sq), list);
			}
		} else if (((c == WHITE && rank_of(blocking_sq) == RANK_4)
			 || (c == BLACK && rank_of(blocking_sq) == RANK_5))
			   && (pawn_block_poss & vacancy_mask)
			   && (pawn_block_poss = pawn_shift(pawn_block_poss, !c) & pawns_bb)) {
			add_move(move_double_push(bitscan(pawn_block_poss), blocking_sq), list);
		}
		blockers_poss = atkers_to_sq(pos, blocking_sq, c, full_bb) & inlcusion_mask;
		while (blockers_poss) {
			add_move(move_normal(bitscan(blockers_poss), blocking_sq), list);
			blockers_poss &= blockers_poss - 1;
		}
	}
}

static void gen_checker_caps(Position* pos, u64 checkers_bb, Movelist* list)
{
	u64 atkers_bb;
	u32 checker, atker, checker_pt;
	u32 const c             = pos->stm;
	u64 const pawns_bb      = pos->bb[PAWN] & pos->bb[c],
	          non_king_mask = ~pos->bb[KING],
	          ep_sq_bb      = pos->state->ep_sq_bb,
		  full_bb       = pos->bb[FULL];
	if (    ep_sq_bb
	    && (pawn_shift(ep_sq_bb, !c) & checkers_bb)) {
		u32 const ep_sq = bitscan(ep_sq_bb);
		u64 ep_poss     = pawns_bb & p_atks[!c][ep_sq];
		while (ep_poss) {
			atker    = bitscan(ep_poss);
			ep_poss &= ep_poss - 1;
			add_move(move_ep(atker, ep_sq), list);
		}
	}
	while (checkers_bb) {
		checker      = bitscan(checkers_bb);
		checker_pt   = piece_type(pos->board[checker]);
		checkers_bb &= checkers_bb - 1;
		atkers_bb    = atkers_to_sq(pos, checker, c, full_bb) & non_king_mask;
		while (atkers_bb) {
			atker      = bitscan(atkers_bb);
			atkers_bb &= atkers_bb - 1;
			if (   (BB(atker) & pawns_bb)
			    && (checker < 8 || checker > 55)) {
				add_move(move_prom_cap(atker, checker, TO_QUEEN, checker_pt), list);
				add_move(move_prom_cap(atker, checker, TO_KNIGHT, checker_pt), list);
				add_move(move_prom_cap(atker, checker, TO_ROOK, checker_pt), list);
				add_move(move_prom_cap(atker, checker, TO_BISHOP, checker_pt), list);
			} else {
				add_move(move_cap(atker, checker, checker_pt), list);
			}
		}
	}
}

void gen_check_evasions(Position* pos, Movelist* list)
{
	u32 const c         = pos->stm,
	          ksq       = pos->king_sq[c];
	u64 checkers_bb     = pos->state->checkers_bb,
	    evasions_bb     = k_atks[ksq] & ~pos->bb[c];
	u64 const full_bb   = pos->bb[FULL];
	u64 const sans_king = full_bb ^ (pos->bb[c] & pos->bb[KING]);

	u32 sq;
	while (evasions_bb) {
		sq = bitscan(evasions_bb);
		evasions_bb &= evasions_bb - 1;
		if (!atkers_to_sq(pos, sq, !c, sans_king))
			add_move(move_cap(ksq, sq, piece_type(pos->board[sq])), list);
	}

	if (checkers_bb & (checkers_bb - 1))
		return;

	gen_checker_caps(pos, checkers_bb, list);

	if (checkers_bb & k_atks[ksq])
		return;

	u64 const blocking_poss_bb = intervening_sqs[bitscan(checkers_bb)][ksq];
	if (blocking_poss_bb)
		gen_check_blocks(pos, blocking_poss_bb, list);
}

static void gen_pawn_captures(Position* pos, Movelist* list)
{
	u64 cap_candidates;
	u32 from, to, cap_pt;
	u32 const  c        = pos->stm;
	u64        pawns_bb = pos->bb[PAWN] & pos->bb[c];
	if (pos->state->ep_sq_bb) {
		u32 const ep_sq   = bitscan(pos->state->ep_sq_bb);
		u64       ep_poss = pawns_bb & p_atks[!c][ep_sq];
		while (ep_poss) {
			from     = bitscan(ep_poss);
			ep_poss &= ep_poss - 1;
			add_move(move_ep(from, ep_sq), list);
		}
	}
	while (pawns_bb) {
		from           = bitscan(pawns_bb);
		pawns_bb      &= pawns_bb - 1;
		cap_candidates = p_atks[c][from] & pos->bb[!c];
		while (cap_candidates) {
			to              = bitscan(cap_candidates);
			cap_pt          = piece_type(pos->board[to]);
			cap_candidates &= cap_candidates - 1;
			if (to < 8 || to > 55) {
				add_move(move_prom_cap(from, to, TO_QUEEN, cap_pt), list);
				add_move(move_prom_cap(from, to, TO_KNIGHT, cap_pt), list);
				add_move(move_prom_cap(from, to, TO_ROOK, cap_pt), list);
				add_move(move_prom_cap(from, to, TO_BISHOP, cap_pt), list);
			} else {
				add_move(move_cap(from, to, cap_pt), list);
			}
		}
	}
}

void gen_captures(Position* pos, Movelist* list)
{
	u32 from, pt;
	u32 const c          = pos->stm;
	u64 const full_bb    = pos->bb[FULL],
	          enemy_mask = pos->bb[!c],
	          us_mask    = pos->bb[c];
	gen_pawn_captures(pos, list);
	u64 curr_piece_bb;
	for (pt = KNIGHT; pt != KING; ++pt) {
		curr_piece_bb = pos->bb[pt] & us_mask;
		while (curr_piece_bb) {
			from           = bitscan(curr_piece_bb);
			curr_piece_bb &= curr_piece_bb - 1;
			extract_caps(pos, from, get_atks(from, pt, full_bb) & enemy_mask, list);
		}
	}
	from = pos->king_sq[c];
	extract_caps(pos, from, k_atks[from] & enemy_mask, list);
}

static void gen_pawn_quiets(Position* pos, Movelist* list)
{
	u64 single_push, from;
	u32 const c            = pos->stm;
	u64 const vacancy_mask = ~pos->bb[FULL];
	u64       pawns_bb     = pos->bb[PAWN] & pos->bb[c];
	while (pawns_bb) {
		from        = pawns_bb & -pawns_bb;
		pawns_bb   &= pawns_bb - 1;
		single_push = pawn_shift(from, c);
		if (single_push & vacancy_mask) {
			if (   (single_push & rank_mask[RANK_1])
			    || (single_push & rank_mask[RANK_8])) {
				u32 const fr = bitscan(from),
				          to = bitscan(single_push);
				add_move(move_prom(fr, to, TO_QUEEN), list);
				add_move(move_prom(fr, to, TO_KNIGHT), list);
				add_move(move_prom(fr, to, TO_ROOK), list);
				add_move(move_prom(fr, to, TO_BISHOP), list);
			} else {
				add_move(move_normal(bitscan(from), bitscan(single_push)), list);
				if (c == WHITE && (from & rank_mask[RANK_2])) {
					u64 const double_push = single_push << 8;
					if (double_push & vacancy_mask)
						add_move(move_double_push(bitscan(from), bitscan(double_push)), list);
				} else if (c == BLACK && (from & rank_mask[RANK_7])) {
					u64 const double_push = single_push >> 8;
					if (double_push & vacancy_mask)
						add_move(move_double_push(bitscan(from), bitscan(double_push)), list);
				}
			}
		}
	}
}

static void gen_castling(Position* pos, Movelist* list)
{
	static u32 const castling_poss[2][2] = {
		{ WKC, WQC },
		{ BKC, BQC }
	};
	static u32 const castling_intermediate_sqs[2][2][2] = {
		{ { F1, G1 }, { D1, C1 } },
		{ { F8, G8 }, { D8, C8 } }
	};
	static u32 const castling_king_sqs[2][2][2] = {
		{ { E1, G1 }, { E1, C1 } },
		{ { E8, G8 }, { E8, C8 } }
	};
	static u64 const castle_mask[2][2] = {
		{ (BB(F1) | BB(G1)), (BB(D1) | BB(C1) | BB(B1)) },
		{ (BB(F8) | BB(G8)), (BB(D8) | BB(C8) | BB(B8)) }
	};

	u32 const c       = pos->stm;
	u64 const full_bb = pos->bb[FULL];

	if (    (castling_poss[c][0] & pos->state->castling_rights)
	    && !(castle_mask[c][0] & pos->bb[FULL])
	    && !(atkers_to_sq(pos, castling_intermediate_sqs[c][0][0], !c, full_bb))
	    && !(atkers_to_sq(pos, castling_intermediate_sqs[c][0][1], !c, full_bb)))
		add_move(move_castle(castling_king_sqs[c][0][0], castling_king_sqs[c][0][1]), list);

	if (    (castling_poss[c][1] & pos->state->castling_rights)
	    && !(castle_mask[c][1] & pos->bb[FULL])
	    && !(atkers_to_sq(pos, castling_intermediate_sqs[c][1][0], !c, full_bb))
	    && !(atkers_to_sq(pos, castling_intermediate_sqs[c][1][1], !c, full_bb)))
		add_move(move_castle(castling_king_sqs[c][1][0], castling_king_sqs[c][1][1]), list);
}

void gen_quiets(Position* pos, Movelist* list)
{
	u32 from, pt;
	u32 const c            = pos->stm;
	u64 const full_bb      = pos->bb[FULL],
	          vacancy_mask = ~full_bb,
	          us_mask      = pos->bb[c];
	gen_castling(pos, list);
	gen_pawn_quiets(pos, list);
	u64 curr_piece_bb;
	for (pt = KNIGHT; pt != KING; ++pt) {
		curr_piece_bb = pos->bb[pt] & us_mask;
		while (curr_piece_bb) {
			from           = bitscan(curr_piece_bb);
			curr_piece_bb &= curr_piece_bb - 1;
			extract_moves(from, get_atks(from, pt, full_bb) & vacancy_mask, list);
		}
	}
	from = pos->king_sq[c];
	extract_moves(from, k_atks[from] & vacancy_mask, list);
}

void gen_pseudo_legal_moves(Position* pos, Movelist* list)
{
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	} else {
		u32 from, pt;
		u32 const c            = pos->stm;
		u64 const full_bb      = pos->bb[FULL],
			  enemy_mask   = pos->bb[!c],
			  vacancy_mask = ~full_bb,
			  us_mask      = pos->bb[c];
		gen_pawn_captures(pos, list);
		gen_castling(pos, list);
		gen_pawn_quiets(pos, list);
		u64 curr_piece_bb;
		for (pt = KNIGHT; pt != KING; ++pt) {
			curr_piece_bb = pos->bb[pt] & us_mask;
			while (curr_piece_bb) {
				from           = bitscan(curr_piece_bb);
				curr_piece_bb &= curr_piece_bb - 1;
				extract_caps(pos, from, get_atks(from, pt, full_bb) & enemy_mask, list);
				extract_moves(from, get_atks(from, pt, full_bb) & vacancy_mask, list);
			}
		}
		from = pos->king_sq[c];
		extract_caps(pos, from, k_atks[from] & enemy_mask, list);
		extract_moves(from, k_atks[from] & vacancy_mask, list);
	}
}

void gen_legal_moves(Position* pos, Movelist* list)
{
	gen_pseudo_legal_moves(pos, list);
	int ksq       = pos->king_sq[pos->stm];
	u64 pinned_bb = pos->state->pinned_bb;
	Move* move;
	for (move = list->moves; move < list->end;) {
		if (  ((pinned_bb & BB(from_sq(*move)))
		     || from_sq(*move) == ksq
		     || move_type(*move) == ENPASSANT)
		    && !legal_move(pos, *move)) {
			--list->end;
			*move = *list->end;
		}
		else
			++move;
	}
}

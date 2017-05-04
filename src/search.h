#ifndef SEARCH_H
#define SEARCH_H

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

#include "search_unit.h"
#include "tt.h"

struct SearchStack
{
	int node_type;
	int early_prune;
	u32 ply;
	u32 killers[2];
	int order_arr[MAX_MOVES_PER_POS];
	Movelist list;
};

static int equal_cap_bound = 50;

static int min_attacker(Position const * const pos, int to, u64 const c_atkers_bb, u64* const occupied_bb, u64* const atkers_bb) {
	u64 tmp;
	int pt = PAWN - 1;
	do {
		++pt;
		tmp = c_atkers_bb & pos->bb[pt];
	} while (!tmp && pt < KING);
	if (pt == KING)
		return pt;
	*occupied_bb ^= tmp & -tmp;
	if (pt == PAWN || pt == BISHOP || pt == QUEEN)
		*atkers_bb |= Bmagic(to, *occupied_bb) & (pos->bb[BISHOP] | pos->bb[QUEEN]);
	if (pt == ROOK || pt == QUEEN)
		*atkers_bb |= Rmagic(to, *occupied_bb) & (pos->bb[ROOK] | pos->bb[QUEEN]);
	*atkers_bb &= *occupied_bb;
	return pt;
}

// Idea taken from Stockfish 6
static int see(Position const * const pos, u32 move)
{
	if (move_type(move) == CASTLE)
		return 0;

	int to = to_sq(move);
	int swap_list[32];
	swap_list[0] = mg_val(piece_val[pos->board[to]]);
	int c = pos->stm;
	int from = from_sq(move);
	u64 occupied_bb = pos->bb[FULL] ^ BB(from);
	if (move_type(move) == ENPASSANT) {
		occupied_bb ^= BB((to - (c == WHITE ? 8 : -8)));
		swap_list[0] = mg_val(piece_val[PAWN]) + 1;
	}
	u64 atkers_bb = all_atkers_to_sq(pos, to, occupied_bb) & occupied_bb;
	c = !c;
	u64 c_atkers_bb = atkers_bb & pos->bb[c];
	int cap = pos->board[from];
	int i;
	for (i = 1; c_atkers_bb;) {
		swap_list[i] = -swap_list[i - 1] + mg_val(piece_val[cap]);
		cap = min_attacker(pos, to, c_atkers_bb, &occupied_bb, &atkers_bb);
		if (cap == KING) {
			if (c_atkers_bb == atkers_bb)
				++i;
			break;
		}

		c = !c;
		c_atkers_bb = atkers_bb & pos->bb[c];
		++i;
	}

	while (--i)
		if (-swap_list[i] < swap_list[i - 1])
			swap_list[i - 1] = -swap_list[i];
	return swap_list[0];
}

inline int cap_order(Position const * const pos, u32 const m)
{
	int cap_val   = mg_val(piece_val[pos->board[to_sq(m)]]);
	int capper_pt = pos->board[from_sq(m)];
	if (move_type(m) == PROMOTION)
		cap_val += mg_val(piece_val[prom_type(m)]);
	if (cap_val - mg_val(piece_val[capper_pt]) > equal_cap_bound)
		return GOOD_CAP + cap_val - capper_pt;

	int see_val = see(pos, m);
	int cap_order;
	if (see_val >= equal_cap_bound)
		cap_order = GOOD_CAP;
	else if (see_val > -equal_cap_bound)
		cap_order = EQUAL_CAP;
	else
		cap_order = BAD_CAP;
	return cap_order + see_val;
}

inline int stopped(SearchUnit* const su)
{
	if (su->ctlr->is_stopped)
		return 1;
	if (   su->target_state != THINKING
	    && su->target_state != ANALYZING) {
		su->ctlr->is_stopped = 1;
		return 1;
	}
	if (   su->ctlr->time_dependent
	    && curr_time() >= su->ctlr->search_end_time) {
		su->ctlr->is_stopped = 1;
		return 1;
	}

	return 0;
}

inline int is_repeat(Position* const pos)
{
	u64 curr_pos_key = pos->state->pos_key;
	State* ptr       = pos->state - 2;
	State* end       = ptr - pos->state->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr_pos_key)
			return 1;
	return 0;
}

static int valid_move(Position* const pos, u32* move)
{
	u32 from = from_sq(*move),
	    to   = to_sq(*move),
	    mt   = move_type(*move),
	    prom = prom_type(*move);
	Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_legal_moves(pos, &list);
	u32* m;
	for (m = list.moves; m != list.end; ++m) {
		if (   from_sq(*m) == from
		    && to_sq(*m) == to
		    && move_type(*m) == mt
		    && prom_type(*m) == prom)
			return 1;
	}
	return 0;
}

inline void print_pv_line(Position* const pos, int depth)
{
	char mstr[6];
	PVEntry entry = pvt_probe(&pvt, pos->state->pos_key);
	if (entry.key == pos->state->pos_key) {
		u32 move = get_move(entry.move);
		if (!valid_move(pos, &move))
			return;
		do_move(pos, move);
		move_str(move, mstr);
		fprintf(stdout, " %s", mstr);
		if (depth > 1)
			print_pv_line(pos, depth - 1);
		undo_move(pos);
	}
}

inline u32 get_pv_move(Position* const pos)
{
	PVEntry entry = pvt_probe(&pvt, pos->state->pos_key);
	if (entry.key == pos->state->pos_key) {
		u32 move = get_move(entry.move);
		if (valid_move(pos, &move))
			return move;
	}
	return 0;
}

inline void clear_search(SearchUnit* const su, SearchStack* const ss)
{
	Controller* const ctlr = su->ctlr;
	ctlr->is_stopped       = 0;
	ctlr->nodes_searched   = 0ULL;
#ifdef STATS
	Position* const pos           = su->pos;
	pos->stats.correct_nt_guess   = 0;
	pos->stats.iid_cutoffs        = 0;
	pos->stats.iid_tries          = 0;
	pos->stats.null_cutoffs       = 0;
	pos->stats.null_tries         = 0;
	pos->stats.first_beta_cutoffs = 0;
	pos->stats.beta_cutoffs       = 0;
	pos->stats.hash_probes        = 0;
	pos->stats.hash_hits          = 0;
#endif
	u32 i, j;
	SearchStack* curr;
	for (i = 0; i != MAX_PLY; ++i) {
		curr              = ss + i;
		curr->node_type   = ALL_NODE;
		curr->early_prune = 1;
		curr->ply         = i;
		curr->killers[0]  = 0;
		curr->killers[1]  = 0;
		curr->list.end    = ss->list.moves;
		for (j = 0; j < MAX_MOVES_PER_POS; ++j)
			curr->order_arr[j] = 0;
	}
	(ss - 1)->killers[0] = (ss - 1)->killers[1] = 0;
	(ss - 2)->killers[0] = (ss - 2)->killers[1] = 0;
}

#endif

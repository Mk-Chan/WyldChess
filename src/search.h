#ifndef SEARCH_H
#define SEARCH_H

/*
 * WyldChess, a free UCI/Xboard compatible chess engine
 * Copyright (C) 2016-2017 Manik Charan
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

static int equal_cap_bound = 50;

static int min_attacker(struct Position const * const pos, int to, u64 const c_atkers_bb, u64* const occupied_bb, u64* const atkers_bb) {
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
static int see(struct Position const * const pos, u32 move)
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

static inline int cap_order(struct Position const * const pos, u32 const m)
{
	int cap_val   = mg_val(piece_val[pos->board[to_sq(m)]]);
	int capper_pt = pos->board[from_sq(m)];
	if (move_type(m) == PROMOTION)
		cap_val += mg_val(piece_val[prom_type(m)]);
	int cap_diff = cap_val - mg_val(piece_val[capper_pt]);
	if (cap_diff > equal_cap_bound)
		return GOOD_CAP + cap_val - capper_pt;
	else if (cap_diff > -equal_cap_bound)
		return GOOD_CAP + capper_pt;

	int see_val = see(pos, m);
	int cap_order;
	if (see_val >= equal_cap_bound)
		cap_order = GOOD_CAP;
	else
		cap_order = NGOOD_CAP;
	return cap_order + see_val;
}

static inline int stopped(struct SearchUnit* const su)
{
	struct Controller* ctlr = &controller;
	if (ctlr->is_stopped)
		return 1;
	if (   su->target_state != THINKING
	    && su->target_state != ANALYZING) {
		ctlr->is_stopped = 1;
		return 1;
	}
	if (   ctlr->time_dependent
	    && curr_time() >= ctlr->search_end_time) {
		ctlr->is_stopped = 1;
		return 1;
	}

	return 0;
}

static inline int is_repeat(struct Position* const pos)
{
	u64 curr_pos_key  = pos->state->pos_key;
	struct State* ptr = pos->state - 2;
	struct State* end = ptr - pos->state->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr_pos_key)
			return 1;
	return 0;
}

static inline void print_pv_line(struct SearchStack* const ss)
{
	char mstr[6];
	u32* curr = ss->pv;
	u32* end  = ss->pv + ss->pv_depth;
	for (; curr != end; ++curr) {
		move_str(*curr, mstr);
		fprintf(stdout, " %s", mstr);
	}
}

static inline u32 get_pv_move(struct SearchStack const * const ss)
{
	return *ss->pv;
}

static inline void clear_search(struct SearchUnit* const su, struct SearchStack* const ss)
{
	struct Controller* const ctlr = &controller;
	ctlr->is_stopped     = 0;
	for (int i = 0; i < MAX_THREADS; ++i)
		ctlr->nodes_searched[i] = 0ULL;
	su->counter          = 0;
	STATS(
		struct Position* const pos    = &su->pos;
		pos->stats.correct_nt_guess   = 0;
		pos->stats.iid_cutoffs        = 0;
		pos->stats.iid_tries          = 0;
		pos->stats.null_cutoffs       = 0;
		pos->stats.null_tries         = 0;
		pos->stats.first_beta_cutoffs = 0;
		pos->stats.beta_cutoffs       = 0;
		pos->stats.hash_probes        = 0;
		pos->stats.hash_hits          = 0;
		pos->stats.all_nodes          = 0ULL;
		pos->stats.pv_nodes           = 0ULL;
		pos->stats.cut_nodes          = 0ULL;
		pos->stats.total_nodes        = 0ULL;
	)
	u32 i, j;
	struct SearchStack* curr;
	for (i = 0; i != MAX_PLY; ++i) {
		curr                = ss + i;
		curr->node_type     = ALL_NODE;
		curr->forward_prune = 1;
		curr->ply           = i;
		curr->killers[0]    = 0;
		curr->killers[1]    = 0;
		curr->list.end      = ss->list.moves;
		for (j = 0; j < MAX_MOVES_PER_POS; ++j)
			curr->order_arr[j] = 0;
	}
}

#endif

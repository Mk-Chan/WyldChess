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

#include "engine.h"
#include "tt.h"

typedef struct Search_Stack_s {

	u32      node_type;
	u32      early_prune;
	u32      ply;
	Move     killers[2];
	Movelist list;

} Search_Stack;

static u64 const HASH_MOVE   = 60000ULL;
static u64 const GOOD_CAP    = 50000ULL;
static u64 const KILLER_PLY  = 45000ULL;
static u64 const KILLER_OLD  = 40000ULL;
static u64 const QUEEN_PROM  = 30000ULL;
static u64 const EQUAL_CAP   = 20000ULL;
static u64 const BAD_CAP     = 10000ULL;
static u64 const PASSER_PUSH = 9000ULL;
static u64 const CASTLING    = 8000ULL;
static u64 const REST        = 1000ULL;

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
static int see(Position const * const pos, Move move)
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

static inline void order_cap(Position const * const pos, Move* const m)
{
	if (cap_type(*m)) {
		int cap_val        = mg_val(piece_val[pos->board[to_sq(*m)]]);
		int capper_pt      = pos->board[from_sq(*m)];
		int piece_val_diff = cap_val - mg_val(piece_val[capper_pt]);
		if (piece_val_diff > equal_cap_bound) {
			encode_order(*m, GOOD_CAP + cap_val - capper_pt);
			return;
		}
	}
	int see_val = see(pos, *m);
	if (see_val > equal_cap_bound)
		encode_order(*m, (GOOD_CAP + see_val));
	else if (see_val > -equal_cap_bound)
		encode_order(*m, (EQUAL_CAP + see_val));
	else
		encode_order(*m, (BAD_CAP + see_val));
}

static inline void sort_moves(Move* const start, Move* const end)
{
	Move* tmp;
	Move* m;
	Move to_shift;
	for (m = start + 1; m < end; ++m) {
		to_shift = *m;
		for (tmp = m; tmp > start && order(to_shift) > order(*(tmp - 1)); --tmp)
			*tmp = *(tmp - 1);
		*tmp = to_shift;
	}
}

static int stopped(Engine* const engine)
{
	if (engine->ctlr->is_stopped)
		return 1;
	if (   engine->target_state != THINKING
	    && engine->target_state != ANALYZING) {
		engine->ctlr->is_stopped = 1;
		return 1;
	}
	if (   engine->ctlr->time_dependent
	    && curr_time() >= engine->ctlr->search_end_time) {
		engine->ctlr->is_stopped = 1;
		return 1;
	}

	return 0;
}

static int is_repeat(Position* const pos)
{
	State* curr = pos->state;
	State* ptr  = curr - 2;
	State* end  = ptr - curr->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr->pos_key)
			return 1;
	return 0;
}

static int valid_move(Position* const pos, Move* move)
{
	u32 from = from_sq(*move),
	    to   = to_sq(*move),
	    mt   = move_type(*move),
	    prom = prom_type(*move);
	static Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, &list);
	for (Move* m = list.moves; m != list.end; ++m) {
		if (   from_sq(*m) == from
		    && to_sq(*m) == to
		    && move_type(*m) == mt
		    && prom_type(*m) == prom)
			return 1;
	}
	return 0;
}

static int get_stored_moves(Position* const pos, int depth)
{
	static char mstr[6];
	if (!depth)
		return 0;
	PV_Entry entry = pvt_probe(&pvt, pos->state->pos_key);
	if (entry.key == pos->state->pos_key) {
		Move move = get_move(entry.move);
		if (  !valid_move(pos, &move)
		   || !legal_move(pos, move))
			return 0;
		do_move(pos, move);
		fprintf(stdout, " ");
		move_str(move, mstr);
		fprintf(stdout, "%s", mstr);
		get_stored_moves(pos, depth - 1);
		undo_move(pos);
		return move;
	}
	return 0;
}

static void clear_search(Engine* const engine, Search_Stack* const ss)
{
	Controller* const ctlr = engine->ctlr;
	ctlr->is_stopped       = 0;
	ctlr->nodes_searched   = 0ULL;
#ifdef STATS
	Position* const pos           = engine->pos;
	pos->stats.correct_nt_guess   = 0;
	pos->stats.iid_cutoffs        = 0;
	pos->stats.iid_tries          = 0;
	pos->stats.futility_cutoffs   = 0;
	pos->stats.futility_tries     = 0;
	pos->stats.null_cutoffs       = 0;
	pos->stats.null_tries         = 0;
	pos->stats.first_beta_cutoffs = 0;
	pos->stats.beta_cutoffs       = 0;
	pos->stats.hash_probes        = 0;
	pos->stats.hash_hits          = 0;
#endif
	u32 i;
	Search_Stack* curr;
	for (i = 0; i != MAX_PLY; ++i) {
		curr              = ss + i;
		curr->node_type   = ALL_NODE;
		curr->early_prune = 1;
		curr->ply         = i;
		curr->killers[0]  = 0;
		curr->killers[1]  = 0;
		curr->list.end    = ss->list.moves;
	}
	(ss - 1)->killers[0] = (ss - 1)->killers[1] = 0;
	(ss - 2)->killers[0] = (ss - 2)->killers[1] = 0;
}

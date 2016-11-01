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
#include "timer.h"

typedef struct Search_Stack_s {

	u32      pv_node;
	u32      early_prune;
	u32      ply;
	Move     killers[2];
	Movelist list;

} Search_Stack;

u64 const HASH_MOVE   = 60000ULL;
u64 const GOOD_CAP    = 50000ULL;
u64 const KILLER_PLY  = 45000ULL;
u64 const KILLER_OLD  = 40000ULL;
u64 const QUEEN_PROM  = 30000ULL;
u64 const EQUAL_CAP   = 20000ULL;
u64 const BAD_CAP     = 10000ULL;
u64 const PASSER_PUSH = 9000ULL;
u64 const CASTLING    = 8000ULL;

static int equal_cap_bound = 50;

int min_attacker(Position const * const pos, int to, u64 const c_atkers_bb, u64* const occupied_bb, u64* const atkers_bb) {
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
int see(Position const * const pos, Move move)
{
	if (move_type(move) == CASTLE)
		return 0;

	int to = to_sq(move);
	int swap_list[32];
	swap_list[0] = mg_val(piece_val[piece_type(pos->board[to])]);
	int c = pos->stm;
	int from = from_sq(move);
	u64 occupied_bb = pos->bb[FULL] ^ BB(from);
	if (move_type(move) == ENPASSANT) {
		occupied_bb ^= BB((to - (c == WHITE ? 8 : -8)));
		swap_list[0] = mg_val(piece_val[PAWN]);
	}
	u64 atkers_bb = all_atkers_to_sq(pos, to, occupied_bb) & occupied_bb;
	c = !c;
	u64 c_atkers_bb = atkers_bb & pos->bb[c];
	int cap = piece_type(pos->board[from]);
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
	if (engine->target_state != THINKING) {
		engine->ctlr->is_stopped = 1;
		return 1;
	}
	if (curr_time() >= engine->ctlr->search_end_time) {
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

static int qsearch(Engine* const engine, Search_Stack* const ss, int alpha, int beta)
{
	if ( !(engine->ctlr->nodes_searched & 2047)
	    && stopped(engine))
		return 0;

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if (pos->state->fifty_moves > 99 || is_repeat(pos))
		return 0;

	if (ss->ply >= MAX_PLY)
		return evaluate(pos);

	++engine->ctlr->nodes_searched;

	int eval = evaluate(pos);
	if (eval >= beta)
		return beta;
	if (eval > alpha)
		alpha = eval;

	int val, checked;
	Movelist* list = &ss->list;
	list->end      = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	checked = pos->state->checkers_bb > 0ULL;
	Move* move;
	if (checked)
		gen_check_evasions(pos, list);
	else
		gen_captures(pos, list);

	for (move = list->moves; move < list->end; ++move) {
		if (   cap_type(*move)
		    || move_type(*move) == ENPASSANT) {
			order_cap(pos, move);
		} else if (   move_type(*move) == PROMOTION
			   && prom_type(*move) == QUEEN) {
			encode_order(*move, QUEEN_PROM);
		}
	}
	sort_moves(list->moves, list->end);

	u32 legal_moves = 0;
	for (move = list->moves; move != list->end; ++move) {
		if (  !checked
		    && order(*move) < BAD_CAP)
			break;

		if (!do_move(pos, *move))
			continue;

		++legal_moves;
		val = -qsearch(engine, ss + 1, -beta, -alpha);
		undo_move(pos);
		if (ctlr->is_stopped)
			return 0;
		if (val >= beta) {
#ifdef STATS
			if (legal_moves == 1)
				++pos->stats.first_beta_cutoffs;
			++pos->stats.beta_cutoffs;
#endif
			return beta;
		}
		if (val > alpha)
			alpha = val;
	}

	if (    checked
	    && !legal_moves)
		return -INFINITY + ss->ply;

	return alpha;
}

static int search(Engine* const engine, Search_Stack* ss, int alpha, int beta, int depth)
{
	if (depth <= 0)
		return qsearch(engine, ss, alpha, beta);

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if ( !(engine->ctlr->nodes_searched & 2047)
	    && stopped(engine))
		return 0;

	if (pos->state->fifty_moves > 99 || is_repeat(pos))
		return 0;

	if (ss->ply >= MAX_PLY)
		return evaluate(pos);

	int pv_node = ss->pv_node;

	TT_Entry entry = tt_probe(&tt, pos->state->pos_key);
	Move tt_move   = get_move(entry.data);
#ifdef STATS
	++pos->stats.hash_probes;
#endif
	if (   !pv_node
	    && (entry.key ^ entry.data) == pos->state->pos_key
	    &&  DEPTH(entry.data) >= depth) {
#ifdef STATS
		++pos->stats.hash_hits;
#endif
		int val  = SCORE(entry.data);
		u64 flag = FLAG(entry.data);

		if (flag == FLAG_EXACT)
			return val;
		else if (flag == FLAG_LOWER)
			alpha = max(alpha, val);
		else if (flag == FLAG_UPPER)
			beta = min(beta, val);

		if (alpha >= beta)
			return beta;
	}

	++ctlr->nodes_searched;

	int val;
	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;

	// Futility pruning
	if (    depth <= 6
	    && !pv_node
	    && !checked
	    &&  ss->early_prune) {
#ifdef STATS
		++pos->stats.futility_tries;
#endif
		val = evaluate(pos) - (mg_val(piece_val[PAWN]) * depth);
		if (   val >= beta
		    && abs(val) < MATE_VAL) {
#ifdef STATS
			++pos->stats.futility_cutoffs;
#endif
			return beta;
		}
	}

	// Null move pruning
	if (    depth >= 4
	    && !pv_node
	    &&  ss->early_prune
	    && !checked
	    &&  pos->state->phase > (MAX_PHASE / 4)
	    &&  evaluate(pos) >= beta - mg_val(piece_val[PAWN])) {
#ifdef STATS
		++pos->stats.null_tries;
#endif
		static int null_reduction[16] = { 0, 0, 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9 };
		int reduction = null_reduction[min(depth, 15)];
		do_null_move(pos);
		ss[1].early_prune = 0;
		val = -search(engine, ss + 1, -beta, -beta + 1, depth - reduction);
		ss[1].early_prune = 1;
		undo_null_move(pos);
		if (ctlr->is_stopped)
			return 0;
		if (   val >= beta
		    && abs(val) < MATE_VAL) {
#ifdef STATS
			++pos->stats.null_cutoffs;
#endif
			return val;
		}
	}

	int old_alpha   = alpha,
	    best_move   = 0,
	    best_val    = -INFINITY,
	    legal_moves = 0;

	Movelist* list  = &ss->list;
	list->end       = list->moves;
	set_pinned(pos);
	gen_pseudo_legal_moves(pos, list);

	/*
	 *  Move ordering:
	 *   1. Hash move
	 *   2. Good captures
	 *   3. Queen promotions
	 *   4. Equal captures
	 *   5. Killer moves
	 *   6. Killer moves from 2 plies ago
	 *   7. Minor promotions
	 *   8. Bad captures
	 *   9. Passed pawn push
	 *  10. Castling
	 *  11. Rest
	 */
	Move* move;
	int tt_move_set = 0;
	for (move = list->moves; move != list->end; ++move) {
		if (*move == tt_move) {
			encode_order(*move, HASH_MOVE);
			tt_move_set = 1;
		} else if (   cap_type(*move)
			   || move_type(*move) == ENPASSANT) {
			order_cap(pos, move);
		} else {
			if (*move == ss->killers[0])
				encode_order(*move, KILLER_PLY + 1);

			else if (*move == ss->killers[1])
				encode_order(*move, KILLER_PLY);

			else if (*move == (ss - 2)->killers[0])
				encode_order(*move, KILLER_OLD + 1);

			else if (*move == (ss - 2)->killers[1])
				encode_order(*move, KILLER_OLD);

			else if (   move_type(*move) == PROMOTION
				 && prom_type(*move) == QUEEN)
				encode_order(*move, QUEEN_PROM);

			else if (move_type(*move) == CASTLE)
				encode_order(*move, CASTLING);

			else if (     piece_type(*move) == PAWN
				 && !(passed_pawn_mask[pos->stm][from_sq(*move)] & pos->bb[PAWN] & pos->bb[!pos->stm]))
				encode_order(*move, PASSER_PUSH);
		}
	}

	// Internal iterative deepening
#ifdef STATS
	int iid = 0;
#endif
	if (   !tt_move_set
	    &&  depth > (pv_node ? 3 : 5)) {
#ifdef STATS
		iid = 1;
		++pos->stats.iid_tries;
#endif
		ss->early_prune = 0;
		ss->pv_node     = 1;
		search(engine, ss, alpha, beta, depth - 2);
		ss->pv_node     = pv_node;
		ss->early_prune = 1;

		entry   = tt_probe(&tt, pos->state->pos_key);
		tt_move = get_move(entry.data);
		for (move = list->moves; move != list->end; ++move) {
			if (*move == tt_move) {
				encode_order(*move, HASH_MOVE);
				tt_move_set = 1;
				break;
			}
		}
	}

	sort_moves(list->moves, list->end);

	int depth_left = depth - 1;
	int ext, checking_move;
	for (move = list->moves; move != list->end; ++move) {
		ext = 0;

		if (!do_move(pos, *move))
			continue;
		++legal_moves;
		checking_move = (checkers(pos, !pos->stm) > 0ULL);

		// Check extension
		if (checking_move) {
		    if ((cap_type(*move) ? order(*move) > BAD_CAP : see(pos, *move) >= -equal_cap_bound))
			ext = 1;
		}

		// Principal Variation Search
		if (legal_moves == 1) {
			ss[1].pv_node = 1;
			val = -search(engine, ss + 1, -beta, -alpha, depth_left + ext);
			ss[1].pv_node = 0;
		} else {
			// Late Move Reduction (LMR) -- Not completely confident of this yet
			if (    depth_left > 2
			    &&  legal_moves > 1
			    && !cap_type(*move)
			    &&  move_type(*move) != PROMOTION
			    && *move != ss->killers[0]
			    && *move != ss->killers[1]
			    && !checking_move
			    && !checked) {
				int reduction = pv_node ? 1 : 1 + (legal_moves > 6) + (depth > 8);
				val = -search(engine, ss + 1, -alpha - 1, -alpha, depth_left - reduction);
			}
			else
				val = -search(engine, ss + 1, -alpha - 1, -alpha, depth_left + ext);

			if (val > alpha) {
				ss[1].pv_node = 1;
				val = -search(engine, ss + 1, -beta, -alpha, depth_left + ext);
				ss[1].pv_node = 0;
			}
		}

		undo_move(pos);
		*move = get_move(*move);

		if (ctlr->is_stopped)
			return 0;

		if (val >= beta) {
#ifdef STATS
			if (legal_moves == 1)
				++pos->stats.first_beta_cutoffs;
			++pos->stats.beta_cutoffs;
			if (iid)
				++pos->stats.iid_cutoffs;
#endif
			if (  !cap_type(*move)
			    && move_type(*move) != PROMOTION) {
				ss->killers[1] = ss->killers[0];
				ss->killers[0] = *move;
			}
			tt_store(&tt, val, FLAG_LOWER, depth, *move, pos->state->pos_key);
			return beta;
		}
		if (val > best_val) {
			best_val  = val;
			best_move = *move;
			if (val > alpha)
				alpha = val;
		}
	}

	if (!legal_moves) {
		if (checked)
			return -INFINITY + ss->ply;
		else
			return 0;
	}

	if (old_alpha == alpha)
		tt_store(&tt, alpha, FLAG_UPPER, depth, best_move, pos->state->pos_key);
	else
		tt_store(&tt, alpha, FLAG_EXACT, depth, best_move, pos->state->pos_key);

	return alpha;
}

static int search_root(Engine* const engine, Search_Stack* ss, int alpha, int beta, int depth)
{
	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	++ctlr->nodes_searched;

	Movelist* list = &ss->list;
	sort_moves(list->moves, list->end);

	int val,
	    best_val    = -INFINITY,
	    best_move   = 0,
	    old_alpha   = alpha,
	    legal_moves = 0;

	Move* move;
	for (move = list->moves; move != list->end; ++move) {
		*move = get_move(*move);
		if (!do_move(pos, *move))
			continue;
		++legal_moves;
		val = -search(engine, ss + 1, -beta, -alpha, depth - 1);
		encode_order(*move, val);
		undo_move(pos);
		if (val >= beta) {
#ifdef STATS
			if (legal_moves == 1)
				++pos->stats.first_beta_cutoffs;
			++pos->stats.beta_cutoffs;
#endif
			tt_store(&tt, val, FLAG_LOWER, depth, get_move(*move), pos->state->pos_key);
			return beta;
		}
		if (val > best_val) {
			best_move = *move;
			best_val  = val;
			if (val > alpha)
				alpha = val;
		}
	}

	if (!legal_moves) {
		if (pos->state->checkers_bb)
			return -INFINITY + ss->ply;
		else
			return 0;
	}

	if (old_alpha == alpha)
		tt_store(&tt, alpha, FLAG_UPPER, depth, get_move(best_move), pos->state->pos_key);
	else
		tt_store(&tt, alpha, FLAG_EXACT, depth, get_move(best_move), pos->state->pos_key);

	if (legal_moves == 1)
		ctlr->is_stopped = 1;

	return alpha;
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
	TT_Entry entry = tt_probe(&tt, pos->state->pos_key);
	if ((entry.key ^ entry.data) == pos->state->pos_key) {
		Move move = get_move(entry.data);
		if (  !valid_move(pos, &move)
		   || !do_move(pos, move))
			return 0;
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
		curr->pv_node     = 0;
		curr->early_prune = 1;
		curr->ply         = i;
		curr->killers[0]  = 0;
		curr->killers[1]  = 0;
		curr->list.end    = ss->list.moves;
	}
	ss->pv_node = 1;
}

void setup_root_moves(Position* const pos, Search_Stack* const ss)
{
	Movelist* list = &ss->list;
	list->end      = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, list);

	Move* move;
	for (move = list->moves; move != list->end; ++move) {
		if (cap_type(*move))
			order_cap(pos, move);
		else if (   move_type(*move) == PROMOTION
			 && prom_type(*move) == QUEEN)
			encode_order(*move, QUEEN_PROM);
	}
}

int begin_search(Engine* const engine)
{
	int val;
	int best_move = 0;
	u64 time;
	// To accomodate (ss - 2) during killer move check at 0 and 1 ply when starting with ss + 2
	Search_Stack ss[MAX_PLY + 2];
	clear_search(engine, ss);
	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;
	setup_root_moves(pos, ss + 2);
	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	int alpha, beta;
	int asp_win_tries;
	static int asp_wins[3] = { 50, 200, INFINITY };
	int depth;
	for (depth = 1; depth <= max_depth; ++depth) {
		if (depth < 5) {
			val = search_root(engine, ss + 2, -INFINITY, +INFINITY, depth);
		} else {
			asp_win_tries = 0;
			while (1) {
				alpha = val - asp_wins[asp_win_tries];
				beta  = val + asp_wins[asp_win_tries];
				val   = search_root(engine, ss + 2, alpha, beta, depth);
				if (val > alpha && val < beta)
					break;
				++asp_win_tries;
			}
		}

		if (   depth > 1
		    && ctlr->is_stopped)
			break;

		time = curr_time() - ctlr->search_start_time;
		if (engine->protocol == CECP)
			fprintf(stdout, "%d %d %llu %llu", depth, val, time / 10, ctlr->nodes_searched);
		else if (engine->protocol == UCI)
			fprintf(stdout, "info depth %u score cp %d nodes %llu time %llu pv", depth, val, ctlr->nodes_searched, time);
		best_move = get_stored_moves(pos, depth);
		fprintf(stdout, "\n");
	}
#ifdef STATS
	fprintf(stdout, "iid cutoff rate=%lf\n",
		((double)pos->stats.iid_cutoffs) / pos->stats.iid_tries);
	fprintf(stdout, "futility cutoff rate=%lf\n",
		((double)pos->stats.futility_cutoffs) / pos->stats.futility_tries);
	fprintf(stdout, "null cutoff rate=%lf\n",
		((double)pos->stats.null_cutoffs) / pos->stats.null_tries);
	fprintf(stdout, "hash hit rate=%lf\n",
		((double)pos->stats.hash_hits) / pos->stats.hash_probes);
	fprintf(stdout, "ordering=%lf\n",
		((double)pos->stats.first_beta_cutoffs) / (pos->stats.beta_cutoffs));
#endif

	return best_move;
}

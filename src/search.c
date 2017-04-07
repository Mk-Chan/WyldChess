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

#include "search.h"

static void order_moves(Position* const pos, SearchStack* const ss, u32 tt_move)
{
	Movelist* list = &ss->list;
	u32* order_list = ss->order_arr;
	u32* move;
	u32* order;
	for (move = list->moves, order = order_list; move < list->end; ++move, ++order) {
		if (*move == tt_move) {
			*order = HASH_MOVE;
		} else if (   cap_type(*move)
			   || move_type(*move) == ENPASSANT) {
			*order = cap_order(pos, *move);
		} else {
			if (*move == ss->killers[0])
				*order = KILLER + 3;

			else if (*move == ss->killers[1])
				*order = KILLER + 2;

			else if (*move == (ss - 2)->killers[0])
				*order = KILLER + 1;

			else if (*move == (ss - 2)->killers[1])
				*order = KILLER;

			else if (prom_type(*move) == QUEEN)
				*order = QUEEN_PROM;

			else if (is_passed_pawn(pos, from_sq(*move), pos->stm))
				*order = QUIET + 2;

			else if (move_type(*move) == CASTLE)
				*order = QUIET + 1;

			else
				*order = QUIET;
		}
	}
}

static u32 get_next_move(SearchStack* const ss)
{
	if (ss->list.end == ss->list.moves)
		return 0;
	Movelist* list = &ss->list;
	u32* order = ss->order_arr;
	int len = list->end - list->moves;
	int best_index = 0;
	int i = 1;
	for (; i < len; ++i) {
		if (order[i] > order[best_index])
			best_index = i;
	}
	u32 best_move = list->moves[best_index];
	--list->end;
	list->moves[best_index] = *list->end;
	order[best_index] = order[len - 1];
	return best_move;
}

static int qsearch(Engine* const engine, SearchStack* const ss, int alpha, int beta)
{
	if ( !(engine->ctlr->nodes_searched & 0x7ff)
	    && stopped(engine))
		return 0;

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if (pos->state->fifty_moves > 99)
		return 0;

	if (  !cap_type((pos->state - 1)->move)
	    && is_repeat(pos))
		return 0;

	if (ss->ply >= MAX_PLY)
		return evaluate(pos);

	// Mate distance pruning
	alpha = max((-MAX_MATE_VAL + ss->ply), alpha);
	beta  = min((MAX_MATE_VAL - ss->ply), beta);
	if (alpha >= beta)
		return alpha;

	++engine->ctlr->nodes_searched;
#ifdef STATS
	++engine->pos->stats.correct_nt_guess;
#endif

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;
	int eval;

	if (!checked) {
		eval = evaluate(pos);
		if (eval >= beta)
			return eval;
		if (eval > alpha)
			alpha = eval;
	}

	Movelist* list  = &ss->list;
	list->end       = list->moves;
	set_pinned(pos);
	if (checked) {
		gen_check_evasions(pos, list);
		if (list->end == list->moves)
			return -INFINITY + ss->ply;

	} else {
		gen_captures(pos, list);
	}

	order_moves(pos, ss, 0);

	int val;
#ifdef STATS
	u32 legal_moves = 0;
#endif
	u32 move;
	while ((move = get_next_move(ss))) {
		if (!legal_move(pos, move))
			continue;
#ifdef STATS
		++legal_moves;
#endif

#ifndef PLAIN_AB
		// Futility pruning
		if (   !checked
		    &&  alpha > -MAX_MATE_VAL
		    && !gives_check(pos, move)) {
			val = eval + mg_val(piece_val[cap_type(move)]) + (mg_val(piece_val[PAWN]) / 2);
			if (move_type(move) == PROMOTION)
				val += mg_val(piece_val[prom_type(move)]);
			if (val <= alpha)
				continue;
		}
#endif

		do_move(pos, move);
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

	return alpha;
}

static int search(Engine* const engine, SearchStack* const ss, int alpha, int beta, int depth)
{
	if (depth <= 0)
		return qsearch(engine, ss, alpha, beta);

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if (ss->ply) {
		if ( !(engine->ctlr->nodes_searched & 0x7ff)
		    && stopped(engine))
			return 0;

		if (pos->state->fifty_moves > 99 || is_repeat(pos))
			return 0;

		if (ss->ply >= MAX_PLY)
			return evaluate(pos);

		// Mate distance pruning
		alpha = max((-MAX_MATE_VAL + ss->ply), alpha);
		beta  = min((MAX_MATE_VAL - ss->ply), beta);
		if (alpha >= beta)
			return alpha;
	}

	int node_type = ss->node_type;

#ifdef STATS
	++pos->stats.hash_probes;
#endif
	TTEntry entry = tt_probe(&tt, pos->state->pos_key);
	u32 tt_move   = 0;
#ifndef PLAIN_AB
	if ((entry.key ^ entry.data) == pos->state->pos_key) {
#ifdef STATS
		++pos->stats.hash_hits;
#endif
		tt_move = get_move(entry.data);
		if (    node_type != PV_NODE
		    &&  DEPTH(entry.data) >= depth) {
			int val  = SCORE(entry.data);
			u64 flag = FLAG(entry.data);
			int a    = alpha,
			    b    = beta;

			if (flag == FLAG_EXACT)
				return val;
			else if (flag == FLAG_LOWER)
				a = max(alpha, val);
			else if (flag == FLAG_UPPER)
				b = min(beta, val);

			if (a >= b)
				return val;
		}
	}
#endif

	++ctlr->nodes_searched;

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;
	int val;
	int static_eval;
	if (node_type != PV_NODE)
		static_eval = evaluate(pos);

	// Futility pruning
	if (    depth < 3
	    &&  node_type != PV_NODE
	    &&  ss->early_prune
	    && !checked) {
		val = static_eval - (200 * depth);
		if (   abs(val) < MAX_MATE_VAL
		    && val >= beta)
			return val;
	}

#ifndef PLAIN_AB
	// Null move pruning
	if (    depth >= 4
	    &&  node_type == CUT_NODE
	    &&  ss->early_prune
	    && !checked
	    &&  static_eval >= beta
	    && (pos->bb[pos->stm] & ~(pos->bb[KING] ^ pos->bb[PAWN]))) {
#ifdef STATS
		++pos->stats.null_tries;
#endif
		int reduction = 4 + min(3, max(0, (static_eval - beta) / mg_val(piece_val[PAWN])));
		int depth_left = max(1, depth - reduction);
		ss[1].node_type   = CUT_NODE;
		ss[1].early_prune = 0;
		do_null_move(pos);
		val = -search(engine, ss + 1, -beta, -beta + 1, depth_left);
		undo_null_move(pos);
		if (ctlr->is_stopped)
			return 0;
		if (val >= beta) {
#ifdef STATS
			++pos->stats.null_cutoffs;
			++pos->stats.correct_nt_guess;
#endif
			if (abs(val) >= MAX_MATE_VAL)
				val = beta;

			return val;
		}
	}
#endif

	// Internal iterative deepening
#ifdef STATS
	int iid = 0;
#endif
#ifndef PLAIN_AB
	// Conditions similar to stockfish as of now, seems to be effective
	if (   !tt_move
	    &&  depth >= 5
	    && (node_type == PV_NODE || static_eval + mg_val(piece_val[PAWN]) >= beta)) {
#ifdef STATS
		iid = 1;
		++pos->stats.iid_tries;
#endif
		int reduction   = 2;
		int ep          = ss->early_prune;
		int nt          = ss->node_type;
		ss->early_prune = 0;

		search(engine, ss, alpha, beta, depth - reduction);

		ss->early_prune = ep;
		ss->node_type   = nt;

		entry   = tt_probe(&tt, pos->state->pos_key);
		tt_move = get_move(entry.data);
	}
#endif

	Movelist* list = &ss->list;
	list->end      = list->moves;
	set_pinned(pos);
	gen_pseudo_legal_moves(pos, list);

	order_moves(pos, ss, tt_move);

	int old_alpha   = alpha,
	    best_val    = -INFINITY,
	    best_move   = 0,
	    legal_moves = 0;
	int checking_move, depth_left;
	u32 move;
	while ((move = get_next_move(ss))) {
		if (!legal_move(pos, move))
			continue;
		++legal_moves;

		depth_left    = depth - 1;
		checking_move = gives_check(pos, move);

		// Check extension
		if (    checking_move
		    && (depth == 1 || see(pos, to_sq(move)) > -equal_cap_bound))
			++depth_left;

#ifndef PLAIN_AB
		// Futility pruning
		if (    depth < 8
		    &&  legal_moves > 1
		    &&  best_val > -MAX_MATE_VAL
		    &&  node_type != PV_NODE
		    &&  move_type(move) != PROMOTION
		    && !checking_move
		    && !cap_type(move)
		    &&  static_eval + 100 * depth_left <= alpha)
			continue;

		// Late move reduction
		if (    ss->ply
		    &&  depth > 2
		    &&  legal_moves > (node_type == PV_NODE ? 5 : 3)
		    &&  move != ss->killers[0]
		    &&  move != ss->killers[1]
		    &&  move != (ss-2)->killers[0]
		    &&  move != (ss-2)->killers[1]
		    &&  prom_type(move) != QUEEN
		    && !cap_type(move)
		    && !checking_move
		    && !checked) {
			int reduction = 2
				     + (legal_moves > 10)
				     + (node_type != PV_NODE);
			depth_left = max(1, depth - reduction);
		}
#endif

		do_move(pos, move);

		// Principal Variation Search
		if (   legal_moves == 1
		    || node_type != PV_NODE) {
			if (node_type == PV_NODE)
				ss[1].node_type = PV_NODE;
			else if (node_type == CUT_NODE && legal_moves == 1)
				ss[1].node_type = ALL_NODE;
			else
				ss[1].node_type = CUT_NODE;
			ss[1].early_prune = 1;
			val = -search(engine, ss + 1, -beta, -alpha, depth_left);
			if (   val > alpha
			    && depth_left < depth - 1) {
				ss[1].node_type = node_type;
				ss[1].early_prune = 1;
				val = -search(engine, ss + 1, -beta, -alpha, depth - 1);
			}
		} else {
			ss[1].node_type = CUT_NODE;
			ss[1].early_prune = 1;
			val = -search(engine, ss + 1, -alpha - 1, -alpha, depth_left);
			if (val > alpha) {
				ss[1].node_type = PV_NODE;
				val = -search(engine, ss + 1, -beta, -alpha, max(depth - 1, depth_left));
			}
		}

		undo_move(pos);

		if (ctlr->is_stopped)
			return 0;

		if (val > best_val) {
			best_val  = val;
			best_move = move;

			if (val > alpha) {
				alpha = val;

				if (val >= beta) {
#ifdef STATS
					if (legal_moves == 1)
						++pos->stats.first_beta_cutoffs;
					++pos->stats.beta_cutoffs;
					pos->stats.iid_cutoffs += iid;
					if (node_type == CUT_NODE)
						++pos->stats.correct_nt_guess;
#endif
					if (  !cap_type(move)
					    && ss->killers[0] != move
					    && move_type(move) != ENPASSANT
					    && move_type(move) != PROMOTION) {
						ss->killers[1] = ss->killers[0];
						ss->killers[0] = move;
					}
					tt_store(&tt, best_val, FLAG_LOWER, depth, best_move, pos->state->pos_key);
					return best_val;
				}
			}
		}

		if (node_type == CUT_NODE)
			node_type = ALL_NODE;
	}

#ifdef STATS
	if (old_alpha == alpha)
		pos->stats.correct_nt_guess += (node_type == ALL_NODE);
	else
		pos->stats.correct_nt_guess += (node_type == PV_NODE);
#endif

	if (  !ss->ply
	    && legal_moves == 1)
		ctlr->is_stopped = 1;

	if (!legal_moves) {
		if (checked)
			return -INFINITY + ss->ply;
		else
			return 0;
	}


	if (old_alpha == alpha)
		tt_store(&tt, best_val, FLAG_UPPER, depth, best_move, pos->state->pos_key);
	else {
		tt_store(&tt, alpha, FLAG_EXACT, depth, best_move, pos->state->pos_key);
		pvt_store(&pvt, get_move(best_move), pos->state->pos_key, depth);
	}

	return alpha;
}

int begin_search(Engine* const engine)
{
	u64 time;
	int val, alpha, beta, asp_win_tries, depth;
	int best_move = 0;

	// To accomodate (ss - 2) during killer move check at 0 and 1 ply when starting with ss + 2
	SearchStack ss[MAX_PLY + 2];
	clear_search(engine, ss + 2);
	pvt_clear(&pvt);

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	static int asp_wins[] = { 10, 25, 50, 100, 200, INFINITY };
	for (depth = 1; depth <= max_depth; ++depth) {
		if (depth < 5) {
			ss[2].node_type = PV_NODE;
			val = search(engine, ss + 2, -INFINITY, +INFINITY, depth);
		} else {
			asp_win_tries = 0;
			while (1) {
				ss[2].node_type = PV_NODE;
				alpha = val - asp_wins[asp_win_tries];
				beta  = val + asp_wins[asp_win_tries];
				val   = search(engine, ss + 2, alpha, beta, depth);

				if (val > alpha && val < beta)
					break;
				++asp_win_tries;
			}
		}

		if (   depth > 1
		    && ctlr->is_stopped)
			break;

		time = curr_time() - ctlr->search_start_time;
		if (engine->protocol == XBOARD)
			fprintf(stdout, "%3d %5d %5llu %9llu", depth, val, time / 10, ctlr->nodes_searched);
		else if (engine->protocol == UCI)
			fprintf(stdout, "info depth %u score cp %d nodes %llu time %llu pv", depth, val, ctlr->nodes_searched, time);
		best_move = get_stored_moves(pos, depth);
		fprintf(stdout, "\n");
	}
#ifdef STATS
	fprintf(stdout, "nps:                      %lf\n",
		time ? ((double)ctlr->nodes_searched * 1000 / time) : 0);
	fprintf(stdout, "iid cutoff rate:          %lf\n",
		((double)pos->stats.iid_cutoffs) / pos->stats.iid_tries);
	fprintf(stdout, "null cutoff rate:         %lf\n",
		((double)pos->stats.null_cutoffs) / pos->stats.null_tries);
	fprintf(stdout, "hash hit rate:            %lf\n",
		((double)pos->stats.hash_hits) / pos->stats.hash_probes);
	fprintf(stdout, "ordering at cut nodes:    %lf\n",
		((double)pos->stats.first_beta_cutoffs) / pos->stats.beta_cutoffs);
	fprintf(stdout, "correct node predictions: %lf\n",
		((double)pos->stats.correct_nt_guess) / ctlr->nodes_searched);
#endif

	return best_move;
}

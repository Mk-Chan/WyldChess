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

	if (ss->ply) {
		if ( !(engine->ctlr->nodes_searched & 2047)
		    && stopped(engine))
			return 0;

		if (pos->state->fifty_moves > 99 || is_repeat(pos))
			return 0;

		if (ss->ply >= MAX_PLY)
			return evaluate(pos);
	}

	int pv_node = ss->pv_node;

	TT_Entry entry = tt_probe(&tt, pos->state->pos_key);
	Move tt_move   = 0;
#ifdef STATS
	++pos->stats.hash_probes;
#endif
	if ((entry.key ^ entry.data) == pos->state->pos_key) {
#ifdef STATS
		++pos->stats.hash_hits;
#endif
		tt_move = get_move(entry.data);
		if (   !pv_node
		    &&  DEPTH(entry.data) >= depth) {
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
	if (      depth >= 4
	    &&   !pv_node
	    &&    ss->early_prune
	    &&   !checked
	    && (((pos->bb[KING] | pos->bb[PAWN]) & pos->bb[pos->stm]) ^ pos->bb[pos->stm]) > 0ULL) {
#ifdef STATS
		++pos->stats.null_tries;
#endif
		int reduction = 4;
		do_null_move(pos);
		ss[1].pv_node = 0;
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

	// Internal iterative deepening
#ifdef STATS
	int iid = 0;
#endif
	// Conditions similar to stockfish as of now, seems to be effective
	if (   !tt_move
	    &&  depth > 5
	    && (pv_node || evaluate(pos) + mg_val(piece_val[PAWN]) >= beta)) {
#ifdef STATS
		iid = 1;
		++pos->stats.iid_tries;
#endif
		ss->early_prune = 0;
		ss->pv_node     = 1;
		int reduction   = 2;
		search(engine, ss, alpha, beta, depth - reduction);
		entry   = tt_probe(&tt, pos->state->pos_key);
		tt_move = get_move(entry.data);
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
	for (move = list->moves; move != list->end; ++move) {
		if (*move == tt_move) {
			encode_order(*move, HASH_MOVE);
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

			else if (     piece_type(pos->board[from_sq(*move)]) == PAWN
				 && !(file_forward_mask[pos->stm][from_sq(*move)] & pos->bb[PAWN] & pos->bb[pos->stm])
				 && !(passed_pawn_mask[pos->stm][from_sq(*move)] & pos->bb[PAWN] & pos->bb[!pos->stm]))
				encode_order(*move, PASSER_PUSH);
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

		// Check extension
		checking_move = (checkers(pos, !pos->stm) > 0ULL);
		if (    pv_node
		    &&  checking_move
		    && (cap_type(*move) ? order(*move) > BAD_CAP : see(pos, *move) >= -equal_cap_bound)) {
			ext = 1;
		}

		// Principal Variation Search
		if (legal_moves == 1) {
			ss[1].pv_node = 1;
			val = -search(engine, ss + 1, -beta, -alpha, depth_left + ext);
		} else {
			ss[1].pv_node     = 0;
			ss[1].early_prune = 1;
			// Late Move Reduction (LMR) -- Not completely confident of this yet
			if (    ss->ply
			    &&  depth_left > 2
			    &&  legal_moves > 1
			    &&  order(*move) <= PASSER_PUSH
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
		tt_store(&tt, alpha, FLAG_UPPER, depth, best_move, pos->state->pos_key);
	else
		tt_store(&tt, alpha, FLAG_EXACT, depth, best_move, pos->state->pos_key);

	return alpha;
}

int begin_search(Engine* const engine)
{
	int val;
	int best_move = 0;
	u64 time;
	// To accomodate (ss - 2) during killer move check at 0 and 1 ply when starting with ss + 2
	Search_Stack ss[MAX_PLY + 2];
	clear_search(engine, ss + 2);
	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;
	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	int alpha, beta;
	int asp_win_tries;
	static int asp_wins[] = { 10, 50, 200, INFINITY };
	int depth;
	for (depth = 1; depth <= max_depth; ++depth) {
		if (depth < 5) {
			ss[2].pv_node = 1;
			val = search(engine, ss + 2, -INFINITY, +INFINITY, depth);
		} else {
			asp_win_tries = 0;
			while (1) {
				ss[2].pv_node = 1;
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
		if (engine->protocol == CECP)
			fprintf(stdout, "%d %d %llu %llu", depth, val, time / 10, ctlr->nodes_searched);
		else if (engine->protocol == UCI)
			fprintf(stdout, "info depth %u score cp %d nodes %llu time %llu pv", depth, val, ctlr->nodes_searched, time);
		best_move = get_stored_moves(pos, depth);
		fprintf(stdout, "\n");
	}
#ifdef STATS
	fprintf(stdout, "iid cutoff rate:       %lf\n",
		((double)pos->stats.iid_cutoffs) / pos->stats.iid_tries);
	fprintf(stdout, "futility cutoff rate:  %lf\n",
		((double)pos->stats.futility_cutoffs) / pos->stats.futility_tries);
	fprintf(stdout, "null cutoff rate:      %lf\n",
		((double)pos->stats.null_cutoffs) / pos->stats.null_tries);
	fprintf(stdout, "hash hit rate:         %lf\n",
		((double)pos->stats.hash_hits) / pos->stats.hash_probes);
	fprintf(stdout, "ordering at cut nodes: %lf\n",
		((double)pos->stats.first_beta_cutoffs) / (pos->stats.beta_cutoffs));
#endif

	return best_move;
}

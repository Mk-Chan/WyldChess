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

#define HISTORY_LIM ((1 << 20))

int history[8][64];

static void reduce_history()
{
	int pt, sq;
	for (pt = PAWN; pt <= KING; ++pt)
		for (sq = 0; sq < 64; ++sq)
			history[pt][sq] /= 256;
}

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

			else
				*order = history[pos->board[from_sq(*move)]][to_sq(*move)];
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
	for (int i = 1; i < len; ++i) {
		if (order[i] > order[best_index])
			best_index = i;
	}
	u32 best_move = list->moves[best_index];
	--list->end;
	list->moves[best_index] = *list->end;
	order[best_index] = order[len - 1];
	return best_move;
}

static int qsearch(SearchUnit* const su, SearchStack* const ss, int alpha, int beta)
{
	Controller* const ctlr = su->ctlr;
	++ctlr->nodes_searched;

	if ( !(ctlr->nodes_searched & 0x7ff)
	    && stopped(su))
		return 0;

	Position* const pos = su->pos;

	if (pos->state->fifty_moves > 99)
		return 0;

	if (cap_type((pos->state - 1)->move)) {
		if (insufficient_material(pos))
			return 0;
	} else if (is_repeat(pos)) {
		return 0;
	}

	if (ss->ply >= MAX_PLY)
		return evaluate(pos);

	// Mate distance pruning
	alpha = max((-MATE + ss->ply), alpha);
	beta  = min((MATE - ss->ply - 1), beta);
	if (alpha >= beta)
		return alpha;

#ifdef STATS
	++pos->stats.correct_nt_guess;
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

	Movelist* list = &ss->list;
	list->end      = list->moves;
	set_pinned(pos);
	if (checked) {
		gen_check_evasions(pos, list);
		if (list->end == list->moves)
			return -MATE + ss->ply;
	} else {
		gen_quiesce_moves(pos, list);
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

		do_move(pos, move);
		val = -qsearch(su, ss + 1, -beta, -alpha);
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

static int search(SearchUnit* const su, SearchStack* const ss, int alpha, int beta, int depth)
{
	if (depth <= 0)
		return qsearch(su, ss, alpha, beta);

	Position* const pos    = su->pos;
	Controller* const ctlr = su->ctlr;
	++ctlr->nodes_searched;
	int old_alpha = alpha;

	if (ss->ply) {
		if ( !(su->ctlr->nodes_searched & 0x7ff)
		    && stopped(su))
			return 0;

		if (   pos->state->fifty_moves > 99
		    || insufficient_material(pos)
		    || is_repeat(pos))
			return 0;

		if (ss->ply >= MAX_PLY)
			return evaluate(pos);

		// Mate distance pruning
		alpha = max((-MATE + ss->ply), alpha);
		beta  = min((MATE - ss->ply - 1), beta);
		if (alpha >= beta)
			return alpha;
	}

	int node_type = ss->node_type;

#ifdef STATS
	++pos->stats.hash_probes;
#endif
	TTEntry entry = tt_probe(&tt, pos->state->pos_key);
	u32 tt_move   = 0;
	if ((entry.key ^ entry.data) == pos->state->pos_key) {
#ifdef STATS
		++pos->stats.hash_hits;
#endif
		tt_move = get_move(entry.data);
		if (   node_type != PV_NODE
		    && DEPTH(entry.data) >= depth) {

			int val  = val_from_tt(SCORE(entry.data), ss->ply);
			u64 flag = FLAG(entry.data);

			if (    flag == FLAG_EXACT
			    || (flag == FLAG_LOWER && val >= beta)
			    || (flag == FLAG_UPPER && val <= alpha))
				return val;
		}
	}

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;
	int val;
	int static_eval;
	if (node_type != PV_NODE)
		static_eval = evaluate(pos);

	int non_pawn_pieces_count = popcnt((pos->bb[pos->stm] & ~(pos->bb[KING] ^ pos->bb[PAWN])));

	// Futility pruning
	if (    depth < 3
	    &&  node_type != PV_NODE
	    && !cap_type((pos->state - 1)->move)
	    &&  ss->early_prune
	    &&  non_pawn_pieces_count > 1
	    && !checked) {
		val = static_eval - (200 * depth);
		if (   abs(val) < MAX_MATE_VAL
		    && val >= beta)
			return val;
	}

	// Null move pruning
	if (    depth >= 4
	    &&  node_type == CUT_NODE
	    &&  ss->early_prune
	    && !checked
	    &&  static_eval >= beta
	    &&  non_pawn_pieces_count > 1) {
#ifdef STATS
		++pos->stats.null_tries;
#endif
		int reduction = 4 + min(3, max(0, (static_eval - beta) / mg_val(piece_val[PAWN])));
		int depth_left = max(1, depth - reduction);
		ss[1].node_type   = CUT_NODE;
		ss[1].early_prune = 0;
		do_null_move(pos);
		val = -search(su, ss + 1, -beta, -beta + 1, depth_left);
		undo_null_move(pos);
		if (ctlr->is_stopped)
			return 0;
		if (val >= beta) {
#ifdef STATS
			++pos->stats.null_cutoffs;
			++pos->stats.correct_nt_guess;
#endif
			if (val >= MAX_MATE_VAL)
				val = beta;

			return val;
		}
	}

	// Internal iterative deepening
#ifdef STATS
	int iid = 0;
#endif
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
		ss->early_prune = 0;
		search(su, ss, alpha, beta, depth - reduction);
		ss->early_prune = ep;

		entry   = tt_probe(&tt, pos->state->pos_key);
		tt_move = get_move(entry.data);
	}

	Movelist* list = &ss->list;
	list->end      = list->moves;
	set_pinned(pos);
	gen_pseudo_legal_moves(pos, list);

	order_moves(pos, ss, tt_move);

	int best_val    = -INFINITY,
	    best_move   = 0,
	    legal_moves = 0;
	int checking_move, depth_left;
	u32 move;
	while ((move = get_next_move(ss))) {
		if (!legal_move(pos, move))
			continue;
		++legal_moves;

		if (  !ss->ply
		    && su->protocol == UCI
		    && curr_time() - ctlr->search_start_time >= 1000) {
			char mstr[6];
			move_str(move, mstr);
			fprintf(stdout, "info currmovenumber %d currmove %s\n", legal_moves, mstr);
		}

		depth_left    = depth - 1;
		checking_move = gives_check(pos, move);

		// Check extension
		if (    checking_move
		    && (depth == 1 || see(pos, to_sq(move)) > -equal_cap_bound))
			++depth_left;

		// Heuristic pruning
		if (    ss->ply
		    &&  best_val > -MAX_MATE_VAL
		    &&  legal_moves > 1
		    &&  prom_type(move) != QUEEN
		    &&  non_pawn_pieces_count > 1
		    && !checking_move
		    && !cap_type(move)) {

			// Futility pruning
			if (   depth < 8
			    && node_type != PV_NODE
			    && static_eval + 100 * depth_left <= alpha)
				continue;

			// Late move reduction
			if (    depth > 2
			    &&  legal_moves > (node_type == PV_NODE ? 5 : 3)
			    &&  move != ss->killers[0]
			    &&  move != ss->killers[1]
			    && !checked) {
				int reduction = 2
					     + (legal_moves > 10)
					     + (node_type != PV_NODE);
				depth_left = max(1, depth - reduction);
			}
		}

		do_move(pos, move);

		// Principal Variation Search
		if (legal_moves == 1) {
			ss[1].node_type = node_type == PV_NODE  ? PV_NODE
				        : node_type == CUT_NODE ? ALL_NODE
					: CUT_NODE;
			ss[1].early_prune = 1;
			val = -search(su, ss + 1, -beta , -alpha, depth_left);
		} else if (node_type != PV_NODE) {
			ss[1].node_type   = CUT_NODE;
			ss[1].early_prune = 1;
			val = -search(su, ss + 1, -beta, -alpha, depth_left);
			if (   val > alpha
			    && depth_left < depth - 1) {
				ss[1].node_type   = ALL_NODE;
				val = -search(su, ss + 1, -beta, -alpha, depth - 1);
			}
		} else {
			ss[1].node_type   = CUT_NODE;
			ss[1].early_prune = 1;
			val = -search(su, ss + 1, -alpha - 1, -alpha, depth_left);
			if (val > alpha) {
				ss[1].node_type = PV_NODE;
				val = -search(su, ss + 1, -beta, -alpha, max(depth - 1, depth_left));
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

				if (  !cap_type(move)
				    && move_type(move) != ENPASSANT
				    && prom_type(move) != QUEEN) {
					int pt = pos->board[from_sq(move)];
					history[pt][to_sq(move)] += depth * depth;
					if (history[pt][to_sq(move)] > HISTORY_LIM)
						reduce_history();
				}

				if (val >= beta) {
#ifdef STATS
					if (legal_moves == 1)
						++pos->stats.first_beta_cutoffs;
					++pos->stats.beta_cutoffs;
					pos->stats.iid_cutoffs += iid;
#endif
					if (  !cap_type(move)
					    && ss->killers[0] != move
					    && move_type(move) != ENPASSANT
					    && prom_type(move) != QUEEN) {
						ss->killers[1] = ss->killers[0];
						ss->killers[0] = move;
					}
					break;
				}
			}
		}
	}

#ifdef STATS
	if (best_val >= beta)
		pos->stats.correct_nt_guess += (node_type == CUT_NODE);
	else if (best_val > old_alpha)
		pos->stats.correct_nt_guess += (node_type == PV_NODE);
	else
		pos->stats.correct_nt_guess += (node_type == ALL_NODE);
#endif

	if (  !ss->ply
	    && alpha < beta
	    && legal_moves == 1)
		ctlr->is_stopped = 1;

	if (!legal_moves) {
		if (checked)
			return -MATE + ss->ply;
		else
			return 0;
	}

	u64 flag = best_val >= beta     ? FLAG_LOWER
		 : best_val > old_alpha ? FLAG_EXACT
		 : FLAG_UPPER;

	tt_store(&tt, val_to_tt(best_val, ss->ply), flag, depth, best_move, pos->state->pos_key);
	if (   node_type == PV_NODE
	    && flag == FLAG_EXACT)
		pvt_store(&pvt, best_move, pos->state->pos_key, depth);

	return best_val;
}

int begin_search(SearchUnit* const su)
{
	u64 time;
	int val, alpha, beta, depth;
	int best_move = 0;

	memset(history, 0, sizeof(int) * 8 * 64);

	SearchStack ss[MAX_PLY + 2];
	clear_search(su, ss + 2);
	pvt_clear(&pvt);

	Position* const pos    = su->pos;
	Controller* const ctlr = su->ctlr;
	ss[2].early_prune = 0;
	ss[2].node_type   = PV_NODE;

	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	static int deltas[] = { 10, 25, 50, 100, 200, INFINITY };
	static int* delta;
	for (depth = 1; depth <= max_depth; ++depth) {
		delta = deltas;
		while (1) {
			if (depth < 5) {
				alpha = -INFINITY;
				beta  =  INFINITY;
			} else {
				alpha = max(val - *delta, -INFINITY);
				beta  = min(val + *delta, +INFINITY);
			}

			val = search(su, ss + 2, alpha, beta, depth);

			if (   depth > 1
			    && ctlr->is_stopped)
				break;

			time = curr_time() - ctlr->search_start_time;
			if (su->protocol == XBOARD) {
				fprintf(stdout, "%3d %5d %5llu %9llu", depth, val, time / 10, ctlr->nodes_searched);
			} else if (su->protocol == UCI) {
				fprintf(stdout, "info ");
				fprintf(stdout, "depth %u ", depth);
				fprintf(stdout, "score ");
				if (abs(val) < MAX_MATE_VAL) {
					printf("cp %d ", val);
				} else {
					printf("mate ");
					if (val < 0)
						printf("%d ", (-val - MATE) / 2);
					else
						printf("%d ", (-val + MATE + 1) / 2);
				}
				fprintf(stdout, "nodes %llu ", ctlr->nodes_searched);
				fprintf(stdout, "time %llu ", time);
				fprintf(stdout, "pv");
			}
			print_pv_line(pos, depth);
			fprintf(stdout, "\n");

			if (val <= alpha || val >= beta)
				++delta;
			else
				break;
		}

		if (   depth > 1
		    && ctlr->is_stopped)
			break;

		best_move = get_pv_move(pos);
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

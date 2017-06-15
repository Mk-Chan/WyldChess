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
#include "syzygy/tbprobe.h"

#define HISTORY_LIM (8000)
#define MAX_HISTORY_DEPTH (12)

// TODO: Following are unsafe for multithreading
u64 tb_hits;
int history[8][64];
u32 counter_move_table[64][64];

void init_search()
{
	memset(history, 0, sizeof(int) * 8 * 64);
	memset(counter_move_table, 0, sizeof(u32) * 64 * 64);
}

static void reduce_history()
{
	int pt, sq, c;
	for (pt = PAWN; pt <= KING; ++pt)
		for (c = WHITE; c <= BLACK; ++c)
			for (sq = 0; sq < 64; ++sq)
				history[pt][sq] /= 16;
}

static void order_moves(struct Position* const pos, struct SearchStack* const ss, u32 tt_move)
{
	struct Movelist* list = &ss->list;
	int* order_list = ss->order_arr;
	int prev_to = -1, prev_from = -1;
	if (ss->ply) {
		int prev_move = (pos->state-1)->move;
		prev_to       = to_sq(prev_move);
		prev_from     = from_sq(prev_move);
	}
	u32* move;
	int* order;
	for (move = list->moves, order = order_list; move < list->end; ++move, ++order) {
		if (*move == tt_move) {
			*order = HASH_MOVE;
		} else if (   cap_type(*move)
			   || move_type(*move) == ENPASSANT) {
			*order = cap_order(pos, *move);
		} else {
			if (*move == ss->killers[0])
				*order = KILLER + 1;

			else if (*move == ss->killers[1])
				*order = KILLER;

			else if (    ss->ply
				 && *move == counter_move_table[prev_from][prev_to])
				*order = COUNTER;

			else
				*order = history[pos->board[from_sq(*move)]][to_sq(*move)];
		}
	}
}

static u32 get_next_move(struct SearchStack* const ss, int move_num)
{
	struct Movelist* list = &ss->list;
	int len = list->end - list->moves;
	if (move_num >= len)
		return 0;
	int* order = ss->order_arr;
	int best_index = move_num;
	for (int i = best_index + 1; i < len; ++i) {
		if (order[i] > order[best_index])
			best_index = i;
	}
	u32 best_move = list->moves[best_index];
	if (best_index != move_num) {
		list->moves[best_index] = list->moves[move_num];
		list->moves[move_num]   = best_move;
		int best_order          = order[best_index];
		order[best_index]       = order[move_num];
		order[move_num]         = best_order;
	}
	return best_move;
}

static int qsearch(struct SearchUnit* const su, struct SearchStack* const ss, int alpha, int beta)
{
	struct Controller* const ctlr = su->ctlr;

	if ( !(ctlr->nodes_searched & 0x7ff)
	    && stopped(su))
		return 0;

	struct Position* const pos = su->pos;
	++ctlr->nodes_searched;
	STATS(++pos->stats.correct_nt_guess;) // Ignore qsearch node guesses

	if (pos->state->fifty_moves > 99)
		return 0;

	if (  !cap_type((pos->state - 1)->move)
	    && is_repeat(pos))
		return 0;

	if (ss->ply >= MAX_PLY)
		return evaluate(pos);

	// Mate distance pruning
	alpha = max((-MATE + ss->ply), alpha);
	beta  = min((MATE - ss->ply), beta);
	if (alpha >= beta)
		return alpha;

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;
	int eval = 0;

	if (!checked) {
		eval = evaluate(pos);
		if (eval >= beta)
			return eval;
		if (eval > alpha)
			alpha = eval;
	}

	struct Movelist* list = &ss->list;
	list->end = list->moves;
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
	STATS(u32 legal_moves = 0;)
	int move_num = 0;
	u32 move;
	while ((move = get_next_move(ss, move_num++))) {
		if (!legal_move(pos, move))
			continue;

		STATS(++legal_moves;)

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
			STATS(
				if (legal_moves == 1)
					++pos->stats.first_beta_cutoffs;
				++pos->stats.beta_cutoffs;
			)
			return beta;
		}
		if (val > alpha)
			alpha = val;
	}

	return alpha;
}

static int search(struct SearchUnit* const su, struct SearchStack* const ss, int alpha, int beta, int depth)
{
	if (depth <= 0)
		return qsearch(su, ss, alpha, beta);

	struct Position* const pos = su->pos;
	struct Controller* const ctlr = su->ctlr;
	++ctlr->nodes_searched;
	int old_alpha = alpha;

	if (ss->ply > su->max_searched_ply)
		su->max_searched_ply = ss->ply;

	if (ss->ply) {
		if ( !(su->ctlr->nodes_searched & 0x7ff)
		    && stopped(su))
			return 0;

		if (   pos->state->fifty_moves > 99
		    || is_repeat(pos))
			return 0;

		if (ss->ply >= MAX_PLY)
			return evaluate(pos);

		// Mate distance pruning
		alpha = max((-MATE + ss->ply), alpha);
		beta  = min((MATE - ss->ply), beta);
		if (alpha >= beta)
			return alpha;
	}

	int node_type = ss->node_type;

	// Probe TT
	STATS(++pos->stats.hash_probes;)
	struct TTEntry entry = tt_probe(&tt, pos->state->pos_key);
	u32 tt_move = 0;
	if ((entry.key ^ entry.data) == pos->state->pos_key) {
		STATS(++pos->stats.hash_hits;)
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

	// Probe EGTB
	// No castling allowed
	// No fifty moves allowed
	if (    TB_LARGEST > 0
	    && !pos->state->castling_rights
	    && !pos->state->fifty_moves
	    &&  popcnt(pos->bb[FULL]) <= TB_LARGEST) {
		u64* bb = pos->bb;
		int ep_sq = pos->state->ep_sq_bb ? bitscan(pos->state->ep_sq_bb) : 0;
		if (ss->ply) {
			// At interior/leaf node only check the result of the position
			unsigned int wdl = tb_probe_wdl(bb[WHITE], bb[BLACK], bb[KING], bb[QUEEN], bb[ROOK],
							bb[BISHOP], bb[KNIGHT], bb[PAWN],
							ep_sq, pos->stm == WHITE);
			if (wdl != TB_RESULT_FAILED) {
				++tb_hits;
				tt_store(&tt, tb_values[wdl], FLAG_EXACT, min(depth + 6, MAX_PLY - 1), 0, pos->state->pos_key);
				return tb_values[wdl];
			}
		} else {
			// At root node construct best move and put it into PV-table
			unsigned int res = tb_probe_root(bb[WHITE], bb[BLACK], bb[KING], bb[QUEEN], bb[ROOK],
							bb[BISHOP], bb[KNIGHT], bb[PAWN], pos->state->fifty_moves,
							ep_sq, pos->stm == WHITE, NULL);
			if (res != TB_RESULT_FAILED) {
				u32 prom = 0;
				switch (TB_GET_PROMOTES(res)) {
				case TB_PROMOTES_QUEEN:
					prom = TO_QUEEN;
					break;
				case TB_PROMOTES_KNIGHT:
					prom = TO_KNIGHT;
					break;
				case TB_PROMOTES_ROOK:
					prom = TO_ROOK;
					break;
				case TB_PROMOTES_BISHOP:
					prom = TO_BISHOP;
					break;
				default:
					break;
				}
				int to   = TB_GET_TO(res);
				u32 move = prom ? move_prom(TB_GET_FROM(res), to, prom)
					        : move_normal(TB_GET_FROM(res), to);
				pvt_store(&pvt, move, pos->state->pos_key, depth);
				ctlr->is_stopped = 1;
				return tb_values[TB_GET_WDL(res)];
			}
		}
	}

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;
	int static_eval = 0;
	if (node_type != PV_NODE)
		static_eval = evaluate(pos);

	int non_pawn_pieces_count = popcnt((pos->bb[pos->stm] & ~(pos->bb[KING] ^ pos->bb[PAWN])));

	// Forward pruning
	if (    node_type != PV_NODE
	    &&  non_pawn_pieces_count
	    && !checked
	    &&  ss->forward_prune) {

		// Futility pruning
		if (   depth < 3
		    && static_eval < WINNING_SCORE
		    && static_eval - 200 * depth >= beta)
			return static_eval;

		// Null move pruning
		if (   depth >= 4
		    && node_type == CUT_NODE
		    && static_eval >= beta) {
			STATS(++pos->stats.null_tries;)
			int reduction       = 4 + min(3, max(0, (static_eval - beta) / mg_val(piece_val[PAWN])));
			int depth_left      = max(1, depth - reduction);
			ss[1].node_type     = ALL_NODE;
			ss[1].forward_prune = 0;
			do_null_move(pos);
			int val = -search(su, ss + 1, -beta, -beta + 1, depth_left);
			undo_null_move(pos);
			if (ctlr->is_stopped)
				return 0;
			if (val >= beta) {
				STATS(
					++pos->stats.null_cutoffs;
					++pos->stats.correct_nt_guess;
				)
				if (val >= MAX_MATE_VAL)
					val = beta;

				return val;
			}
		}
	}

	// Internal iterative deepening
	// Conditions similar to stockfish as of now, seems to be effective
	STATS(int iid = 0;)
	if (   !tt_move
	    &&  depth >= 5
	    && (node_type == PV_NODE || static_eval + mg_val(piece_val[PAWN]) >= beta)) {
		STATS(
			iid = 1;
			++pos->stats.iid_tries;
		)

		int reduction     = 2;
		int ep            = ss->forward_prune;
		ss->forward_prune = 0;
		search(su, ss, alpha, beta, depth - reduction);
		ss->forward_prune = ep;

		entry   = tt_probe(&tt, pos->state->pos_key);
		tt_move = get_move(entry.data);
	}

	struct Movelist* list = &ss->list;
	list->end = list->moves;
	set_pinned(pos);
	gen_legal_moves(pos, list);

	order_moves(pos, ss, tt_move);

	u32 counter_move = 0;
	if (ss->ply)
		counter_move = counter_move_table[from_sq((pos->state-1)->move)][to_sq((pos->state-1)->move)];

	int best_val    = -INFINITY,
	    best_move   = 0,
	    legal_moves = 0;
	int checking_move, depth_left, val;
	u32 move;
	while ((move = get_next_move(ss, legal_moves))) {
		++legal_moves;

		if (  !ss->ply
		    && su->protocol == UCI) {
			u64 time_passed = curr_time() - ctlr->search_start_time;
			if (time_passed >= 1000ULL) {
				char mstr[6];
				move_str(move, mstr);
				fprintf(stdout, "info currmovenumber %d currmove %s nps %llu\n",
					legal_moves, mstr, ctlr->nodes_searched * 1000 / time_passed);
			}
		}

		depth_left    = depth - 1;
		checking_move = gives_check(pos, move);

		// Check extension
		if (    checking_move
		    && (depth == 1 || see(pos, to_sq(move)) > -equal_cap_bound))
			++depth_left;

		// Heuristic pruning and reductions
		if (    ss->ply
		    &&  best_val > -MAX_MATE_VAL
		    &&  legal_moves > 1
		    &&  prom_type(move) != QUEEN
		    &&  non_pawn_pieces_count
		    && !checking_move
		    && !cap_type(move)) {

			// Futility pruning
			if (   depth < 8
			    && node_type != PV_NODE
			    && static_eval + 100 * depth_left <= alpha)
				continue;

			// Prune moves with horrible SEE at low depth(idea from Stockfish)
			if (   depth < 8
			    && see(pos, move) < -10 * depth * depth)
				continue;

			int passer_move = is_passed_pawn(pos, from_sq(move), pos->stm)
				  && (pos->stm == WHITE ? rank_of(to_sq(move)) >= RANK_6 : rank_of(to_sq(move)) <= RANK_3);

			// Late move reduction
			if (    depth > 2
			    &&  legal_moves > (node_type == PV_NODE ? 5 : 3)
			    &&  move != ss->killers[0]
			    &&  move != ss->killers[1]
			    &&  move != counter_move
			    && !passer_move
			    && !checked) {
				int reduction = 2;
				int hist_val = history[pos->board[from_sq(move)]][to_sq(move)];
				reduction += (legal_moves > 10)
					   + (node_type != PV_NODE)
					   + (hist_val < -500)
					   + (hist_val < -3000)
					   - (hist_val > 500)
					   - (hist_val > 3000);
				depth_left = max(1, depth - max(2, reduction));
			}
		}

		do_move(pos, move);

		// Principal Variation Search
		if (legal_moves == 1) {
			ss[1].node_type = node_type == PV_NODE  ? PV_NODE
				        : node_type == CUT_NODE ? ALL_NODE
					: CUT_NODE;
			ss[1].forward_prune = 1;
			val = -search(su, ss + 1, -beta , -alpha, depth_left);
		} else if (node_type != PV_NODE) {
			ss[1].node_type     = CUT_NODE;
			ss[1].forward_prune = 1;
			val = -search(su, ss + 1, -beta, -alpha, depth_left);
			if (   val > alpha
			    && depth_left < depth - 1) {
				ss[1].node_type = ALL_NODE;
				val = -search(su, ss + 1, -beta, -alpha, depth - 1);
			}
		} else {
			ss[1].node_type     = CUT_NODE;
			ss[1].forward_prune = 1;
			val = -search(su, ss + 1, -alpha - 1, -alpha, depth_left);
			if (val > alpha) {
				ss[1].node_type = PV_NODE;
				val = -search(su, ss + 1, -beta, -alpha, max(depth - 1, depth_left));
			}
		}

		undo_move(pos);

		if (ctlr->is_stopped)
			return 0;

		int quiet_move =   !cap_type(move)
				 && move_type(move) != ENPASSANT
				 && prom_type(move) != QUEEN;

		if (val > best_val) {
			best_val  = val;
			best_move = move;

			if (val > alpha) {
				alpha = val;

				if (   quiet_move
				    && depth <= MAX_HISTORY_DEPTH) {
					int pt = pos->board[from_sq(move)];
					history[pt][to_sq(move)] += depth * depth;
					if (history[pt][to_sq(move)] > HISTORY_LIM)
						reduce_history();
				}

				if (val >= beta) {
					STATS(
						if (legal_moves == 1)
							++pos->stats.first_beta_cutoffs;
						++pos->stats.beta_cutoffs;
						pos->stats.iid_cutoffs += iid;
					)
					if (quiet_move) {
						if (ss->killers[0] != move) {
							ss->killers[1] = ss->killers[0];
							ss->killers[0] = move;
						}
						int prev_move = (pos->state-1)->move;
						if (prev_move)
							counter_move_table[from_sq(prev_move)][to_sq(prev_move)] = move;
					}

					if (depth <= MAX_HISTORY_DEPTH) {
						for (u32* curr = list->moves + legal_moves - 2; curr >= list->moves; --curr) {
							if (  !cap_type(*curr)
							    && move_type(*curr) != ENPASSANT
							    && prom_type(*curr) != QUEEN) {
								int pt = pos->board[from_sq(*curr)];
								history[pt][to_sq(*curr)] -= depth * depth;
								if (history[pt][to_sq(*curr)] < -HISTORY_LIM)
									reduce_history();
							}
						}
					}
					break;
				}
			}
		}
	}

	STATS(
		if (best_val >= beta)
			pos->stats.correct_nt_guess += (node_type == CUT_NODE);
		else if (best_val > old_alpha)
			pos->stats.correct_nt_guess += (node_type == PV_NODE);
		else
			pos->stats.correct_nt_guess += (node_type == ALL_NODE);
	)

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
	if (flag == FLAG_EXACT)
		pvt_store(&pvt, best_move, pos->state->pos_key, depth);

	return best_val;
}

int begin_search(struct SearchUnit* const su)
{
	u64 time;
	int val, alpha, beta, depth;
	int best_move = 0;

	reduce_history();

	struct SearchStack ss[MAX_PLY];
	clear_search(su, ss);

	tt_clear(&pvt);
	tb_hits = 0ULL;

	ss->forward_prune    = 0;
	ss->node_type        = PV_NODE;
	su->max_searched_ply = 0;

	u64 old_node_counts[2];

	struct Controller* const ctlr = su->ctlr;
	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	static int deltas[] = { 10, 25, 50, 100, 200, INFINITY };
	static int* alpha_delta;
	static int* beta_delta;
	for (depth = 1; depth <= max_depth; ++depth) {
		alpha_delta = beta_delta = deltas;
		if (depth < 5) {
			alpha = -INFINITY;
			beta  =  INFINITY;
		} else {
			alpha = max(val - *alpha_delta, -INFINITY);
			beta  = min(val + *beta_delta, +INFINITY);
		}
		while (1) {
			val = search(su, ss, alpha, beta, depth);

			if (   depth > 1
			    && ctlr->is_stopped)
				goto end_search;

			time = curr_time() - ctlr->search_start_time;
			if (su->protocol == XBOARD) {
				fprintf(stdout, "%3d %5d %5llu %9llu", depth, val, time / 10, ctlr->nodes_searched);
			} else if (su->protocol == UCI) {
				fprintf(stdout, "info ");
				fprintf(stdout, "depth %u ", depth);
				fprintf(stdout, "seldepth %u ", su->max_searched_ply);
				fprintf(stdout, "tbhits %llu ", tb_hits);
				if (depth > 2)
					fprintf(stdout, "ebf %lf ", sqrt((double)ctlr->nodes_searched / old_node_counts[1]));
				fprintf(stdout, "score ");
				if (abs(val) < MAX_MATE_VAL) {
					fprintf(stdout, "cp %d ", val);
				} else {
					fprintf(stdout, "mate ");
					if (val < 0)
						fprintf(stdout, "%d ", (-val - MATE) / 2);
					else
						fprintf(stdout, "%d ", (-val + MATE + 1) / 2);
				}
				fprintf(stdout, "nodes %llu ", ctlr->nodes_searched);
				if (time > 1000ULL)
					fprintf(stdout, "nps %llu ", ctlr->nodes_searched * 1000 / time);
				fprintf(stdout, "time %llu ", time);
				fprintf(stdout, "pv");
			}
			print_pv_line(su->pos, depth);
			fprintf(stdout, "\n");

			if (depth > 1)
				old_node_counts[1] = old_node_counts[0];
			old_node_counts[0] = ctlr->nodes_searched;

			if (val <= alpha) {
				++alpha_delta;
				alpha -= (alpha - val) + *alpha_delta;
			} else if (val >= beta) {
				++beta_delta;
				beta += (val - beta) + *beta_delta;
			} else {
				break;
			}
		}

		best_move = get_pv_move(su->pos);
	}
end_search:;
	STATS(
		struct Position* const pos = su->pos;
		time = curr_time() - ctlr->search_start_time;
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
	)

	return best_move;
}

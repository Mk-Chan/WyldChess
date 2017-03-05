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

#include "tune.h"
#include "position.h"
#include "search.h"

#define MAX_FEN_LEN (150)
#define EPD_FILE_NAME ("epd_list.epd")
#define PARAM_FILE ("param_file")

static double K = 1.1;
static Movelist glist[MAX_PLY];

static int qsearch(Position* pos, int ply, int alpha, int beta)
{
	if (pos->state->fifty_moves > 99)
		return 0;

	if (ply >= MAX_PLY)
		return evaluate(pos);

	// Mate distance pruning
	alpha = max((-MAX_MATE_VAL + ply), alpha);
	beta  = min((MAX_MATE_VAL - ply), beta);
	if (alpha >= beta)
		return alpha;

	set_checkers(pos);
	int checked = pos->state->checkers_bb > 0ULL;

	if (!checked) {
		int eval = evaluate(pos);
		if (eval >= beta)
			return eval;
		if (eval > alpha)
			alpha = eval;
	}

	Movelist* list = glist + ply;
	list->end      = list->moves;
	set_pinned(pos);
	Move* move;
	if (checked) {
		gen_check_evasions(pos, list);
		if (list->end == list->moves)
			return -INFINITY + ply;

	} else {
		gen_captures(pos, list);
	}

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

	int val;
	for (move = list->moves; move != list->end; ++move) {
		if (  !checked
		    && order(*move) < BAD_CAP)
			break;

		if (!legal_move(pos, *move))
			continue;
		do_move(pos, *move);
		val = -qsearch(pos, ply + 1, -beta, -alpha);
		undo_move(pos);
		if (val >= beta) {
			return beta;
		}
		if (val > alpha)
			alpha = val;
	}

	return alpha;
}

static double sigmoid(double score)
{
	return 1.0 / (1.0 + pow(10.0, ((-K * score) / 400.0)));
}

static void dump_values()
{
	FILE* f = fopen(PARAM_FILE, "w");
	Tunable* t = tunables;
	int i;
	for (; t < tunables_end; ++t)
		for (i = 0; i < t->size; ++i)
			fprintf(f, "%s[%d]:%d\n", t->name, i, t->get_val(i));
	fclose(f);
}

double calc_error()
{
	double res = 0.0;
	FILE* fen_file = fopen(EPD_FILE_NAME, "r");
	int count = 0;
	int fen_index;
	int eval;
	double R, curr_error;
	char fen[MAX_FEN_LEN];
	static State state_list[MAX_PLY];
	static Position pos;
	while (fgets(fen, MAX_FEN_LEN, fen_file)) {
		init_pos(&pos, state_list);
		fen_index = set_pos(&pos, fen);
		R = fen[fen_index] - '0';
		if (!R && fen[fen_index + 1] == '.')
			R = 0.5;
		eval = qsearch(&pos, 0, -INFINITY, +INFINITY);
		if (pos.stm == BLACK)
			eval = -eval;
		if (   abs(eval) >= 500
		    || pos.state->phase < 30)
			continue;
		curr_error = R - sigmoid(eval);
		res += curr_error * curr_error;
		++count;
	}
	fclose(fen_file);
	return res / count;
}

void tune()
{
	double curr_error = calc_error();
	printf("Current error = %f\n", curr_error);
	double new_error;
	int improved, i, local_improved;
	Tunable* t;
	do {
		improved = 0;
		t = tunables;
		for (t = tunables; t < tunables_end; ++t) {
			for (i = 0; i < t->size; ++i) {
				do {
					local_improved = 0;
					t->inc_val(1, i);
					new_error = calc_error();
					if (new_error < curr_error - 0.000001) {
						t->score = curr_error - new_error;
						curr_error = new_error;
						printf("New error: %f\n", curr_error);
						printf("%s[%d] = %d\n", t->name, i, t->get_val(i));
						local_improved = 1;
						improved = 1;
					} else {
						t->inc_val(-2, i);
						new_error = calc_error();
						if (new_error < curr_error - 0.000001) {
							t->score = curr_error - new_error;
							curr_error = new_error;
							printf("New error: %f\n", curr_error);
							printf("%s[%d] = %d\n", t->name, i, t->get_val(i));
							local_improved = 1;
							improved = 1;
						} else {
							t->inc_val(1, i);
						}
					}
					dump_values();
				} while (local_improved);
			}
		}
		qsort(tunables, tunables_end - tunables, sizeof(Tunable), &tunable_comparator);
	} while (improved);
	printf("Done!\n");
}

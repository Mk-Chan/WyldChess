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

#include "syzygy/tbprobe.h"
#include "defs.h"
#include "search_unit.h"
#include "tt.h"
#include "options.h"
#include "search.h"

static inline void print_spin_option(struct SpinOption* option)
{
	fprintf(stdout, "option name %s type spin default %d min %d max %d\n",
		option->name, option->curr_val, option->min_val, option->max_val);
}

static inline void print_eval_term_spin_option(struct EvalTerm* term, int num)
{
	if (num < TAPERED_END) {
		char str[53];
		int len = strlen(term->name);
		memcpy(str, term->name, len * sizeof(char));
		str[len] = 'M';
		str[len+1] = 'g';
		str[len+2] = '\0';
		fprintf(stdout, "option name %s type spin default %d min %d max %d\n",
			str, mg_val(*term->ptr), -10000, 10000);
		str[len] = 'E';
		fprintf(stdout, "option name %s type spin default %d min %d max %d\n",
			str, eg_val(*term->ptr), -10000, 10000);
	} else {
		fprintf(stdout, "option name %s type spin default %d min %d max %d\n",
			term->name, *term->ptr, -10000, 10000);
	}
}

static inline void print_options_uci()
{
	fprintf(stdout, "id name %s\n", ENGINE_NAME);
	fprintf(stdout, "id author %s\n", AUTHOR_NAME);
	fprintf(stdout, "option name UCI_Chess960 type check default false\n");
	fprintf(stdout, "option name Ponder type check default true\n");
	fprintf(stdout, "option name SyzygyPath type string default <empty>\n");
	fprintf(stdout, "option name Hash type spin default 128 min 1 max 1048576\n");

	struct SpinOption* curr = spin_options;
	struct SpinOption* end  = spin_options + arr_len(spin_options);
	for (; curr != end; ++curr)
		print_spin_option(curr);

	struct EvalTerm* curr_et = eval_terms;
	struct EvalTerm* end_et  = eval_terms + NUM_TERMS;
	for (; curr_et != end_et; ++curr_et)
		print_eval_term_spin_option(curr_et, curr_et - eval_terms);

	fprintf(stdout, "uciok\n");
}

void* su_loop_uci(void* args)
{
	char mstr[6];
	u32 move;
	struct SearchUnit* su = (struct SearchUnit*) args;
	while (1) {
		switch(su->target_state) {
		case WAITING:
			su->curr_state = WAITING;
			pthread_mutex_lock(&su->mutex);
			while (su->target_state == WAITING)
				pthread_cond_wait(&su->sleep_cv, &su->mutex);
			pthread_mutex_unlock(&su->mutex);
			break;

		case THINKING:
			su->curr_state = THINKING;
			move = begin_search(su);
			move_str(move, mstr);
			fprintf(stdout, "bestmove %s", mstr);
			if (su->ponder_allowed) {
				move_str(su->ponder_move, mstr);
				fprintf(stdout, " ponder %s", mstr);
			}
			fprintf(stdout, "\n");
			su->target_state = WAITING;
			break;

		case QUITTING:
			su->curr_state = QUITTING;
			pthread_exit(0);
		}
	}
}

void uci_loop()
{
	static int const max_len = 4096;
	char input[max_len];
	char* ptr;
	char* end;
	u32   move;

	print_options_uci();

	struct SearchUnit* su = search_units;
	init_search_unit(su);
	struct Position* pos    = &su->pos;
	struct Controller* ctlr = &controller;
	su->protocol = UCI;
	pthread_t* search_thread = search_threads;
	pthread_create(search_thread, NULL, su_loop_uci, (void*) su);
	pthread_detach(*search_thread);

	while (1) {
		fgets(input, max_len, stdin);
		input[strlen(input)-1] = '\0';
		if (!strncmp(input, "isready", 7)) {

			fprintf(stdout, "readyok\n");

		} else if (!strncmp(input, "ucinewgame", 10)) {

			init_search(&su->sl);
			tt_clear(&tt);

		} else if (!strncmp(input, "position", 8)) {

			transition(su, WAITING);
			ptr = input + 9;
			init_pos(pos);
			if (!strncmp(ptr, "startpos", 8)) {
				ptr += 9;
				set_pos(pos, INITIAL_POSITION);
			} else if (!strncmp(ptr, "fen", 3)) {
				ptr += 4;
				ptr += set_pos(pos, ptr);
			}
			if (  *(ptr - 1) != '\0'
			    && !strncmp(ptr, "moves", 5)) {
				while ((ptr = strstr(ptr, " "))) {
					++ptr;
					move = parse_move(pos, ptr);
					if (!legal_move(pos, move)) {
						char mstr[6];
						move_str(move, mstr);
						fprintf(stdout, "Illegal move: %s\n", mstr);
						break;
					}
					do_move(pos, move);
				}
			}

		} else if (!strncmp(input, "print", 5)) {

			transition(su, WAITING);
			print_board(pos);

		} else if (!strncmp(input, "stop", 4)) {

			transition(su, WAITING);

		} else if (!strncmp(input, "quit", 4)) {

			transition(su, WAITING);
			transition(su, QUITTING);
			goto cleanup_and_exit;

		} else if (!strncmp(input, "setoption name", 14)) {

			ptr = input + 15;
			if (!strncmp(ptr, "Hash", 4)) {
				ptr += 5;
				if (!strncmp(ptr, "value", 5))
					tt_alloc_MB(&tt, strtoul(ptr + 6, &end, 10));
			} else if (!strncmp(ptr, "SyzygyPath", 10)) {
				ptr += 11;
				if (!strncmp(ptr, "value", 5)) {
					tb_init(ptr + 6);
					fprintf(stdout, "info string Largest tablebase size = %u\n", TB_LARGEST);
				}
			} else if (!strncmp(ptr, "Ponder", 6)) {

				ptr += 7;
				if (!strncmp(ptr, "value", 5)) {
					ptr += 6;
					if (!strncmp(ptr, "false", 5)) {
						su->ponder_allowed = 0;
					} else if (!strncmp(ptr, "true", 4)) {
						su->ponder_allowed = 1;
					}
				}

			} else if (!strncmp(ptr, "UCI_Chess960", 12)) {

				ptr += 13;
				if (!strncmp(ptr, "value", 5)) {
					ptr += 6;
					if (!strncmp(ptr, "false", 5)) {
						is_frc = 0;
					} else if (!strncmp(ptr, "true", 4)) {
						is_frc = 1;
					}
				}
			} else {
				int found = 0;
				struct SpinOption* curr = spin_options;
				struct SpinOption* option_end = spin_options + arr_len(spin_options);
				for (; curr != option_end; ++curr) {
					int len = strlen(curr->name);
					if (!strncmp(ptr, curr->name, len)) {
						found = 1;
						ptr += len + 1;
						if (!strncmp(ptr, "value", 5)) {
							int value = strtoul(ptr + 6, &end, 10);
							if (   value <= curr->max_val
							    && value >= curr->min_val) {
								curr->curr_val = value;
								if (curr->handler)
									curr->handler();
							}
						}
					}
				}
				if (!found) {
					struct EvalTerm* curr_et = eval_terms;
					struct EvalTerm* end_et  = eval_terms + NUM_TERMS;
					for (; curr_et != end_et; ++curr_et) {
						int len = strlen(curr_et->name);
						if (!strncmp(ptr, curr_et->name, len)) {
							if (curr_et - eval_terms < TAPERED_END) {
								ptr += len;
								if (!strncmp(ptr, "Mg", 2)) {
									ptr += 3;
									if (!strncmp(ptr, "value", 5)) {
										int value = strtoul(ptr + 6, &end, 10);
										if (value <= 10000 && value >= -10000)
											set_mg_val(*curr_et->ptr, value);
									}
								} else if (!strncmp(ptr, "Eg", 2)) {
									ptr += 3;
									if (!strncmp(ptr, "value", 5)) {
										int value = strtoul(ptr + 6, &end, 10);
										if (value <= 10000 && value >= -10000)
											set_eg_val(*curr_et->ptr, value);
									}
								}
							} else {
								ptr += len + 1;
								if (!strncmp(ptr, "value", 5)) {
									int value = strtoul(ptr + 6, &end, 10);
									if (value <= 10000 && value >= -10000)
										*curr_et->ptr = value;
								}
							}
						}
					}
				}

			}

		} else if (!strncmp(input, "perft", 5)) {

			transition(su, WAITING);
			performance_test(pos, atoi(input + 6));

		} else if (!strncmp(input, "ponderhit", 9)) {

			ctlr->search_end_time += curr_time() - ctlr->search_start_time;
			ctlr->time_dependent = 1;

		} else if (!strncmp(input, "go", 2)) {

			transition(su, WAITING);
			ctlr->time_dependent    = 1;
			ctlr->depth             = MAX_PLY;
			ctlr->moves_per_session = 40;
			ctlr->moves_left        = ctlr->moves_per_session;
			ctlr->time_left         = 240000;
			ctlr->increment         = 0;
			su->limited_moves_num   = 0;

			ptr = input;
			while ((ptr = strstr(ptr, " "))) {
				++ptr;
				if ((    pos->stm == WHITE
				     && !strncmp(ptr, "wtime", 5))
				    || ( pos->stm == BLACK
				     && !strncmp(ptr, "btime", 5))) {

					ctlr->time_left = strtoull(ptr + 6, &end, 10);
					ptr = end;

				} else if (!strncmp(ptr, "movestogo", 9)) {

					ctlr->moves_left = strtoull(ptr + 10, &end, 10);
					ptr = end;

				} else if ((    pos->stm == WHITE
				            && !strncmp(ptr, "winc", 4))
				           || ( pos->stm == BLACK
				            && !strncmp(ptr, "binc", 4))) {

					ctlr->increment = strtoull(ptr + 5, &end, 10);
					ptr = end;

				} else if (!strncmp(ptr, "depth", 5)) {

					ctlr->depth = (u32) strtol(ptr + 6, &end, 10);
					ctlr->time_dependent = 0;
					ptr = end;

				} else if (!strncmp(ptr, "movetime", 8)) {

					ctlr->time_left  = strtoull(ptr + 9, &end, 10);
					ctlr->moves_left = 1;
					ptr = end;

				} else if (!strncmp(ptr, "ponder", 6)) {

					ctlr->time_dependent = 0;

				} else if (!strncmp(ptr, "infinite", 8)) {

					ctlr->time_dependent = 0;

				} else if (!strncmp(ptr, "searchmoves", 11)) {

					su->limited_moves_num = 0;
					while ((ptr = strstr(ptr, " "))) {
						++ptr;
						move = parse_move(pos, ptr);
						if (!move) {
							printf("strleft: %s\n", ptr);
							--ptr;
							break;
						}
						if (!legal_move(pos, move)) {
							char mstr[6];
							move_str(move, mstr);
							fprintf(stdout, "Illegal move: %s\n", mstr);
							break;
						}
						su->limited_moves[su->limited_moves_num] = move;
						++su->limited_moves_num;
					}
					if (!ptr)
						break;

				}
			}

			start_thinking(su);

		}
	}
cleanup_and_exit:
	pthread_cond_destroy(&su->sleep_cv);
	pthread_mutex_destroy(&su->mutex);
}

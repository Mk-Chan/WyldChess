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

#include "syzygy/tbprobe.h"
#include "defs.h"
#include "search_unit.h"
#include "tt.h"

static inline void print_options_uci()
{
	fprintf(stdout, "id name %s\n", ENGINE_NAME);
	fprintf(stdout, "id author %s\n", AUTHOR_NAME);
	fprintf(stdout, "option name Hash type spin default 128 min 1 max 1048576\n");
	fprintf(stdout, "option name SyzygyPath type string default <empty>\n");
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
			fprintf(stdout, "bestmove %s\n", mstr);
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

	struct Position pos;
	struct Controller ctlr;
	struct SearchUnit su;
	su.protocol = UCI;
	pthread_mutex_init(&su.mutex, NULL);
	pthread_cond_init(&su.sleep_cv, NULL);
	su.pos  = &pos;
	su.ctlr = &ctlr;
	su.ctlr->depth = MAX_PLY;
	su.ctlr->time_dependent = 1;
	su.target_state = WAITING;
	struct State state_list[MAX_MOVES_PER_GAME + MAX_PLY];
	init_pos(&pos, state_list);
	set_pos(&pos, INITIAL_POSITION);
	pthread_t su_thread;
	pthread_create(&su_thread, NULL, su_loop_uci, (void*) &su);
	pthread_detach(su_thread);

	while (1) {
		fgets(input, max_len, stdin);
		input[strlen(input)-1] = '\0';
		if (!strncmp(input, "isready", 7)) {

			fprintf(stdout, "readyok\n");

		} else if (!strncmp(input, "position", 8)) {

			transition(&su, WAITING);
			ptr = input + 9;
			su.game_over = 0;
			init_pos(&pos, state_list);
			if (!strncmp(ptr, "startpos", 8)) {
				ptr += 9;
				set_pos(&pos, INITIAL_POSITION);
			} else if (!strncmp(ptr, "fen", 3)) {
				ptr += 4;
				ptr += set_pos(&pos, ptr);
			}
			if (  *(ptr - 1) != '\0'
			    && !strncmp(ptr, "moves", 5)) {
				while ((ptr = strstr(ptr, " "))) {
					++ptr;
					move = parse_move(&pos, ptr);
					if (!legal_move(&pos, move)) {
						char mstr[6];
						move_str(move, mstr);
						fprintf(stdout, "Illegal move: %s\n", mstr);
						break;
					}
					do_move(&pos, move);
				}
			}

		} else if (!strncmp(input, "print", 5)) {

			transition(&su, WAITING);
			print_board(&pos);

		} else if (!strncmp(input, "stop", 4)) {

			transition(&su, WAITING);

		} else if (!strncmp(input, "quit", 4)) {

			transition(&su, WAITING);
			transition(&su, QUITTING);
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
			}

		} else if (!strncmp(input, "perft", 5)) {

			transition(&su, WAITING);
			performance_test(&pos, atoi(input + 6));

		} else if (!strncmp(input, "go", 2)) {

			transition(&su, WAITING);
			su.game_over           = 0;
			ctlr.time_dependent    = 1;
			ctlr.moves_per_session = 0;
			ctlr.moves_left        = 40;
			ctlr.depth             = MAX_PLY;
			ctlr.increment         = 0;
			ctlr.time_left         = 240000;
			ptr = input;
			while ((ptr = strstr(ptr, " "))) {
				++ptr;
				if ((    pos.stm == WHITE
				     && !strncmp(ptr, "wtime", 5))
				    || ( pos.stm == BLACK
				     && !strncmp(ptr, "btime", 5))) {

					ctlr.time_left = strtoull(ptr + 6, &end, 10);
					ptr = end;

				} else if (!strncmp(ptr, "movestogo", 9)) {

					ctlr.moves_left = strtoull(ptr + 10, &end, 10);
					ptr = end;

				} else if ((    pos.stm == WHITE
				            && !strncmp(ptr, "winc", 4))
				           || ( pos.stm == BLACK
				            && !strncmp(ptr, "binc", 4))) {

					ctlr.increment = strtoull(ptr + 5, &end, 10);
					ptr = end;

				} else if (!strncmp(ptr, "depth", 5)) {

					ctlr.depth = (u32) strtol(ptr + 6, &end, 10);
					ctlr.time_dependent = 0;
					ptr = end;

				} else if (!strncmp(ptr, "movetime", 8)) {

					ctlr.time_left  = strtoull(ptr + 9, &end, 10);
					ctlr.moves_left = 1;
					ptr = end;

				} else if (!strncmp(ptr, "infinite", 8)) {

					ctlr.time_dependent = 0;

				}
			}

			start_thinking(&su);

		}
	}
cleanup_and_exit:
	pthread_cond_destroy(&su.sleep_cv);
	pthread_mutex_destroy(&su.mutex);
}

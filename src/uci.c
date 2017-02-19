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

#include "defs.h"
#include "engine.h"
#include "tt.h"
#include "tune.h"

static inline void print_options_uci()
{
	fprintf(stdout, "id name %s\n", ENGINE_NAME);
	fprintf(stdout, "id author %s\n", AUTHOR_NAME);
	fprintf(stdout, "option name Hash type spin default 128 min 1 max 1048576\n");
	fprintf(stdout, "uciok\n");
}

void* engine_loop_uci(void* args)
{
	char mstr[6];
	Move move;
	Engine* engine = (Engine*) args;
	while (1) {
		switch(engine->target_state) {
		case WAITING:
			engine->curr_state = WAITING;
#ifndef TEST
			pthread_mutex_lock(&engine->mutex);
			while (engine->target_state == WAITING)
				pthread_cond_wait(&engine->sleep_cv, &engine->mutex);
			pthread_mutex_unlock(&engine->mutex);
#endif
			break;

		case THINKING:
			engine->curr_state = THINKING;
			move = begin_search(engine);
			move_str(move, mstr);
			fprintf(stdout, "bestmove %s\n", mstr);
			engine->target_state = WAITING;
			break;

		case QUITTING:
			engine->curr_state = QUITTING;
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
	Move move;

	print_options_uci();

	Position pos;
	Controller ctlr;
	ctlr.depth = MAX_PLY;
	Engine engine;
	engine.protocol = UCI;
	pthread_mutex_init(&engine.mutex, NULL);
	pthread_cond_init(&engine.sleep_cv, NULL);
	engine.pos  = &pos;
	engine.ctlr = &ctlr;
	engine.ctlr->time_dependent = 1;
	engine.target_state = WAITING;
	init_pos(&pos);
	set_pos(&pos, INITIAL_POSITION);
	pthread_t engine_thread;
	pthread_create(&engine_thread, NULL, engine_loop_uci, (void*) &engine);
	pthread_detach(engine_thread);

	while (1) {
		fgets(input, max_len, stdin);
		if (!strncmp(input, "isready", 7)) {

			fprintf(stdout, "readyok\n");

		} else if (!strncmp(input, "position", 8)) {

			transition(&engine, WAITING);
			ptr = input + 9;
			engine.game_over = 0;
			init_pos(&pos);
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

			transition(&engine, WAITING);
			print_board(&pos);

		} else if (!strncmp(input, "stop", 4)) {

			transition(&engine, WAITING);

		} else if (!strncmp(input, "quit", 4)) {

			transition(&engine, QUITTING);
			goto cleanup_and_exit;

		} else if (!strncmp(input, "setoption name", 14)) {

			ptr = input + 15;
			if (!strncmp(ptr, "Hash", 4)) {
				ptr += 5;
				if (!strncmp(ptr, "value", 5))
					tt_alloc_MB(&tt, strtoul(ptr + 6, &end, 10));
			} else {
				Tunable* tunable = get_tunable(ptr);
				if (!tunable)
					continue;
				ptr += tunable->name_len + 1;
				if (!strncmp(ptr, "value", 5)) // Directly accepts S(mg, eg)
					tunable->set_val(strtoul(ptr + 6, &end, 10));
			}

		} else if (!strncmp(input, "go", 2)) {

			transition(&engine, WAITING);
			engine.game_over       = 0;
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
					ptr = end;

				} else if (!strncmp(ptr, "movetime", 8)) {

					ctlr.time_left  = strtoull(ptr + 9, &end, 10);
					ctlr.moves_left = 1;
					ptr = end;

				} else if (!strncmp(ptr, "infinite", 8)) {

					ctlr.time_dependent = 0;

				}
			}

			start_thinking(&engine);

		}
	}
cleanup_and_exit:
	pthread_cond_destroy(&engine.sleep_cv);
	pthread_mutex_destroy(&engine.mutex);
}

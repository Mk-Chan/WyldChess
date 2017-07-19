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

#include "defs.h"
#include "search.h"
#include "search_unit.h"
#include "syzygy/tbprobe.h"
#include "tt.h"
#include "options.h"

enum Result {
	NO_RESULT,
	DRAW,
	CHECKMATE
};

static int three_fold_repetition(struct Position* const pos)
{
	struct State* curr = pos->state;
	struct State* ptr  = curr - 2;
	struct State* end  = curr - curr->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	u32 repeats = 0;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr->pos_key)
			++repeats;
	return repeats >= 2;
}

static int check_stale_and_mate(struct Position* const pos)
{
	struct Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, &list);
	u32* move;
	for (move = list.moves; move != list.end; ++move) {
		if (!legal_move(pos, *move))
			continue;
		return NO_RESULT;
	}
	return pos->state->checkers_bb ? CHECKMATE : DRAW;
}

static int check_result(struct Position* const pos)
{
	if (pos->state->fifty_moves > 99) {
		fprintf(stdout, "1/2-1/2 {Fifty move rule}\n");
		return DRAW;
	}
	if (insufficient_material(pos)) {
		fprintf(stdout, "1/2-1/2 {Insufficient material}\n");
		return DRAW;
	}
	if (three_fold_repetition(pos)) {
		fprintf(stdout, "1/2-1/2 {Threefold repetition}\n");
		return DRAW;
	}
	int result = check_stale_and_mate(pos);
	switch (result) {
	case DRAW:
		fprintf(stdout, "1/2-1/2 {Stalemate}\n");
		break;
	case CHECKMATE:
		if (pos->stm == WHITE)
			fprintf(stdout, "0-1 {Black mates}\n");
		else
			fprintf(stdout, "1-0 {White mates}\n");
		break;
	}

	return result;
}

static inline void print_spin_option(struct SpinOption* option)
{
	fprintf(stdout, "feature option=\"%s -spin %d %d %d\"\n",
			option->name, option->curr_val, option->min_val, option->max_val);
}

static inline void print_options_xboard()
{
	fprintf(stdout, "feature done=0\n");
	fprintf(stdout, "feature ping=1\n");
	fprintf(stdout, "feature myname=\"%s\"\n", ENGINE_NAME);
	fprintf(stdout, "feature reuse=1\n");
	fprintf(stdout, "feature sigint=0\n");
	fprintf(stdout, "feature sigterm=0\n");
	fprintf(stdout, "feature setboard=1\n");
	fprintf(stdout, "feature colors=0\n");
	fprintf(stdout, "feature usermove=1\n");
	fprintf(stdout, "feature memory=1\n");
	fprintf(stdout, "feature time=1\n");
	fprintf(stdout, "feature smp=1\n");
	fprintf(stdout, "feature egt=syzygy\n");
	struct SpinOption* curr = spin_options;
	struct SpinOption* end  = spin_options + arr_len(spin_options);
	for (; curr != end; ++curr)
		print_spin_option(curr);
	fprintf(stdout, "feature done=1\n");
}

void* su_loop_xboard(void* args)
{
	char mstr[6];
	u32 move;
	struct SearchUnit* su   = (struct SearchUnit*) args;
	struct Position* pos    = &su->pos;
	struct Controller* ctlr = &controller;
	while (1) {
		switch (su->target_state) {
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
			su->target_state = WAITING;
			move_str(move, mstr);
			if (!legal_move(pos, move)) {
				fprintf(stdout, "Invalid move by engine: %s\n", mstr);
				pthread_exit(0);
			}
			do_move(pos, move);
			fprintf(stdout, "move %s\n", mstr);
			break;

		case ANALYZING:
			su->curr_state = ANALYZING;
			ctlr->search_start_time = curr_time();
			begin_search(su);
			su->target_state = WAITING;
			break;

		case QUITTING:
			su->curr_state = QUITTING;
			pthread_exit(0);
		}
	}
}

void xboard_loop()
{
	static int const max_len = 100;
	char  input[max_len];
	char* ptr;
	char* end;
	u32   move;

	struct SearchUnit* su = search_units;
	init_search_unit(su);
	su->protocol = XBOARD;
	struct Position* pos    = &su->pos;
	struct Controller* ctlr = &controller;
	su->game_over           = 0;
	su->side                = BLACK;
	ctlr->time_dependent    = 1;
	ctlr->depth             = MAX_PLY;
	ctlr->moves_per_session = 40;
	ctlr->moves_left        = ctlr->moves_per_session;
	ctlr->time_left         = 240000;
	ctlr->increment         = 0;

	pthread_t* search_thread = search_threads;
	pthread_create(search_thread, NULL, su_loop_xboard, (void*) su);
	pthread_detach(*search_thread);

	while (1) {
		fgets(input, max_len, stdin);
		input[strlen(input)-1] = '\0';
		if (!strncmp(input, "protover 2", 10)) {

			print_options_xboard();

		} else if (!strncmp(input, "print", 5)) {

			print_board(pos);

		} else if (!strncmp(input, "ping", 4)) {

			fprintf(stdout, "pong %d\n", atoi(input + 5));

		} else if (!strncmp(input, "memory", 6)) {

			tt_alloc_MB(&tt, strtoul(input + 7, &end, 10));

		} else if (!strncmp(input, "cores", 5)) {

			struct SpinOption* option = spin_options + THREADS;
			int value = strtoul(input + 6, &end, 10);
			if (   value <= option->max_val
			    && value >= option->min_val) {
				option->curr_val = value;
				if (option->handler)
					option->handler();
			}

		} else if (!strncmp(input, "egtpath", 7)) {

			ptr = input + 8;
			if (!strncmp(ptr, "syzygy", 6)) {
				ptr += 7;
				tb_init(ptr);
			}

		} else if (!strncmp(input, "new", 3)) {

			transition(su, WAITING);
			su->game_over = 0;
			init_search(&su->sl);
			tt_clear(&tt);
			init_pos(pos);
			set_pos(pos, INITIAL_POSITION);
			su->side                 = BLACK;
			ctlr->time_dependent    = 1;
			ctlr->depth             = MAX_PLY;
			ctlr->moves_per_session = 40;
			ctlr->moves_left        = ctlr->moves_per_session;
			ctlr->time_left         = 240000;
			ctlr->increment         = 0;

		} else if (!strncmp(input, "quit", 4)) {

			transition(su, WAITING);
			transition(su, QUITTING);
			goto cleanup_and_exit;

		} else if (!strncmp(input, "analyze", 7)) {

			transition(su, WAITING);
			ctlr->time_dependent = 0;
			ctlr->analyzing      = 1;
			su->side             = -1;
			transition(su, ANALYZING);

		} else if (!strncmp(input, "exit", 4)) {

			transition(su, WAITING);
			ctlr->analyzing = 0;

		} else if (!strncmp(input, "setboard", 8)) {

			transition(su, WAITING);
			init_pos(pos);
			set_pos(pos, input + 9);
			if (ctlr->moves_per_session) {
				ctlr->moves_left = ctlr->moves_per_session
					- ((pos->state->full_moves - 1) % ctlr->moves_per_session);
			}

		} else if (!strncmp(input, "time", 4)) {

			transition(su, WAITING);
			ctlr->time_dependent = 1;
			ctlr->time_left = 10 * atoi(input + 5);

		} else if (!strncmp(input, "level", 5)) {

			transition(su, WAITING);
			ctlr->time_dependent = 1;
			ptr = input + 6;
			ctlr->moves_per_session = strtol(ptr, &end, 10);
			ctlr->moves_left = ctlr->moves_per_session;
			if (!ctlr->moves_left)
				ctlr->moves_left = 40;
			ptr = end;
			ctlr->time_left  = 60000 * strtol(ptr, &end, 10);
			ptr = end;
			if (*ptr == ':') {
				++ptr;
				ctlr->time_left += 1000 * strtol(ptr, &end, 10);
				ptr = end;
			}
			ctlr->increment  = 1000 * strtod(ptr, &end);

		} else if (!strncmp(input, "perft", 5)) {

			transition(su, WAITING);
			performance_test(pos, atoi(input + 6));

		} else if (!strncmp(input, "st", 2)) {

			// Seconds per move
			transition(su, WAITING);
			ctlr->time_dependent    = 1;
			ctlr->time_left         = 1000 * atoi(input + 3);
			ctlr->moves_per_session = 1;
			ctlr->moves_left        = 1;
			ctlr->increment         = 0;

		} else if (!strncmp(input, "sd", 2)) {

			transition(su, WAITING);
			ctlr->depth = atoi(input + 3);

		} else if (!strncmp(input, "force", 5)) {

			transition(su, WAITING);
			su->side = -1;

		} else if (!strncmp(input, "result", 6)) {

			transition(su, WAITING);
			su->game_over = 1;

		} else if (!strncmp(input, "?", 1)) {

			transition(su, WAITING);

		} else if (!strncmp(input, "go", 2)) {

			transition(su, WAITING);
			su->side = pos->stm;
			if (ctlr->moves_per_session) {
				ctlr->moves_left = ctlr->moves_per_session
					- ((pos->state->full_moves - 1) % ctlr->moves_per_session);
			}

			if (su->game_over)
				check_result(pos);
			else if (check_result(pos) != NO_RESULT)
				su->game_over = 1;
			else
				start_thinking(su);

		} else if (!strncmp(input, "eval", 4)) {

			transition(su, WAITING);
			fprintf(stdout, "evaluation = %d\n", evaluate(pos));
			fprintf(stdout, "phase = %d\n", pos->phase);

		} else if (!strncmp(input, "undo", 4)) {

			transition(su, WAITING);
			if (pos->state > pos->hist)
				undo_move(pos);
			su->side = -1;
			if (ctlr->analyzing)
				transition(su, ANALYZING);

		} else if (!strncmp(input, "option", 6)) {

			ptr = input + 7;
			struct SpinOption* curr = spin_options;
			struct SpinOption* option_end = spin_options + arr_len(spin_options);
			for (; curr != option_end; ++curr) {
				int len = strlen(curr->name);
				if (!strncmp(ptr, curr->name, len)) {
					ptr += len;
					if (*ptr == '=') {
						int value = strtol(ptr + 1, &end, 10);
						if (   value <= curr->max_val
						    && value >= curr->min_val) {
							curr->curr_val = value;
							if (curr->handler)
								curr->handler();
						}
					}
				}
			}

		} else {

			if (!strncmp(input, "usermove", 8)) {
				ptr = input + 9;
			} else if (input[1] < '9' && input[1] > '0') {
				ptr = input;
			} else {
				fprintf(stdout, "ERROR (Unsupported): %s\n", input);
				continue;
			}

			transition(su, WAITING);
			move = parse_move(pos, ptr);
			if (  !move
			   || !legal_move(pos, move))
				fprintf(stdout, "Illegal move: %s\n", ptr);
			else
				do_move(pos, move);

			if (su->game_over)
				check_result(pos);
			else if (check_result(pos) != NO_RESULT)
				su->game_over = 1;
			else if (su->side == pos->stm)
				start_thinking(su);
			else if (ctlr->analyzing)
				transition(su, ANALYZING);

		}
	}
cleanup_and_exit:
	pthread_cond_destroy(&su->sleep_cv);
	pthread_mutex_destroy(&su->mutex);
}

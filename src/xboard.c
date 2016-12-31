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

enum Result {
	NO_RESULT,
	DRAW,
	CHECKMATE
};

static int three_fold_repetition(Position* const pos)
{
	State* curr = pos->state;
	State* ptr  = curr - 2;
	State* end  = curr - curr->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	u32 repeats = 0;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr->pos_key)
			++repeats;
	return repeats >= 2;
}

static int check_stale_and_mate(Position* const pos)
{
	static Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, &list);
	for (Move* move = list.moves; move != list.end; ++move) {
		if (!legal_move(pos, *move))
			continue;
		return NO_RESULT;
	}
	return pos->state->checkers_bb ? CHECKMATE : DRAW;
}

static int check_result(Position* const pos)
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
	fprintf(stdout, "feature done=1\n");
}

void* engine_loop_xboard(void* args)
{
	char mstr[6];
	Move move;
	Engine* engine = (Engine*) args;
	Position* pos  = engine->pos;
	while (1) {
		switch (engine->target_state) {
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
			if (!legal_move(pos, move)) {
				fprintf(stdout, "Invalid move by engine: %s\n", mstr);
				pthread_exit(0);
			}
			do_move(pos, move);
			fprintf(stdout, "move %s\n", mstr);
			if (!engine->ctlr->tpm) {
				engine->ctlr->time_left -= curr_time() - engine->ctlr->search_start_time;
				engine->ctlr->time_left += engine->ctlr->increment;
			}
			engine->target_state     = WAITING;
			break;

		case ANALYZING:
			engine->curr_state = ANALYZING;
			engine->ctlr->search_start_time = curr_time();
			begin_search(engine);
			engine->target_state = WAITING;
			break;

		case QUITTING:
			engine->curr_state = QUITTING;
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
	Move move;

	Position pos;
	Controller ctlr;
	ctlr.depth = MAX_PLY;
	Engine engine;
	engine.protocol = XBOARD;
	pthread_mutex_init(&engine.mutex, NULL);
	pthread_cond_init(&engine.sleep_cv, NULL);
	engine.pos  = &pos;
	engine.ctlr = &ctlr;
	engine.side = BLACK;
	ctlr.time_dependent = 1;
	ctlr.analyzing = 0;
	engine.target_state = WAITING;
	init_pos(&pos);
	set_pos(&pos, INITIAL_POSITION);

	pthread_t engine_thread;
	pthread_create(&engine_thread, NULL, engine_loop_xboard, (void*) &engine);
	pthread_detach(engine_thread);

	while (1) {
		fgets(input, max_len, stdin);
		if (!strncmp(input, "protover 2", 10)) {

			print_options_xboard();

		} else if (!strncmp(input, "print", 5)) {

			print_board(&pos);

		} else if (!strncmp(input, "ping", 4)) {

			fprintf(stdout, "pong %d\n", atoi(input + 5));

		} else if (!strncmp(input, "new", 3)) {

			transition(&engine, WAITING);
			engine.game_over = 0;
			init_pos(&pos);
			set_pos(&pos, INITIAL_POSITION);
			engine.side = BLACK;
			ctlr.time_dependent = 1;
			ctlr.depth  = MAX_PLY;
			ctlr.tpm = 0;
			ctlr.moves_per_session = 40;
			ctlr.moves_left = ctlr.moves_per_session;
			ctlr.time_left = 240000;
			ctlr.increment = 0;

		} else if (!strncmp(input, "quit", 4)) {

			transition(&engine, QUITTING);
			goto cleanup_and_exit;

		} else if (!strncmp(input, "analyze", 7)) {

			transition(&engine, WAITING);
			ctlr.time_dependent = 0;
			ctlr.tpm            = 0;
			ctlr.analyzing      = 1;
			engine.side         = -1;
			transition(&engine, ANALYZING);

		} else if (!strncmp(input, "exit", 4)) {

			transition(&engine, WAITING);
			ctlr.analyzing = 0;

		} else if (!strncmp(input, "setboard", 8)) {

			transition(&engine, WAITING);
			engine.side = -1;
			set_pos(&pos, input + 9);
			if (ctlr.moves_per_session) {
				ctlr.moves_left = ctlr.moves_per_session
					- ((pos.state->full_moves - 1) % ctlr.moves_per_session);
			}
			engine.side = pos.stm == WHITE ? BLACK : WHITE;

		} else if (!strncmp(input, "time", 4)) {

			ctlr.time_dependent = 1;
			ctlr.tpm = 0;
			ctlr.time_left = 10 * atoi(input + 5);

		} else if (!strncmp(input, "level", 5)) {

			ctlr.time_dependent = 1;
			ctlr.tpm = 0;
			ptr = input + 6;
			ctlr.moves_per_session = strtol(ptr, &end, 10);
			ctlr.moves_left = ctlr.moves_per_session;
			if (!ctlr.moves_left)
				ctlr.moves_left = 40;
			ptr = end;
			ctlr.time_left  = 60000 * strtol(ptr, &end, 10);
			ptr = end;
			if (*ptr == ':') {
				++ptr;
				ctlr.time_left += 1000 * strtol(ptr, &end, 10);
				ptr = end;
			}
			ctlr.increment  = 1000 * strtod(ptr, &end);

		} else if (!strncmp(input, "perft", 5)) {

			transition(&engine, WAITING);
			performance_test(&pos, atoi(input + 6));

		} else if (!strncmp(input, "st", 2)) {

			// Seconds per move
			ctlr.time_dependent    = 1;
			ctlr.tpm               = 1;
			ctlr.time_left         = 1000 * atoi(input + 3);
			ctlr.moves_per_session = 1;
			ctlr.moves_left        = 1;
			ctlr.increment         = 0;

		} else if (!strncmp(input, "sd", 2)) {

			ctlr.depth = atoi(input + 3);

		} else if (!strncmp(input, "force", 5)) {

			transition(&engine, WAITING);
			engine.side = -1;

		} else if (!strncmp(input, "result", 6)) {

			transition(&engine, WAITING);
			engine.game_over = 1;

		} else if (!strncmp(input, "?", 1)) {

			transition(&engine, WAITING);

		} else if (!strncmp(input, "go", 2)) {

			transition(&engine, WAITING);
			engine.side = pos.stm;
			if (ctlr.moves_per_session) {
				ctlr.moves_left = ctlr.moves_per_session
					- ((pos.state->full_moves - 1) % ctlr.moves_per_session);
			}

			if (engine.game_over)
				check_result(&pos);
			else if (check_result(&pos) != NO_RESULT)
				engine.game_over = 1;
			else
				start_thinking(&engine);

		} else if (!strncmp(input, "eval", 4)) {

#ifdef STATS
			transition(&engine, WAITING);
			fprintf(stdout, "evaluation = %d\n", evaluate(&pos));
			fprintf(stdout, "phase = %d\n", pos.state->phase);

			static char* c_str[2]  = { "WHITE", "BLACK" };
			static char* pt_str[7] = { "\0", "\0", "PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN" };
			int c, pt;
			fprintf(stdout, "Split eval:\n");
			for (c = WHITE; c <= BLACK; ++c)
				fprintf(stdout, "%7s                        ", c_str[c]);
			fprintf(stdout, "\n");
			for (pt = PAWN; pt != KING; ++pt) {
				for (c = WHITE; c <= BLACK; ++c)
					fprintf(stdout, "%7s: %4d                  ", pt_str[pt], es.pt_score[c][pt]);
				fprintf(stdout, "\n");
			}
			for (c = WHITE; c <= BLACK; ++c)
				fprintf(stdout, "PASSERS: %4d                  ", es.passed_pawn[c]);
			fprintf(stdout, "\n");
			for (c = WHITE; c <= BLACK; ++c)
				fprintf(stdout, " K_ATKS: %4d                  ", es.king_atks[c]);
			fprintf(stdout, "\n");
			for (c = WHITE; c <= BLACK; ++c)
				fprintf(stdout, "  P_PSQ: %4d                  ", es.piece_psq_eval[c]);
			fprintf(stdout, "\n");
#endif

		} else if (!strncmp(input, "undo", 4)) {

			// Does not recover time
			transition(&engine, WAITING);
			if (pos.state > pos.hist)
				undo_move(&pos);
			engine.side = -1;
			if (ctlr.analyzing)
				transition(&engine, ANALYZING);

		} else {

			if (!strncmp(input, "usermove", 8)) {
				ptr = input + 9;
			} else if (input[1] < '9' && input[1] > '0') {
				ptr = input;
			} else {
				fprintf(stdout, "ERROR (Unsupported): %s\n", input);
				continue;
			}

			transition(&engine, WAITING);
			move = parse_move(&pos, ptr);
			if (  !move
			   || !legal_move(&pos, move))
				fprintf(stdout, "Illegal move: %s\n", ptr);
			else
				do_move(&pos, move);

			if (engine.game_over)
				check_result(&pos);
			else if (check_result(&pos) != NO_RESULT)
				engine.game_over = 1;
			else if (engine.side == pos.stm)
				start_thinking(&engine);
			else if (ctlr.analyzing)
				transition(&engine, ANALYZING);

		}
	}
cleanup_and_exit:
	pthread_cond_destroy(&engine.sleep_cv);
	pthread_mutex_destroy(&engine.mutex);
}

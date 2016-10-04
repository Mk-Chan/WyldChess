#include <pthread.h>
#include "defs.h"
#include "engine.h"
#include "timer.h"

char* fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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

static int insufficient_material(Position* const pos)
{
	u64 const * const bb = pos->bb;
	if (bb[PAWN] || bb[QUEEN] || bb[ROOK])
		return NO_RESULT;
	if (   popcnt(bb[BISHOP] & bb[WHITE]) > 2
	    || popcnt(bb[BISHOP] & bb[BLACK]) > 2)
		return NO_RESULT;
	if (   popcnt(bb[KNIGHT] & bb[WHITE]) > 2
	    || popcnt(bb[KNIGHT] & bb[BLACK]) > 2)
		return NO_RESULT;
	if (   popcnt((bb[BISHOP] ^ bb[KNIGHT]) & bb[WHITE]) > 1
	    || popcnt((bb[BISHOP] ^ bb[KNIGHT]) & bb[BLACK]) > 1)
		return NO_RESULT;
	return 1;
}

static int check_stale_and_mate(Position* const pos)
{
	static Movelist list;
	list.end = list.moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, &list);
	} else {
		gen_quiets(pos, &list);
		gen_captures(pos, &list);
	}
	for (Move* move = list.moves; move != list.end; ++move) {
		if (!do_move(pos, *move))
			continue;
		undo_move(pos);
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

void* engine_loop(void* args)
{
	static char mstr[6];
	Move move;
	Engine* engine = (Engine*) args;
	Position* pos  = engine->pos;
	while (1) {
		switch (engine->target_state) {
		case WAITING:
			engine->curr_state = WAITING;
			break;

		case THINKING:
			engine->curr_state = THINKING;
			move = begin_search(engine);
			move_str(move, mstr);
			if (!do_move(pos, move)) {
				fprintf(stdout, "Invalid move by engine: %s\n", mstr);
				pthread_exit(0);
			}
			fprintf(stdout, "move %s\n", mstr);
			engine->ctlr->time_left -= curr_time() - engine->ctlr->search_start_time;
			engine->ctlr->time_left += engine->ctlr->increment;
			engine->target_state     = WAITING;
			break;

		case GAME_OVER:
			engine->curr_state = GAME_OVER;
			break;

		case QUITTING:
			engine->curr_state = QUITTING;
			pthread_exit(0);
		}
	}
}

static inline void print_options()
{
	fprintf(stdout, "feature done=0\n");
	fprintf(stdout, "feature ping=1\n");
	fprintf(stdout, "feature myname=\"WyldChess\"\n");
	fprintf(stdout, "feature reuse=1\n");
	fprintf(stdout, "feature sigint=0\n");
	fprintf(stdout, "feature sigterm=0\n");
	fprintf(stdout, "feature setboard=1\n");
	fprintf(stdout, "feature analyze=0\n");
	fprintf(stdout, "feature done=1\n");
}

static inline int synced(Engine const * const engine)
{
	return engine->curr_state == engine->target_state;
}

static inline void sync(Engine const * const engine)
{
	while (!synced(engine))
		continue;
}

static inline void transition(Engine* const engine, int target_state)
{
	engine->target_state = target_state;
	sync(engine);
}

static inline void start_thinking(Engine* const engine)
{
	if (engine->curr_state == GAME_OVER)
		return;
	transition(engine, WAITING);
	if (check_result(engine->pos) != NO_RESULT) {
		transition(engine, GAME_OVER);
		return;
	}

	if (engine->side == engine->pos->stm) {
		Controller* const ctlr  = engine->ctlr;
		ctlr->search_start_time = curr_time();
		ctlr->search_end_time   =  ctlr->search_start_time
			                + (ctlr->time_left / ctlr->moves_left);
		fprintf(stdout, "time left=%llu moves left=%u time allotted=%llu\n",
			ctlr->time_left, ctlr->moves_left, ctlr->search_end_time - ctlr->search_start_time);
		transition(engine, THINKING);
		--ctlr->moves_left;
		if (ctlr->moves_left < 1)
			ctlr->moves_left = ctlr->moves_per_session;
	}
}

void cecp_loop()
{
	static int const max_len = 100;
	char  input[max_len];
	char* ptr;
	char* end;
	fgets(input, max_len, stdin);
	if (strncmp(input, "xboard", 6)) {
		fprintf(stdout, "Protocol not found!\n");
		return;
	}

	Move move;
	Position pos;
	Controller ctlr;
	ctlr.depth = MAX_PLY;
	Engine engine;
	engine.pos   = &pos;
	engine.ctlr  = &ctlr;
	engine.target_state = WAITING;
	engine.side  = BLACK;
	init_pos(engine.pos);
	set_pos(engine.pos, fen1);
	pthread_t engine_thread;
	pthread_create(&engine_thread, NULL, engine_loop, (void*) &engine);
	pthread_detach(engine_thread);

	// Controller parameters are still left
	while (1) {
		fgets(input, max_len, stdin);
		if (!strncmp(input, "protover 2", 10)) {

			print_options();

		} else if (!strncmp(input, "print", 5)) {

			print_board(engine.pos);

		} else if (!strncmp(input, "ping", 4)) {

			fprintf(stdout, "pong %d\n", atoi(input + 5));

		} else if (!strncmp(input, "new", 3)) {

			transition(&engine, WAITING);
			init_pos(engine.pos);
			set_pos(engine.pos, fen1);
			engine.side = BLACK;
			ctlr.depth  = MAX_PLY;

		} else if (!strncmp(input, "quit", 4)) {

			transition(&engine, QUITTING);
			return;

		} else if (!strncmp(input, "exit", 4)) {

			transition(&engine, WAITING);
			engine.side = -1;

		} else if (!strncmp(input, "setboard", 8)) {

			transition(&engine, WAITING);
			engine.side = -1;
			set_pos(engine.pos, input + 9);
			ctlr.moves_left = ctlr.moves_per_session
				- ((pos.state->full_moves - 1) % ctlr.moves_per_session);
			engine.side = engine.pos->stm == WHITE ? BLACK : WHITE;

		} else if (!strncmp(input, "time", 4)) {

			ctlr.time_left = 10 * atoi(input + 5);

		} else if (!strncmp(input, "level", 5)) {

			ptr = input + 6;
			ctlr.moves_per_session = strtol(ptr, &end, 10);
			ctlr.moves_left = ctlr.moves_per_session;
			ptr = end;
			ctlr.time_left  = 60000 * strtol(ptr, &end, 10);
			ptr = end;
			if (*ptr == ':') {
				++ptr;
				ctlr.time_left += 1000 * strtol(ptr, &end, 10);
				ptr = end;
			}
			ctlr.increment  = 1000 * strtod(ptr, &end);
			fprintf(stdout, "moves=%d timeleft=%llu inc=%llu\n",
				ctlr.moves_left, ctlr.time_left, ctlr.increment);

		} else if (!strncmp(input, "perft", 5)) {

			transition(&engine, WAITING);
			performance_test(&pos, atoi(input + 6));

		} else if (!strncmp(input, "st", 2)) {

			// Seconds per move
			ctlr.time_left  = 1000 * atoi(input + 3);
			ctlr.moves_left = 1;
			ctlr.increment  = 0;

		} else if (!strncmp(input, "sd", 2)) {

			ctlr.depth = atoi(input + 3);

		} else if (!strncmp(input, "force", 5)) {

			transition(&engine, WAITING);
			engine.side = -1;

		} else if (!strncmp(input, "result", 6)) {

			// Use result for learning later
			transition(&engine, GAME_OVER);

		} else if (!strncmp(input, "?", 1)) {

			transition(&engine, WAITING);

		} else if (!strncmp(input, "go", 2)) {

			engine.side = engine.pos->stm;
			ctlr.moves_left = ctlr.moves_per_session
				- ((pos.state->full_moves - 1) % ctlr.moves_per_session);
			start_thinking(&engine);

		} else if (!strncmp(input, "eval", 4)) {

			transition(&engine, WAITING);
			fprintf(stdout, "evaluation = %d\n", evaluate(&pos));
			fprintf(stdout, "phase = %d\n", pos.state->phase);

		} else if (!strncmp(input, "undo", 4)) {

			// Does not recover time at the moment
			transition(&engine, WAITING);
			if (pos.state > pos.hist)
				undo_move(&pos);
			engine.side = -1;

		} else {

			move = parse_move(engine.pos, input);
			if (  !move
			   || !do_move(engine.pos, move))
				fprintf(stdout, "Illegal move: %s\n", input);
			start_thinking(&engine);
		}
	}
}

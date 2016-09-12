#include <pthread.h>
#include "defs.h"
#include "engine.h"

char* fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum Result {
	NO_RESULT,
	DRAW,
	CHECKMATE
};

static void print_features()
{
	write(1, "feature done=0\n", 15);
	write(1, "feature myname=\"Wyld\"\n", 22);
	write(1, "feature reuse=1\n", 16);
	write(1, "feature ping=1\n", 15);
	write(1, "feature setboard=1\n", 19);
	write(1, "feature done=1\n", 15);
}

static int three_fold_repetition(Position const * const pos)
{
	State* curr = pos->state;
	State* ptr  = curr - curr->fifty_moves;
	u32 repeats = 0;
	for (; ptr != curr; ++curr)
		if (ptr->pos_key == curr->pos_key)
			++repeats;
	return repeats >= 2 ? DRAW : NO_RESULT;
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
	Movelist* list = pos->list;
	list->end      = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	} else {
		gen_quiets(pos, list);
		gen_captures(pos, list);
	}
	for (u32* move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, move))
			continue;
		undo_move(pos, move);
		return NO_RESULT;
	}
	return pos->state->checkers_bb ? CHECKMATE : DRAW;
}

static int check_status(Position* const pos)
{
	if (pos->ply > 99) {
		write(1, "1/2-1/2 Fifty move rule (Claimed by Wyld)\n", 44);
		return DRAW;
	}
	if (insufficient_material(pos)) {
		write(1, "1/2-1/2 Insufficient material (Claimed by Wyld)\n", 48);
		return DRAW;
	}
	if (three_fold_repetition(pos)) {
		write(1, "1/2-1/2 Threefold repetition (Claimed by Wyld)\n", 47);
		return DRAW;
	}
	int result = check_stale_and_mate(pos);
	switch (result) {
	case DRAW:
		write(1, "1/2-1/2 Stalemate (Claimed by Wyld)\n", 36);
		break;
	case CHECKMATE:
		if (pos->stm == WHITE)
			write(1, "0-1 Black wins (Claimed by Wyld)\n", 33);
		else
			write(1, "1-0 White wins (Claimed by Wyld)\n", 33);
		break;
	}

	return result;
}

void* engine_loop(void* args)
{
	u32 move;
	Engine* engine = (Engine*) args;
	Position* pos  = engine->pos;
	while (1) {
		switch (engine->state) {
		case WAITING:
			if (engine->side == pos->stm)
				engine->state = THINKING;
			break;

		case THINKING:
			move = begin_search(engine);
			if (!do_move(pos, &move)) {
				write(1, "Invalid move returned: ", 23);
				write(1, move_str(move), 6);
				write(1, "\n", 1);
				engine->state = QUITTING;
				return 0;
			}
			break;

		case ANALYZING:
			begin_search(engine);
			break;

		case QUITTING:
			return 0;
		}
	}
}

void cecp_loop()
{
	static const int max_len = 50;
	char input[max_len];
	read(0, input, max_len);
	if (strncmp(input, "xboard", 6)) {
		write(1, "Protocol not found!\n", 20);
		return;
	}

	u32 move;
	Position pos;
	Controller ctlr;
	Engine engine;
	engine.pos   = &pos;
	engine.ctlr  = &ctlr;
	engine.state = WAITING;
	engine.side  = BLACK;
	pthread_t engine_thread;
	pthread_create(&engine_thread, NULL, engine_loop, (void*) &engine);

	// Controller parameters are still left
	while (1) {
		read(0, input, max_len);
		if (!strncmp(input, "protover 2", 10)) {

			print_features();

		} else if (!strncmp(input, "ping", 4)) {

			input[1] = 'o';
			write(1, input, 5);
			char* ptr = input + 5;
			while (*ptr <= '9' && *ptr >= '0') {
				write(1, ptr, 1);
				++ptr;
			}
			write(1, "\n", 1);

		} else if (!strncmp(input, "print", 5)) {

			print_board(engine.pos);

		} else if (!strncmp(input, "quit", 4)) {

			engine.state = QUITTING;
			return;

		} else if (!strncmp(input, "new", 3)) {

			engine.side = BLACK;
			init_pos(engine.pos);
			set_pos(engine.pos, fen1);

		} else if (!strncmp(input, "setboard",8 )) {

			engine.side = -1;
			set_pos(engine.pos, input + 9);

		} else if (!strncmp(input, "force", 5)) {

			engine.side = -1;

		} else if (!strncmp(input, "go", 2)) {

			engine.side = engine.pos->stm;

		} else if (!strncmp(input, "usermove", 8)) {

			move = parse_move(engine.pos, input + 9);
			if (  !move
			   || !do_move(engine.pos, &move)) {
				write(1, "Illegal usermove!\n", 18);
				engine.state = QUITTING;
				return;
			}

		} else if (!strncmp(input, "stop", 4)) {

			engine.state = WAITING;

		}
	}
}

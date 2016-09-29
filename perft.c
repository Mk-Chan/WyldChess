#include <stdio.h>
#include "defs.h"
#include "position.h"
#include "timer.h"

static u64 perft(Position* const pos, Movelist* list, u32 depth)
{
	list->end = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	}
	else {
		gen_quiets(pos, list);
		gen_captures(pos, list);
	}

	u64 count = 0ULL;
	Move* move;
	if (depth == 1) {
		for(move = list->moves; move < list->end; ++move) {
			if(!do_move(pos, *move)) continue;
			undo_move(pos);
			++count;
		}
	} else {
		for(move = list->moves; move < list->end; ++move) {
			if(!do_move(pos, *move)) continue;
			count += perft(pos, list + 1, depth - 1);
			undo_move(pos);
		}
	}


	return count;
}

void performance_test(Position* const pos, u32 max_depth)
{
	Movelist list[MAX_PLY];
	u32 depth;
	u64 count;
	u64 t1, t2;
	for (depth = 1; depth <= max_depth; ++depth) {
		t1 = curr_time();
		count = perft(pos, list, depth);
		t2 = curr_time();
		fprintf(stdout, "Perft(%2d) = %20llu (%10llu ms)\n", depth, count, (t2 - t1));
	}
}

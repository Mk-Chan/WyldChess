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

#include <stdio.h>
#include "defs.h"
#include "position.h"
#include "misc.h"

static u64 perft(Position* const pos, Movelist* list, u32 depth)
{
	list->end = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	gen_pseudo_legal_moves(pos, list);

	u64 count = 0ULL;
	u32* move;
	if (depth == 1) {
		for (move = list->moves; move < list->end; ++move) {
			if (!legal_move(pos, *move)) continue;
			do_move(pos, *move);
			undo_move(pos);
			++count;
		}
	} else {
		for (move = list->moves; move < list->end; ++move) {
			if (!legal_move(pos, *move)) continue;
			do_move(pos, *move);
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
		fprintf(stdout, "info depth %u time %llu nodes %llu\n", depth, (t2 - t1), count);
	}
	fprintf(stdout, "done\n");
}

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

#include <time.h>
#include "search_unit.h"
#include "magicmoves.h"
#include "position.h"
#include "search.h"
#include "misc.h"
#include "tt.h"
#include "pt.h"

int protocol;
struct TT tt;
struct PT pt;
struct Controller controller;

void init_parallel_resources()
{
	pthread_mutex_init(&split_mutex, NULL);
	struct SplitPoint* curr;
	for (int i = 0; i < MAX_SPLIT_POINTS; ++i) {
		curr = split_points + i;
		curr->in_use = 0;
		curr->joinable = 0;
		curr->finished = 1;
		pthread_mutex_init(&curr->mutex, NULL);
		if (i)
			search_units[i].type = HELPER;
		else
			search_units[i].type = MAIN;
	}
	for (int i = 0; i < MAX_THREADS; ++i) {
		for (int j = 0; j < MAX_PLY; ++j) {
			search_stacks[i][j].ply = j;
		}
	}
}

int main()
{
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	init_timer();
	init_genrand64(234702970592742ULL);
	init_zobrist_keys();
	initmagicmoves();
	init_lookups();
	init_eval_terms();
	init_parallel_resources();
	tt_alloc_MB(&tt, 128);
	pt_alloc_MB(&pt, 8);

	struct SpinOption* curr = spin_options;
	struct SpinOption* end  = spin_options + NUM_OPTIONS;
	for (; curr != end; ++curr) {
		if (curr->handler)
			curr->handler();
	}

	char input[100];
	while (1) {
		fgets(input, 100, stdin);
		if (!strncmp(input, "xboard", 6)) {
			protocol = XBOARD;
			xboard_loop();
			break;
		} else if (!strncmp(input, "uci", 3)) {
			protocol = UCI;
			uci_loop();
			break;
		} else if (!strncmp(input, "quit", 4)) {
			break;
		} else {
			fprintf(stdout, "Protocol not found!\n");
		}
	}

	pt_destroy(&pt);
	tt_destroy(&tt);

	return 0;
}

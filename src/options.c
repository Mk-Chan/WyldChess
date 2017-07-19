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
#include "options.h"
#include "search_unit.h"

struct SpinOption spin_options[NUM_OPTIONS] = {
	{ "MoveOverhead", 30, 1, 5000, NULL },
	{ "Threads", 1, 1, 64, &handle_threads }
};

pthread_t* search_threads;
struct SearchUnit* search_units;
struct SearchStack (*search_stacks)[MAX_PLY];
struct SearchParams* search_params;

void handle_threads()
{
	int num        = spin_options[THREADS].curr_val - 1;
	search_threads = realloc(search_threads, sizeof(pthread_t) * num);
	search_units   = realloc(search_units, sizeof(struct SearchUnit) * num);
	search_params  = realloc(search_params, sizeof(struct SearchParams) * num);
	search_stacks  = realloc(search_stacks, sizeof(struct SearchStack[MAX_PLY]) * num);
}

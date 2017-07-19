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

pthread_t search_threads[MAX_THREADS];
struct SearchUnit search_units[MAX_THREADS];
struct SearchStack search_stacks[MAX_THREADS][MAX_PLY];
struct SearchParams search_params[MAX_THREADS];

struct SpinOption spin_options[NUM_OPTIONS] = {
	{ "MoveOverhead", 30, 1, 5000, NULL },
	{ "Threads", 1, 1, MAX_THREADS, NULL }
};

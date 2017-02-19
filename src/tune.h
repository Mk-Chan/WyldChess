#ifndef TUNE_H
#define TUNE_H

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

#include <stdarg.h>

#include "defs.h"

typedef void (*fxn_ptr) (int, ...);

typedef struct Tunable_s {

	char*   name;
	int     name_len;
	fxn_ptr set_val;

} Tunable;

#define T(name) { #name, sizeof(#name) / (sizeof(char)) - 1, &set_##name }

#define TUNABLE(term) extern int term;                                                             \
	int term;                                                                                  \
	static inline void set_##term(int val, ...) { term = val; }
#define TUNABLE_ARR(term, size) extern int term[size];                                             \
	int term[size];                                                                            \
	static inline void set_##term(int val, ...) {                                              \
		va_list list;                                                                      \
		va_start(list, val);                                                               \
		term[va_arg(list, int)] = val;                                                     \
	}

/*
**************************************************
Making variables visible to the tuning framework *
**************************************************
                                                 *
1. Declare variables to be tunable as follows:   *
TUNABLE(x)                                       *
TUNABLE_ARR(y, 2)                                *
                                                 *
2. Append them to the tunables[] array:          *
Tunable tunables[] = {                           *
       ...                                       *
       T(x),                                     *
       T(y)                                      *
};                                               *
                                                 *
**************************************************
*/

TUNABLE(doubled_pawns)
TUNABLE(isolated_pawn)
TUNABLE(bishop_pair)
TUNABLE(rook_on_7th)
TUNABLE(rook_open_file)
TUNABLE(rook_semi_open)

TUNABLE_ARR(king_atk_wt, 7)
TUNABLE_ARR(king_open_file, 2)
TUNABLE_ARR(passed_pawn, 8)
TUNABLE_ARR(mobility, 7)
TUNABLE_ARR(min_mob_count, 7)
TUNABLE_ARR(outpost, 2)

static Tunable tunables[] = {
	T(doubled_pawns),
	T(isolated_pawn),
	T(bishop_pair),
	T(rook_on_7th),
	T(rook_open_file),
	T(rook_semi_open),

	T(king_atk_wt),
	T(king_open_file),
	T(passed_pawn),
	T(mobility),
	T(min_mob_count),
	T(outpost)
};
static Tunable* tunables_end = tunables + (sizeof(tunables) / sizeof(tunables[0]));

static inline Tunable* get_tunable(char* name) {
	Tunable* curr = tunables;
	for (; curr != tunables_end; ++curr)
		if (!strncmp(name, curr->name, curr->name_len))
			return curr;
	return NULL;
}

#endif

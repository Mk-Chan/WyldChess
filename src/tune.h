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

	double  score;
	char*   name;
	int     name_len;
	int     size;
	int   (*get_val)(int);
	fxn_ptr inc_val;
	fxn_ptr set_val;

} Tunable;

static inline int tunable_comparator(void const * a, void const * b)
{
	return ((Tunable*)b)->score - ((Tunable*)a)->score;
}

#define T(name) { 0.0, #name"_mg", sizeof(#name"_mg") / (sizeof(char)) - 1,                        \
	sizeof(name) / sizeof(int), &get_##name##_mg, &inc_##name##_mg, &set_##name##_mg },        \
                { 0.0, #name"_eg", sizeof(#name"_eg") / (sizeof(char)) - 1,                        \
	sizeof(name) / sizeof(int), &get_##name##_eg, &inc_##name##_eg, &set_##name##_eg }

#define TUNABLE(name) extern int name;                                                             \
	int name;                                                                                  \
	static inline int get_##name##_mg(int dummy) {                                             \
		return mg_val(name);                                                               \
	}                                                                                          \
	static inline void set_##name##_mg(int val, ...) {                                         \
		name = S(val, eg_val(name));                                                       \
	}                                                                                          \
	static inline int get_##name##_eg(int dummy) {                                             \
		return eg_val(name);                                                               \
	}                                                                                          \
	static inline void set_##name##_eg(int val, ...) {                                         \
		name = S(mg_val(name), val);                                                       \
	}                                                                                          \
	static inline void inc_##name##_mg(int val, ...) {                                         \
		name = S(mg_val(name) + val, eg_val(name));                                        \
	}                                                                                          \
	static inline void inc_##name##_eg(int val, ...) {                                         \
		name = S(mg_val(name), eg_val(name) + val);                                        \
	}

#define TUNABLE_ARR(name, size) extern int name[size];                                             \
	int name[size];                                                                            \
	static inline int get_##name##_mg(int index) {                                             \
		return mg_val(name[index]);                                                        \
	}                                                                                          \
	static inline void set_##name##_mg(int val, ...) {                                         \
		va_list list;                                                                      \
		va_start(list, val);                                                               \
		int i = va_arg(list, int);                                                         \
		name[i] = S(val, eg_val(name[i]));                                                 \
	}                                                                                          \
	static inline int get_##name##_eg(int index) {                                             \
		return eg_val(name[index]);                                                        \
	}                                                                                          \
	static inline void set_##name##_eg(int val, ...) {                                         \
		va_list list;                                                                      \
		va_start(list, val);                                                               \
		int i = va_arg(list, int);                                                         \
		name[i] = S(mg_val(name[i]), val);                                                 \
	}                                                                                          \
	static inline void inc_##name##_mg(int val, ...) {                                         \
		va_list list;                                                                      \
		va_start(list, val);                                                               \
		int i = va_arg(list, int);                                                         \
		name[i] = S(mg_val(name[i]) + val, eg_val(name[i]));                               \
	}                                                                                          \
	static inline void inc_##name##_eg(int val, ...) {                                         \
		va_list list;                                                                      \
		va_start(list, val);                                                               \
		int i = va_arg(list, int);                                                         \
		name[i] = S(mg_val(name[i]), eg_val(name[i]) + val);                               \
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

TUNABLE_ARR(piece_val, 8)
TUNABLE_ARR(king_atk_wt, 7)
TUNABLE_ARR(king_open_file, 2)
TUNABLE_ARR(passed_pawn, 8)
TUNABLE_ARR(outpost, 2)
TUNABLE_ARR(mobility_knight, 9)
TUNABLE_ARR(mobility_bishop, 14)
TUNABLE_ARR(mobility_rook, 15)
TUNABLE_ARR(mobility_queen, 28)

static Tunable tunables[] = {
	T(mobility_knight),
	T(mobility_bishop),
	T(mobility_rook),
	T(mobility_queen),
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

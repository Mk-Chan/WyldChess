#ifndef MISC_H
#define MISC_H

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

#include <sys/time.h>

#include "defs.h"

#define rng(void) (genrand64_int64())

extern void init_genrand64(u64 seed);
extern u64 genrand64_int64(void);
extern void init_zobrist_keys();

extern u64 psq_keys[2][8][64];
extern u64 castle_keys[16];
extern u64 stm_key;

extern void init_timer();
extern unsigned long long curr_time();

#endif

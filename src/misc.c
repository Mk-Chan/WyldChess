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

#include "misc.h"

u64 psq_keys[2][8][64];
u64 castle_keys[16];
u64 stm_key;

struct timeval start_time;

void init_timer()
{
	gettimeofday(&start_time, 0);
}

unsigned long long curr_time()
{
	static struct timeval curr;
	gettimeofday(&curr, 0);
	return ((curr.tv_sec - start_time.tv_sec) * 1000 + ((curr.tv_usec - start_time.tv_usec) / 1000.0));
}

u64 get_rand()
{
	u64 r;
	do {
		r = rng();
	} while (!r);
	return r;
}

void init_zobrist_keys()
{
	int i, j, k;
	for (i = 0; i != 2; ++i)
		for (j = 0; j != 8; ++j)
			for (k = 0; k != 64; ++k)
				psq_keys[i][j][k] = get_rand();
	for (i = 0; i != 16; ++i)
		castle_keys[i] = get_rand();
	stm_key = get_rand();
}

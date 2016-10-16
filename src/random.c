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

#include "random.h"

HashKey psq_keys[2][8][64];
HashKey castle_keys[16];
HashKey stm_key;

void init_zobrist_keys()
{
	int i, j, k;
	for (i = 0; i != 2; ++i)
		for (j = 0; j != 8; ++j)
			for (k = 0; k != 64; ++k)
				psq_keys[i][j][k] = rng();
	for (i = 0; i != 16; ++i)
		castle_keys[i] = rng();
	stm_key = rng();
}

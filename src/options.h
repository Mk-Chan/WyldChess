#ifndef OPTIONS_H
#define OPTIONS_H

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

struct SpinOption
{
	char name[50];
	int curr_val;
	int min_val;
	int max_val;
};

enum OPTIONS
{
	MOVE_OVERHEAD
};

static struct SpinOption spin_options[] = {
	{ "MoveOverhead", 30, 1, 5000 }
};

#endif

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

#include "defs.h"

extern int doubled_pawns;
extern int isolated_pawn;
extern int rook_7th_rank;
extern int rook_open_file;
extern int rook_semi_open;
extern int pinned_piece;
extern int pawn_blocked_bishop;
extern int dual_bishops;

set_tunable(doubled_pawns)
set_tunable(isolated_pawn)
set_tunable(rook_7th_rank)
set_tunable(rook_open_file)
set_tunable(rook_semi_open)
set_tunable(pinned_piece)
set_tunable(pawn_blocked_bishop)
set_tunable(dual_bishops)

#endif

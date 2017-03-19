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

#include <time.h>
#include "magicmoves.h"
#include "position.h"
#include "misc.h"
#include "tt.h"
#include "engine.h"

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

	char input[100];
	while (1) {
		fgets(input, 100, stdin);
		if (!strncmp(input, "xboard", 6)) {
			tt_alloc_MB(&tt, 128);
			pvt_init(&pvt, 0x400);
			xboard_loop();
			tt_destroy(&tt);
			break;
		} else if (!strncmp(input, "uci", 3)) {
			tt_alloc_MB(&tt, 128);
			pvt_init(&pvt, 0x400);
			uci_loop();
			tt_destroy(&tt);
			break;
		} else if (!strncmp(input, "tune", 4)) {
			tune();
			break;
		} else {
			fprintf(stdout, "Protocol not found!\n");
		}
	}

	return 0;
}

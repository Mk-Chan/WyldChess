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
#include "random.h"
#include "tt.h"
#include "engine.h"
#include "timer.h"
#include "tune.h"

typedef void (*fxn_ptr)(int, int);

void parse_tuner_input(char* ptr)
{
	static char* eval_terms[6] = {
		"dp_mg",
		"dp_eg",
		"ip_mg",
		"ip_eg",
		"pb_mg",
		"pb_eg",
	};

	static fxn_ptr fxns[3] = {
		&set_param_doubled_pawns,
		&set_param_isolated_pawn,
		&set_param_blocked_bishop,
	};

	static int mg = -1,
		   eg = -1;
	char str[3];
	str[2] = '\0';
	int i;
	for (i = 0; i != 6; ++i) {
		if (!strncmp(ptr, eval_terms[i], 4)) {
			if (i & 1) {
				eg = atoi(ptr + 5);
				(*fxns[i >> 1])(mg, eg);
				str[0] = ptr[0];
				str[1] = ptr[1];
				fprintf(stdout, "set %s=%d,%d\n", str, mg, eg);
			} else {
				mg = atoi(ptr + 5);
			}
			break;
		}
	}
}

void parse_piece_val_tuner_input(char* ptr)
{
	static char* eval_terms[10] = {
		"p_mg",
		"p_eg",
		"n_mg",
		"n_eg",
		"b_mg",
		"b_eg",
		"r_mg",
		"r_eg",
		"q_mg",
		"q_eg"
	};

	static int mg    = -1,
		   eg    = -1;
	int i;
	for (i = 0; i != 10; ++i) {
		if (!strncmp(ptr, eval_terms[i], 4)) {
			if (i & 1) {
				eg = atoi(ptr + 5);
				set_param_piece_val((i >> 1) + 2, mg, eg);
				fprintf(stdout, "set %c=%d,%d\n", *ptr, mg_val(piece_val[(i >> 1) + 2]), eg_val(piece_val[(i >> 1) + 2]));
			} else {
				mg = atoi(ptr + 5);
			}
			break;
		}
	}
}

int main(int argc, char** argv)
{
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	init_timer();
	init_genrand64(time(0));
	init_zobrist_keys();
	initmagicmoves();
	init_atks();
	init_intervening_sqs();
	init_masks();
#ifdef PERFT
	Position pos;
	init_pos(&pos);
	set_pos(&pos, INITIAL_POSITION);
	int depth;
	if (argc > 1) {
		depth = atoi(argv[1]);
	} else {
		fprintf(stdout, "Usage: ./wyldchess <depth>\n");
		return 1;
	}
#ifdef THREADS
	performance_test_parallel(&pos, atoi(argv[1]));
#else
	performance_test(&pos, atoi(argv[1]));
#endif
	return 0;
#endif
	tt_init(&tt, 10000000);
	char input[100];
	while (1) {
		fgets(input, 100, stdin);
		if (!strncmp(input, "setvalue", 8)) {
			parse_tuner_input(input + 9);
		} else if (!strncmp(input, "setpieceval", 11)) {
			parse_piece_val_tuner_input(input + 12);
		} else if (!strncmp(input, "xboard", 6)) {
			cecp_loop();
			break;
		} else if (!strncmp(input, "uci", 3)) {
			uci_loop();
			break;
		} else {
			fprintf(stdout, "Protocol not found!\n");
		}
	}

	tt_destroy(&tt);
	return 0;
}

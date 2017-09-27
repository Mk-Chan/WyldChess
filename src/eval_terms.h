#ifndef EVAL_TERMS_H
#define EVAL_TERMS_H

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

extern int piece_val[8], king_atk_wt[7], king_shelter_close, king_shelter_far,
           doubled_pawns, isolated_pawn, mobility[7], bishop_pair,
	   rook_on_7th, rook_open_file, rook_semi_open, outpost[2];

enum EvalTermList
{
	PAWN_VAL, KNIGHT_VAL, BISHOP_VAL, ROOK_VAL, QUEEN_VAL,
	KING_SHELTER_CLOSE, KING_SHELTER_FAR,
	DOUBLED_PAWNS, ISOLATED_PAWN,
	BISHOP_PAIR, ROOK_ON_7TH, ROOK_OPEN, ROOK_SEMI_OPEN,
	KNIGHT_OUTPOST, BISHOP_OUTPOST,
	TAPERED_END,
	KNIGHT_K_ATK_WT=TAPERED_END, BISHOP_K_ATK_WT, ROOK_K_ATK_WT, QUEEN_K_ATK_WT,
	KNIGHT_MOB_WT, BISHOP_MOB_WT, ROOK_MOB_WT, QUEEN_MOB_WT,
	NUM_TERMS
};

struct EvalTerm
{
	char name[50];
	int* ptr;
};

extern struct EvalTerm eval_terms[NUM_TERMS];

static inline int* eval_term(char* term)
{
	struct EvalTerm* curr = eval_terms;
	struct EvalTerm* end  = eval_terms + NUM_TERMS;
	for (; curr != end; ++curr)
		if (!strcmp(term, curr->name))
			return curr->ptr;
	return NULL;
}

#endif

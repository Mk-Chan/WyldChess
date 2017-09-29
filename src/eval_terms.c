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

#include "eval_terms.h"

struct EvalTerm eval_terms[NUM_TERMS] = {
	// Tapered terms
	{ "PawnValue",        &piece_val[PAWN]     },
	{ "KnightValue",      &piece_val[KNIGHT]   },
	{ "BishopValue",      &piece_val[BISHOP]   },
	{ "RookValue",        &piece_val[ROOK]     },
	{ "QueenValue",       &piece_val[QUEEN]    },
	{ "KingShelterClose", &king_shelter_close  },
	{ "KingShelterFar",   &king_shelter_far    },
	{ "DoubledPawns",     &doubled_pawns       },
	{ "IsolatedPawn",     &isolated_pawn       },
	{ "BishopPair",       &bishop_pair         },
	{ "RookOn7th",        &rook_on_7th         },
	{ "RookOpenFile",     &rook_open_file      },
	{ "RookSemiOpenFile", &rook_semi_open      },
	{ "KnightOutpost",    &outpost[1]          },
	{ "BishopOutpost",    &outpost[0]          },
	// Non-tapered terms
	{ "KnightKingAtkWt",  &king_atk_wt[KNIGHT] },
	{ "BishopKingAtkWt",  &king_atk_wt[BISHOP] },
	{ "RookKingAtkWt",    &king_atk_wt[ROOK]   },
	{ "QueenKingAtkWt",   &king_atk_wt[QUEEN]  },
	{ "KnightMobilityWt", &mobility[KNIGHT]    },
	{ "BishopMobilityWt", &mobility[BISHOP]    },
	{ "RookMobilityWt",   &mobility[ROOK]      },
	{ "QueenMobilityWt",  &mobility[QUEEN]     },
};

int parse_persona_file(char const * const path)
{
	FILE* file = fopen(path, "r");
	static const int max_len = 100;
	char buf[max_len];
	while (1) {
		if (!fgets(buf, max_len, file))
			break;
		buf[strlen(buf) - 1] = '\0';
		parse_eval_term(buf, "=");
	}

	return 0;
}

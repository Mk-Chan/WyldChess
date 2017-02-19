#ifndef EVAL_H
#define EVAL_H

#include "defs.h"
#include "position.h"
#include "bitboard.h"

// King	terms
extern int king_atk_wt[7];
extern int king_open_file[2];

// Pawn structure
extern int passed_pawn[8];
extern int doubled_pawns;
extern int isolated_pawn;

// Mobility terms
extern int mobility[7];
extern int min_mob_count[7];

// Miscellaneous terms
extern int bishop_pair;
extern int roon_on_7th;
extern int rook_open_file;
extern int rook_semi_open;
extern int outpost[2];

#endif

/*
 * WyldChess, a free Xboard/Winboard compatible chess engine
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
#include "position.h"
#include "bitboard.h"

int piece_val[7] = {
	0,
	0,
	S(100, 120),
	S(300, 300),
	S(310, 310),
	S(550, 600),
	S(1000, 1100)
};

int psq_val[8][64] = {
	{ 0 },
	{ 0 },
	{	 // Pawn
		S(0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(  0, 0),
		S(0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S(0, 0), S(20, 0), S(20, 0), S(0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S(0, 0), S(20, 0), S(20, 0), S(0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(0, 0),
		S(0, 0), S( 0, 0), S( 0, 0), S( 0, 0), S(0,  0), S( 0, 0), S( 0, 0), S(  0, 0)
	},
	{	 // Knight
		S(-50, -50), S(-30, -50), S(-10, -10), S(-10, -10), S(-10, -10), S(-20, -20), S(-30, -30), S(-50, -50),
		S(-30, -30), S(-10, -10), S(0, 0), S(10, 10), S(10, 10), S(0, 0), S(-10, -10), S(-30,-30),
		S(-10, -10), S(10, 10), S(20, 20), S(25, 25), S(25, 25), S(20, 20), S(10, 10), S(-10, -10),
		S(0, 0), S(10, 10), S(30, 30), S(35, 35), S(35, 35), S(30, 30), S(10, 10), S(0, 0),
		S(0, 0), S(15, 15), S(30, 30), S(40, 40), S(40, 40), S(30, 30), S(15, 15), S(0, 0),
		S(0, 0), S(15, 15), S(40, 40), S(50, 50), S(50, 50), S(40, 40), S(15, 15), S(0, 0),
		S(-10, -10), S(-5, -5), S(5, 5), S(5, 5), S(5, 5), S(5, 5), S(-5, -5), S(-10, -10),
		S(-50, -50), S(-30, -50), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50)
	},
	{	 // Bishop
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50),
		S(10, 10), S(20, 20), S(30, 30), S(20, 20), S(20, 20), S(30, 30), S(20, 20), S(10, 10),
		S(10, 10), S(40, 40), S(40, 40), S(35, 35), S(35, 35), S(40, 40), S(40, 40), S(10, 10),
		S(25, 25), S(30, 30), S(40, 40), S(40, 40), S(40, 40), S(40, 40), S(30, 30), S(25, 25),
		S(20, 20), S(40, 40), S(30, 30), S(30, 30), S(30, 30), S(30, 30), S(40, 40), S(20, 20),
		S(10, 10), S(20, 20), S(25, 25), S(20, 20), S(20, 20), S(25, 25), S(20, 20), S(10, 10),
		S(-10, -10), S(-5, -5), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(-5, -5), S(-10, -10),
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50)
	},
	{	 // Rook
		S(-20, -20), S(-20, -20), S(-10, -10), S(5, 5), S(5, 5), S(-20, -20), S(-20, -20), S(-20, -20),
		S(-20, -20), S(-20, -20), S(-10, -10), S(5, 5), S(5, 5), S(-20, -20), S(-20, -20), S(-20, -20),
		S(-10, -10), S(-10, -10), S(-5, -5), S(5, 5), S(5, 5), S(-5, -5), S(-10, -10), S(-10, -10),
		S(0, 0), S(0, 0), S(5, 5), S(5, 5), S(5, 5), S(5, 5), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(5, 5), S(5, 5), S(5, 5), S(5, 5), S(0, 0), S(0, 0),
		S(0, 0), S(0, 0), S(5, 5), S(5, 5), S(5, 5), S(5, 5), S(0, 0), S(0, 0),
		S(10, 10), S(10, 10), S(15, 15), S(15, 15), S(15, 15), S(15, 15), S(10, 10), S(10, 10),
		S(-10, -10), S(-10, -10), S(-5, -5), S(5, 5), S(5, 5), S(-5, -5), S(-10, -10), S(-10, -10)
	},
	{
		 // Queen
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50),
		S(-30, -30), S(-10, -10), S(0, 0), S(20, 20), S(20, 20), S(0, 0), S(-10, -10), S(-30, -30),
		S(-10, -10), S(10, 10), S(10, 10), S(15, 15), S(15, 15), S(10, 10), S(10, 10), S(-10, -10),
		S(0, 0), S(10, 10), S(20, 20), S(25, 25), S(25, 25), S(20, 20), S(10, 10), S(0, 0),
		S(0, 0), S(15, 15), S(30, 30), S(40, 40), S(40, 40), S(30, 30), S(15, 15), S(0, 0),
		S(0, 0), S(15, 15), S(40, 40), S(50, 50), S(50, 50), S(40, 40), S(15, 15), S(0, 0),
		S(-10, -10), S(-5, -5), S(5, 5), S(5, 5), S(5, 5), S(5, 5), S(-5, -5), S(-10, -10),
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50),
	},
	{
		 // King
		 S(30, -30), S(35, -25), S(10, -15), S(-10, -10), S(-10, -10), S(10, -15), S(35, -25), S(30, -30),
		 S(10, -10), S(20, -5), S(0, 0), S(-15, 10), S(-15, 10), S(0, 0), S(20, -5), S(10, -10),
		 S(-20, 10), S(-25, 15), S(-30, 15), S(-30, 30), S(-30, 30), S(-30, 15), S(-25, 15), S(-20, 10),
		 S(-40, 20), S(-50, 25), S(-60, 30), S(-70, 40), S(-70, 40), S(-60, 30), S(-50, 25), S(-40, 20),
		 S(-70, 20), S(-80, 25), S(-90, 30), S(-90, 40), S(-90, 40), S(-90, 30), S(-80, 25), S(-70, 20),
		 S(-70, 10), S(-80, 15), S(-90, 15), S(-90, 30), S(-90, 30), S(-90, 15), S(-80, 15), S(-70, 10),
		 S(-80, -10), S(-80, -5), S(-90, 0), S(-90, 10), S(-90, 10), S(-90, -0), S(-80, -5), S(-80, -10),
		 S(-90, -30), S(-90, -25), S(-90, -15), S(-90, -10), S(-90, -10), S(-90, -15), S(-90, -25), S(-90, -30)
	}
};
int mobility[7][32] = {
	{ 0 }, { 0 }, { 0 },
	{	// Knight
		S(-20, -30), S(-10, -10), S(0, 0), S(10, 5), S(15, 10), S(20, 15), S(25, 20), S(30, 25), S(30, 30)
	},
	{	// Bishop
		S(-40, -80), S(-30, -60), S(-20, -40), S(-10, -20), S(0, -10), S(5, 0), S(10, 10), S(15, 20),
		S(20, 30), S(25, 40), S(30, 40), S(35, 40), S(40, 45), S(40, 45), S(40, 45), S(40, 45)
	},
	{
		// Rook
		S(-40, -80), S(-30, -50), S(-20, -20), S(-10, -10), S(0, 0), S(5, 10), S(10, 20), S(15, 30),
		S(20, 40), S(25, 50), S(30, 60), S(35, 70), S(40, 70), S(40, 70), S(40, 80), S(40, 90)
	},
	{	// Queen
		S(-30, -80), S(-10, -40), S(0, -10), S(2, 0), S(4, 5), S(6, 10), S(8, 15), S(10, 20),
		S(12, 25), S(14, 30), S(16, 35), S(18, 40), S(20, 40), S(25, 40), S(25, 40), S(25, 40),
		S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40),
		S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40), S(25, 40)
	}
};
int passed_pawn[2][8] = {
	{ 0, S(0, 0), S(0, 0), S(20, 30), S(30, 50), S(50, 80), S(80, 100), 0 },
	{ 0, S(80, 100), S(50, 80), S(30, 50), S(20, 30), S(0, 0), S(0, 0), 0 }
};
int doubled_pawns       = S(-20, -30);
int isolated_pawn       = S(-10, -20);
int rook_7th_rank       = S(50, 0);
int rook_open_file      = S(20, 0);
int rook_semi_open_file = S(10, 0);

typedef struct Eval_s {

	u64 pawn_bb[2];
	u64 passed_pawn_bb[2];
	u64 atks_bb[2][7];

} Eval;

static int eval_passed_pawns(Position* const pos, Eval* const ev)
{
	int eval[2] = { S(0, 0), S(0, 0) };
	return eval[WHITE] - eval[BLACK];
}

static int eval_pawns(Position* const pos, Eval* const ev)
{
	int eval[2] = { S(0, 0), S(0, 0) };
	u64 bb, pawn_bb, opp_pawn_bb, atk_bb;
	int c, sq;
	for (c = WHITE; c <= BLACK; ++c) {
		ev->atks_bb[c][PAWN]  = 0ULL;
		ev->passed_pawn_bb[c] = 0ULL;
		pawn_bb               = ev->pawn_bb[c];
		opp_pawn_bb           = ev->pawn_bb[!c];
		bb                    = pawn_bb;
		while (bb) {
			sq  = bitscan(bb);
			bb &= bb - 1;
			atk_bb = p_atks[c][sq];
			// king atk counter here
			ev->atks_bb[c][PAWN] |= atk_bb;
			if (pawn_bb & file_forward_mask[c][sq])
				eval[c] += doubled_pawns;
			if (!(pawn_bb & adjacent_files_mask[file_of(sq)]))
				eval[c] += isolated_pawn;
			if (!(opp_pawn_bb & passed_pawn_mask[c][sq])) {
				ev->passed_pawn_bb[c] |= BB(sq);
				eval[c] += passed_pawn[c][rank_of(sq)];
			}
		}
	}
	return eval[WHITE] - eval[BLACK];
}

static int eval_pieces(Position* const pos, Eval* const ev)
{
	int eval[2]  = { S(0, 0), S(0, 0) };
	eval[WHITE] += popcnt((pos->bb[ROOK] & pos->bb[WHITE] & rank_mask[RANK_7])) * rook_7th_rank;
	eval[BLACK] += popcnt((pos->bb[ROOK] & pos->bb[BLACK] & rank_mask[RANK_2])) * rook_7th_rank;
	int sq, c;
	u64 pinned_bb = pos->state->pinned_bb;
	u64 bb, c_bb, atk_bb, opp_pawn_atks_bb;
	for (c = WHITE; c <= BLACK; ++c) {
		c_bb = pos->bb[c];
		opp_pawn_atks_bb = ev->atks_bb[!c][PAWN];

		// Knight
		ev->atks_bb[c][KNIGHT] = 0ULL;
		bb = pos->bb[KNIGHT] & c_bb & ~pinned_bb;
		while (bb) {
			sq       = bitscan(bb);
			bb      &= bb - 1;
			atk_bb   = n_atks[sq] & ~c_bb & ~opp_pawn_atks_bb;
			eval[c] += mobility[KNIGHT][popcnt(atk_bb)];
			ev->atks_bb[c][KNIGHT] |= atk_bb;
		}

		// Bishop
		ev->atks_bb[c][BISHOP] = 0ULL;
		bb = pos->bb[BISHOP] & c_bb & ~pinned_bb;
		while (bb) {
			sq       = bitscan(bb);
			bb      &= bb - 1;
			atk_bb   = Bmagic(sq, pos->bb[FULL]) & ~c_bb & ~opp_pawn_atks_bb;
			eval[c] += mobility[BISHOP][popcnt(atk_bb)];
			ev->atks_bb[c][BISHOP] |= atk_bb;
		}

		// Rook
		ev->atks_bb[c][ROOK] = 0ULL;
		bb = pos->bb[ROOK] & c_bb & ~pinned_bb;
		while (bb) {
			sq       = bitscan(bb);
			bb      &= bb - 1;
			atk_bb   = Rmagic(sq, pos->bb[FULL]) & ~c_bb & ~opp_pawn_atks_bb;
			eval[c] += mobility[ROOK][popcnt(atk_bb)];
			ev->atks_bb[c][ROOK] |= atk_bb;
			if (!(file_forward_mask[c][sq] & ev->pawn_bb[c])) {
				eval[c] += !(file_forward_mask[c][sq] & ev->pawn_bb[!c])
					           ? rook_open_file
						   : rook_semi_open_file;
			}
		}

		// Queen
		ev->atks_bb[c][QUEEN] = 0ULL;
		bb = pos->bb[QUEEN] & c_bb & ~pinned_bb;
		while (bb) {
			sq = bitscan(bb);
			bb &= bb - 1;
			atk_bb = Qmagic(sq, pos->bb[FULL]) & ~c_bb & ~opp_pawn_atks_bb;
			eval[c] += mobility[QUEEN][popcnt(atk_bb)];
			ev->atks_bb[c][QUEEN] |= atk_bb;
		}
	}
	return eval[WHITE] - eval[BLACK];
}

int evaluate(Position* const pos)
{
	Eval ev;
	ev.pawn_bb[WHITE] = pos->bb[PAWN] & pos->bb[WHITE];
	ev.pawn_bb[BLACK] = pos->bb[PAWN] & pos->bb[BLACK];
	int eval = pos->state->piece_psq_eval[WHITE] - pos->state->piece_psq_eval[BLACK];
	eval += eval_pawns(pos, &ev);
	eval += eval_pieces(pos, &ev);
	eval += eval_passed_pawns(pos, &ev);
	eval  = phased_val(eval, pos->state->phase);
	return pos->stm == WHITE ? eval : -eval;
}
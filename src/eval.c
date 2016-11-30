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
#include "position.h"
#include "bitboard.h"

int piece_val[8] = {
	0,
	0,
	S(90, 100),
	S(400, 320),
	S(400, 330),
	S(600, 550),
	S(1200, 1000),
	S(0, 0)
};

int psq_val[8][64] = {
	{ 0 },
	{ 0 },
	{	 // Pawn
		S(0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S(0,  0),
		S(0, -5), S( 5, -5), S( 5, -5), S( 0, -5), S( 0, -5), S( 5, -5), S( 5, -5), S(0, -5),
		S(0,  5), S( 0,  5), S( 0,  5), S( 0,  5), S( 0,  5), S( 0,  5), S( 0,  5), S(0,  5),
		S(0, 10), S( 0, 10), S( 0, 10), S(20, 10), S(20, 10), S( 0, 10), S( 0, 10), S(0, 10),
		S(0, 30), S( 0, 30), S( 0, 30), S(20, 30), S(20, 30), S( 0, 30), S( 0, 30), S(0, 30),
		S(0, 50), S( 0, 50), S( 0, 50), S( 0, 50), S( 0, 50), S( 0, 50), S( 0, 50), S(0, 50),
		S(0, 80), S( 0, 80), S( 0, 80), S( 0, 80), S( 0, 80), S( 0, 80), S( 0, 80), S(0, 80),
		S(0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S( 0,  0), S(0,  0)
	},
	{	 // Knight
		S(-50, -50), S(-30, -50), S(-10, -10), S(-10, -10), S(-10, -10), S(-20, -20), S(-30, -30), S(-50, -50),
		S(-30, -30), S(-10, -10), S(  0,   0), S( 10,  10), S( 10,  10), S(  0,   0), S(-10, -10), S(-30, -30),
		S(-10, -10), S( 10,  10), S( 20,  20), S( 25,  25), S( 25,  25), S( 20,  20), S( 10,  10), S(-10, -10),
		S(  0,   0), S( 10,  10), S( 30,  30), S( 35,  35), S( 35,  35), S( 30,  30), S( 10,  10), S(  0,   0),
		S(  0,   0), S( 15,  15), S( 30,  30), S( 40,  40), S( 40,  40), S( 30,  30), S( 15,  15), S(  0,   0),
		S(  0,   0), S( 15,  15), S( 40,  40), S( 50,  50), S( 50,  50), S( 40,  40), S( 15,  15), S(  0,   0),
		S(-10, -10), S( -5,  -5), S(  5,   5), S(  5,   5), S(  5,   5), S(  5,   5), S( -5,  -5), S(-10, -10),
		S(-50, -50), S(-30, -50), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50)
	},
	{	 // Bishop
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50),
		S( 10,  10), S( 20,  20), S( 30,  30), S( 20,  20), S( 20,  20), S( 30,  30), S( 20,  20), S( 10,  10),
		S( 10,  10), S( 40,  40), S( 40,  40), S( 35,  35), S( 35,  35), S( 40,  40), S( 40,  40), S( 10,  10),
		S( 25,  25), S( 30,  30), S( 40,  40), S( 40,  40), S( 40,  40), S( 40,  40), S( 30,  30), S( 25,  25),
		S( 20,  20), S( 40,  40), S( 30,  30), S( 30,  30), S( 30,  30), S( 30,  30), S( 40,  40), S( 20,  20),
		S( 10,  10), S( 20,  20), S( 25,  25), S( 20,  20), S( 20,  20), S( 25,  25), S( 20,  20), S( 10,  10),
		S(-10, -10), S( -5,  -5), S(  0,   0), S(  0,   0), S(  0,   0), S(  0,   0), S( -5,  -5), S(-10, -10),
		S(-50, -50), S(-30, -30), S(-10, -10), S(-10, -10), S(-10, -10), S(-10, -10), S(-30, -30), S(-50, -50)
	},
	{	 // Rook
		S(-20, -20), S(-20, -20), S(-10, -10), S( 5,  5), S( 5,  5), S(-20, -20), S(-20, -20), S(-20, -20),
		S(-20, -20), S(-20, -20), S(-10, -10), S( 5,  5), S( 5,  5), S(-20, -20), S(-20, -20), S(-20, -20),
		S(-10, -10), S(-10, -10), S( -5,  -5), S( 5,  5), S( 5,  5), S( -5,  -5), S(-10, -10), S(-10, -10),
		S(  0,   0), S(  0,   0), S(  5,   5), S( 5,  5), S( 5,  5), S(  5,   5), S(  0,   0), S(  0,   0),
		S(  0,   0), S(  0,   0), S(  5,   5), S( 5,  5), S( 5,  5), S(  5,   5), S(  0,   0), S(  0,   0),
		S(  0,   0), S(  0,   0), S(  5,   5), S( 5,  5), S( 5,  5), S(  5,   5), S(  0,   0), S(  0,   0),
		S( 50,  10), S( 50,  10), S( 55,  15), S(55, 15), S(55, 15), S( 55,  15), S( 50,  10), S( 50,  10),
		S(-10, -10), S(-10, -10), S( -5,  -5), S( 5,  5), S( 5,  5), S( -5,  -5), S(-10, -10), S(-10, -10)
	},
	{
		 // Queen
		S(-10, -50), S(  0, -30), S(  0, -10), S(  0, -5), S(  0, -5), S(  0, -10), S(  0, -30), S(-10, -50),
		S(  0, -30), S(  5, -15), S(  5,   0), S(  5,  5), S(  5,  5), S(  5,   0), S(  5, -15), S(  0, -30),
		S(  0, -10), S(  5,   0), S(  5,  10), S(  5, 15), S(  5, 15), S(  5,  10), S(  5,   0), S(  0, -10),
		S(  0,  -5), S( 10,  10), S( 10,  15), S( 10, 20), S( 10, 20), S( 10,  15), S( 10,  10), S(  0,  -5),
		S(  0,  -5), S( 10,  10), S( 10,  15), S( 10, 20), S( 10, 20), S( 10,  15), S( 10,  10), S(  0,  -5),
		S(  0, -10), S(  5,   0), S(  5,  10), S(  5, 15), S(  5, 15), S(  5,  10), S(  5,   0), S(  0, -10),
		S(  0, -30), S(  5, -15), S(  5,   0), S(  5,  5), S(  5,  5), S(  5,   0), S(  5, -15), S(  0, -30),
		S(-10, -50), S(  0, -30), S(  0, -10), S(  0, -5), S(  0, -5), S(  0, -10), S(  0, -30), S(-10, -50)
	},
	{
		 // King
		S( 30, -70), S( 45, -45), S( 35, -35), S(-10, -20), S(-10, -20), S( 35, -35), S( 45, -45), S( 30, -70),
		S( 10, -40), S( 20, -25), S(  0, -10), S(-15,   5), S(-15,   5), S(  0, -10), S( 20, -25), S( 10, -40),
		S(-20, -30), S(-25, -15), S(-30,   5), S(-30,  10), S(-30,  10), S(-30,   5), S(-25, -15), S(-20, -30),
		S(-40, -20), S(-50,   5), S(-60,  10), S(-70,  20), S(-70,  20), S(-60,  10), S(-50,   5), S(-40, -20),
		S(-70, -20), S(-80,   5), S(-90,  10), S(-90,  20), S(-90,  20), S(-90,  10), S(-80,   5), S(-70, -20),
		S(-70, -30), S(-80, -15), S(-90,   5), S(-90,  10), S(-90,  10), S(-90,   5), S(-80, -15), S(-70, -30),
		S(-80, -40), S(-80, -25), S(-90, -10), S(-90,   0), S(-90,   0), S(-90, -10), S(-80, -25), S(-80, -40),
		S(-90, -70), S(-90, -45), S(-90, -15), S(-90, -20), S(-90, -20), S(-90, -15), S(-90, -45), S(-90, -70)
	}
};

int mobility[][29] = {
	{ 0 }, { 0 }, { 0 },
	{	// Knight
		S(-30, -40), S(-20, -30), S(-10, -10), S(0, 0), S(10, 10), S(20, 20), S(30, 30), S(35, 35), S(40, 40)
	},
	{	// Bishop
		S(-30, -30), S(-20, -20), S(-10, -10), S(0, 0), S(10, 10), S(20, 20), S(25, 25), S(30, 30),
		S(35, 35), S(40, 40), S(40, 40), S(40, 40), S(40, 45), S(40, 45), S(40, 45), S(40, 45)
	},
	{	// Rook
		S(-30, -40), S(-30, -30), S(-20, -20), S(-10, -10), S(0, 0), S(5, 10), S(10, 15), S(15, 20),
		S(20, 25), S(25, 30), S(30, 35), S(35, 40), S(40, 45), S(40, 50), S(40, 55), S(40, 60)
	},
	{	// Queen
		S(-10, -50), S(-5, -30), S(0, -10), S(5, 0), S(5, 5), S(10, 10), S(10, 10), S(15, 15),
		S(15, 15), S(15, 15), S(15, 15), S(20, 20), S(20, 20), S(20, 20), S(25, 25),
		S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25),
		S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25), S(25, 25)
	}
};

int king_atk_table[100] = { // Taken from CPW(Glaurung 1.2)
	  0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
	  8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
	 55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
	150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
	250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
	350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
	450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
	550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
	650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
	650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

// King terms
int king_atk_wt[7] = { 0, 0, 0, 3, 3, 4, 5 };
int king_cover[4]  = { 6, 4, 2, 0 };

// Pawn terms
int passed_pawn[8] = { 0, S(5, 10), S(10, 20), S(20, 40), S(30, 70), S(50, 120), S(80, 200), 0 };
int doubled_pawns  = S(-20, -30);
int isolated_pawn  = S(-10, -20);
int backward_pawn  = S(-10, -20);

// Knight terms

// Bishop terms
int blocked_bishop = S(-5, -5);
int dual_bishops   = S(30, 50);

// Rook terms
int rook_open_file = S(30, 0);
int rook_semi_open = S(10, 0);

// Miscellaneous terms
int outpost[2]           = { S(15, 0), S(15, 10) }; // Bishop, Knight
int protected_outpost[2] = { S(10, 5), S(15, 10) }; // Bishop, Knight
int weak_color_occupied  = S(5, 5);

typedef struct Eval_s {

	u64 atks_bb[2][7];
	u64 pawn_bb[2];
	u64 king_danger_zone_bb[2];
	u64 pinned_bb[2];
	u64 blocked_pawns_bb[2];
	int king_atks[2];

} Eval;

static int eval_pawns(Position* const pos, Eval* const ev)
{
	int eval[2] = { S(0, 0), S(0, 0) };
	u64 bb, pawn_bb, opp_pawn_bb, atk_bb;
	u64* atks_bb;
	int c, sq;
	for (c = WHITE; c <= BLACK; ++c) {
		atks_bb     = ev->atks_bb[c];
		pawn_bb     = ev->pawn_bb[c];
		opp_pawn_bb = ev->pawn_bb[!c];
		bb          = pawn_bb;
		while (bb) {
			sq     = bitscan(bb);
			bb    &= bb - 1;
			atk_bb = p_atks_bb[c][sq];

			atks_bb[PAWN] |= atk_bb;

			if (!(adjacent_files_mask[file_of(sq)] & pawn_bb))
				eval[c] += isolated_pawn;

			if (file_forward_mask[c][sq] & pawn_bb) {
				eval[c] += doubled_pawns;
			} else if (!(opp_pawn_bb & passed_pawn_mask[c][sq])) {
				eval[c] += passed_pawn[(c == WHITE ? rank_of(sq) : rank_of((sq ^ 56)))];
			} else if (    (opp_pawn_bb & backwards_pawn_restrictors_mask[c][sq])
				   &&  (pawn_bb & adjacent_forward_mask[c][sq])
				   && !(pawn_bb & adjacent_sqs_mask[sq])) {
				eval[c] += backward_pawn;
			}
		}
	}
	return eval[WHITE] - eval[BLACK];
}

static int eval_pieces(Position* const pos, Eval* const ev)
{
	int sq, c, pt, ksq;
	u64  curr_bb, c_bb, atk_bb, xrayable_pieces_bb, strong_color_bb,
	     mobility_mask, non_pinned_bb, king_atks_bb, sq_bb;
	u64* atks_bb;

	u64 const p_atks_bb[2] = {
		ev->atks_bb[WHITE][PAWN],
		ev->atks_bb[BLACK][PAWN]
	};

	int  eval[2] = { S(0, 0), S(0, 0) };
	u64* bb      = pos->bb;
	u64  full_bb = bb[FULL];

	for (c = WHITE; c <= BLACK; ++c) {
		atks_bb            =   ev->atks_bb[c];
		non_pinned_bb      =  ~ev->pinned_bb[c];
		c_bb               =   bb[c];
		ksq                =   pos->king_sq[c];
		mobility_mask      = ~(ev->blocked_pawns_bb[c] | BB(ksq) | p_atks_bb[!c]);
		xrayable_pieces_bb =   c_bb ^ (BB(ksq) | ev->blocked_pawns_bb[c] | ev->pinned_bb[c]);
		ev->king_atks[!c] +=   king_cover[popcnt(passed_pawn_mask[c][ksq] & k_atks_bb[ksq] & ev->pawn_bb[c])];

		for (pt = KNIGHT; pt != KING; ++pt) {
			curr_bb = bb[pt] & c_bb;
			if (pt == BISHOP) {
				if (   (curr_bb & color_sq_mask[WHITE])
				    && (curr_bb & color_sq_mask[BLACK])) {
					eval[c] += dual_bishops;
				} else if (popcnt(curr_bb) == 1) {
					strong_color_bb = color_sq_mask[sq_color[bitscan(curr_bb)]];
					eval[c]        += popcnt(~strong_color_bb & bb[!c] & ~bb[PAWN]) * weak_color_occupied;
					eval[c]        += popcnt(strong_color_bb & ev->pawn_bb[c]) * blocked_bishop;
				}
			}

			curr_bb &= non_pinned_bb;
			while (curr_bb) {
				sq       = bitscan(curr_bb);
				sq_bb    = BB(sq);
				curr_bb &= curr_bb - 1;
				atk_bb   = get_atks(sq, pt, full_bb);
				atk_bb  |= get_atks(sq, pt, full_bb ^ (atk_bb & xrayable_pieces_bb));

				atks_bb[pt] |= atk_bb;

				king_atks_bb = atk_bb & ev->king_danger_zone_bb[!c];

				atk_bb  &= mobility_mask;
				eval[c] += mobility[pt][popcnt(atk_bb)];

				ev->king_atks[c] += popcnt(king_atks_bb) * king_atk_wt[pt];

				if (   (pt == KNIGHT || pt == BISHOP)
				    && (~p_atks_bb[!c] & sq_bb)
				    && (sq_bb & outpost_ranks_mask[c])) {

					eval[c] += outpost[pt & 1];
					if (p_atks_bb[c] & sq_bb)
						eval[c] += protected_outpost[pt & 1];

				} else if (     pt == ROOK
					   && !(file_forward_mask[c][sq] & ev->pawn_bb[c])) {
					eval[c] += !(file_forward_mask[c][sq] & ev->pawn_bb[!c])
						? rook_open_file
						: rook_semi_open;
				}
			}
		}

		// Pinned
		curr_bb = ~non_pinned_bb;
		while (curr_bb) {
			sq                = bitscan(curr_bb);
			curr_bb          &= curr_bb - 1;
			pt                = piece_type(pos->board[sq]);
			atk_bb            = get_atks(sq, pt, full_bb);
			atk_bb           |= get_atks(sq, pt, full_bb ^ (atk_bb & xrayable_pieces_bb));
			atks_bb[pt]      |= atk_bb;
			king_atks_bb      = atk_bb & ev->king_danger_zone_bb[!c];
			ev->king_atks[c] += popcnt(king_atks_bb) * king_atk_wt[pt];
		}
	}

	return eval[WHITE] - eval[BLACK];
}

int evaluate(Position* const pos)
{
	if (   popcnt(pos->bb[FULL]) <= 4
	    && insufficient_material(pos))
		return 0;
	Eval ev;
	int ksq, c, pt;
	for (c = WHITE; c <= BLACK; ++c) {
		ksq             = pos->king_sq[c];
		ev.king_atks[c] = 0;
		ev.pawn_bb[c]   = pos->bb[PAWN] & pos->bb[c];
		ev.king_danger_zone_bb[c] = k_atks_bb[ksq] | BB(ksq);
		for (pt = PAWN; pt != KING; ++pt)
			ev.atks_bb[c][pt] = 0ULL;
	}

	ev.blocked_pawns_bb[WHITE] = (pos->bb[FULL] >> 8) & ev.pawn_bb[WHITE];
	ev.blocked_pawns_bb[BLACK] = (pos->bb[FULL] << 8) & ev.pawn_bb[BLACK];

	ev.pinned_bb[WHITE] = get_pinned(pos, WHITE);
	ev.pinned_bb[BLACK] = get_pinned(pos, BLACK);

	int eval = pos->state->piece_psq_eval[WHITE] - pos->state->piece_psq_eval[BLACK];

	eval += eval_pawns(pos, &ev);
	eval += eval_pieces(pos, &ev);

	ev.king_atks[WHITE] = min(max(ev.king_atks[WHITE], 0), 99);
	ev.king_atks[BLACK] = min(max(ev.king_atks[BLACK], 0), 99);

	int king_atk_diff = king_atk_table[ev.king_atks[WHITE]] - king_atk_table[ev.king_atks[BLACK]];
	eval += S(king_atk_diff, 0);

	eval  = phased_val(eval, pos->state->phase);

	return pos->stm == WHITE ? eval : -eval;
}

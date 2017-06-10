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

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "bitboard.h"
#include "magicmoves.h"

u64 p_atks_bb[2][64];
u64 n_atks_bb[64];
u64 k_atks_bb[64];
u64 b_pseudo_atks_bb[64];
u64 r_pseudo_atks_bb[64];
u64 q_pseudo_atks_bb[64];
u64 dirn_sqs_bb[64][64];
u64 intervening_sqs_bb[64][64];

u64 passed_pawn_mask[2][64];
u64 file_forward_mask[2][64];
u64 adjacent_files_mask[8];
u64 adjacent_ranks_mask[8];
u64 adjacent_sqs_mask[64];
u64 color_sq_mask[2];
u64 adjacent_forward_mask[2][64];
u64 outpost_ranks_mask[2];

int distance[64][64];
int sq_color[64];
int rank_lookup[2][8];

u64 rank_mask[8] = {
	0xffULL,
	0xffULL << 8,
	0xffULL << 16,
	0xffULL << 24,
	0xffULL << 32,
	0xffULL << 40,
	0xffULL << 48,
	0xffULL << 56
};

u64 file_mask[8] = {
	0x0101010101010101ULL,
	0x0202020202020202ULL,
	0x0404040404040404ULL,
	0x0808080808080808ULL,
	0x1010101010101010ULL,
	0x2020202020202020ULL,
	0x4040404040404040ULL,
	0x8080808080808080ULL
};

static inline int file_diff(int sq1, int sq2)
{
	return abs((sq1 % 8) - (sq2 % 8));
}

void init_masks()
{
	int c, sq;

	outpost_ranks_mask[WHITE] = rank_mask[RANK_4] | rank_mask[RANK_5] | rank_mask[RANK_6];
	outpost_ranks_mask[BLACK] = rank_mask[RANK_3] | rank_mask[RANK_4] | rank_mask[RANK_5];

	for (sq = 0; sq != 8; ++sq) {
		adjacent_files_mask[sq] = 0ULL;
		adjacent_ranks_mask[sq] = 0ULL;
		if (sq != 7) {
			adjacent_files_mask[sq] |= file_mask[sq + 1];
			adjacent_ranks_mask[sq] |= rank_mask[sq + 1];
		}
		if (sq != 0) {
			adjacent_files_mask[sq] |= file_mask[sq - 1];
			adjacent_ranks_mask[sq] |= rank_mask[sq - 1];
		}
	}

	color_sq_mask[WHITE] = 0ULL;
	color_sq_mask[BLACK] = 0ULL;
	for (sq = 0; sq != 64; ++sq) {
		if (   (rank_of(sq) % 2 == 0 && sq % 2 == 0)
		    || (rank_of(sq) % 2 == 1 && sq % 2 == 1)) {
			color_sq_mask[BLACK] |= BB(sq);
			sq_color[sq] = BLACK;
		} else {
			color_sq_mask[WHITE] |= BB(sq);
			sq_color[sq] = WHITE;
		}

		adjacent_sqs_mask[sq] = 0ULL;
		if (file_of(sq) != 0)
			adjacent_sqs_mask[sq] |= BB((sq - 1));
		if (file_of(sq) != 7)
			adjacent_sqs_mask[sq] |= BB((sq + 1));
	}

	int forward, i;
	for (c = WHITE; c <= BLACK; ++c) {
		forward = c == WHITE ? 1 : -1;
		for (sq = 0; sq != 64; ++sq) {
			file_forward_mask[c][sq] = 0ULL;
			for (i = sq + forward * 8; i >= 0 && i <= 63; i += forward * 8)
				file_forward_mask[c][sq] |= BB(i);
		}
	}

	for (c = WHITE; c <= BLACK; ++c) {
		for (sq = 0; sq != 64; ++sq) {
			passed_pawn_mask[c][sq] = file_forward_mask[c][sq];
			if (file_of(sq) != 0)
				passed_pawn_mask[c][sq] |= file_forward_mask[c][sq - 1];
			if (file_of(sq) != 7)
				passed_pawn_mask[c][sq] |= file_forward_mask[c][sq + 1];

			adjacent_forward_mask[c][sq] = passed_pawn_mask[c][sq] & adjacent_files_mask[file_of(sq)];
		}
	}
}

void init_atks()
{
	static int king_offsets[8] = { -9, -8, -7, -1, 1, 7, 8, 9 };
	static int knight_offsets[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
	static int pawn_offsets[2][2] = { { 7, 9 }, { -9, -7 } };
	int off, ksq, nsq, psq, c,
	    sq = 0;

	for (; sq != 64; ++sq) {
		k_atks_bb[sq] = 0ULL;
		n_atks_bb[sq] = 0ULL;
		p_atks_bb[WHITE][sq] = 0ULL;
		p_atks_bb[BLACK][sq] = 0ULL;

		for (off = 0; off != 8; ++off) {
			ksq = sq + king_offsets[off];
			if (   ksq <= H8
			    && ksq >= A1
			    && file_diff(sq, ksq) <= 1)
				k_atks_bb[sq] |= BB(ksq);

			nsq = sq + knight_offsets[off];
			if (   nsq <= H8
			    && nsq >= A1
			    && file_diff(sq, nsq) <= 2)
				n_atks_bb[sq] |= BB(nsq);
		}

		for (off = 0; off != 2; ++off) {
			for (c = 0; c != 2; ++c) {
				psq = sq + pawn_offsets[c][off];
				if (   psq <= H8
				    && psq >= A1
				    && file_diff(sq, psq) <= 1)
					p_atks_bb[c][sq] |= BB(psq);
			}
		}

		b_pseudo_atks_bb[sq] = Bmagic(sq, 0ULL);
		r_pseudo_atks_bb[sq] = Rmagic(sq, 0ULL);
		q_pseudo_atks_bb[sq] = Qmagic(sq, 0ULL);
	}
}

void init_intervening_sqs()
{
	int i, j, high, low;
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
			distance[i][j] = max(abs(rank_of(i) - rank_of(j)), abs(file_of(i) - file_of(j)));
			intervening_sqs_bb[i][j] = 0ULL;
			if (i == j)
				continue;
			high = j;
			if (i > j) {
				high = i;
				low = j;
			}
			else
				low = i;
			if (file_of(high) == file_of(low)) {
				dirn_sqs_bb[i][j] = Rmagic(high, 0ULL) & Rmagic(low, 0ULL);
				for (high -= 8; high != low; high -= 8)
					intervening_sqs_bb[i][j] |= BB(high);
			}
			else if (rank_of(high) == rank_of(low)) {
				dirn_sqs_bb[i][j] = Rmagic(high, 0ULL) & Rmagic(low, 0ULL);
				for (--high; high != low; high--)
					intervening_sqs_bb[i][j] |= BB(high);
			}
			else if (rank_of(high) - rank_of(low) == file_of(high) - file_of(low)) {
				dirn_sqs_bb[i][j] = Bmagic(high, 0ULL) & Bmagic(low, 0ULL);
				for (high -= 9; high != low; high -= 9)
					intervening_sqs_bb[i][j] |= BB(high);
			}
			else if (rank_of(high) - rank_of(low) == file_of(low) - file_of(high)) {
				dirn_sqs_bb[i][j] = Bmagic(high, 0ULL) & Bmagic(low, 0ULL);
				for (high -= 7; high != low; high -= 7)
					intervening_sqs_bb[i][j] |= BB(high);
			}
		}
	}
}

void init_lookups()
{
	init_atks();
	init_intervening_sqs();
	init_masks();
	int c, r;
	for (c = WHITE; c <= BLACK; ++c)
		for (r = RANK_1; r <= RANK_8; ++r)
			rank_lookup[c][r] = c == WHITE ? r : RANK_8 - r;
}

void print_bb(u64 bb)
{
	printf("Bitboard:\n");
	int sq = 0;
	for (; sq != 64; ++sq) {
		if (sq && !(sq & 7))
			printf("\n");

		if ((1ULL << (sq ^ 56)) & bb)
			printf("X  ");
		else
			printf("-  ");
	}
	printf("\n");
}

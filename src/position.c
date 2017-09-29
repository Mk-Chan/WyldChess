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

#include <stdio.h>
#include "position.h"
#include "misc.h"

int phase[8] = { 0, 0, 1, 10, 10, 20, 40, 0 };
int is_frc = 0;
int castling_rook_pos[2][2];
u32 castle_perms[64];

static inline u32 get_piece_from_char(char c)
{
	switch (c) {
	case 'p': return BP;
	case 'r': return BR;
	case 'n': return BN;
	case 'b': return BB;
	case 'q': return BQ;
	case 'k': return BK;
	case 'P': return WP;
	case 'R': return WR;
	case 'N': return WN;
	case 'B': return WB;
	case 'Q': return WQ;
	case 'K': return WK;
	default : return -1;
	}
}

static inline u32 get_cr_from_char(char c)
{
	switch (c) {
	case 'K': return WKC;
	case 'Q': return WQC;
	case 'k': return BKC;
	case 'q': return BQC;
	default : return -1;
	}
}

void get_position_copy(struct Position const * const pos, struct Position* const copy_pos)
{
	memcpy(copy_pos->bb, pos->bb, sizeof(u64) * 9);
	copy_pos->stm = pos->stm;
	copy_pos->king_sq[WHITE] = pos->king_sq[WHITE];
	copy_pos->king_sq[BLACK] = pos->king_sq[BLACK];
	memcpy(copy_pos->board, pos->board, sizeof(int) * 64);
	copy_pos->phase = pos->phase;
	copy_pos->piece_psq_eval[WHITE] = pos->piece_psq_eval[WHITE];
	copy_pos->piece_psq_eval[BLACK] = pos->piece_psq_eval[BLACK];
	copy_pos->state = &copy_pos->hist[pos->state - pos->hist];
	memcpy(copy_pos->state, pos->state, sizeof(struct State));
}

void init_pos(struct Position* pos)
{
	u32 i;
	for (i = 0; i != 64; ++i)
		pos->board[i] = 0;
	for (i = 0; i != 9; ++i)
		pos->bb[i] = 0ULL;
	pos->stm                    = WHITE;
	pos->state                  = pos->hist;
	pos->phase                  = 0;
	pos->state->pos_key         = 0ULL;
	pos->state->pawn_key        = 0ULL;
	pos->state->ep_sq_bb        = 0ULL;
	pos->state->pinned_bb       = 0ULL;
	pos->state->full_moves      = 0;
	pos->state->fifty_moves     = 0;
	pos->state->checkers_bb     = 0ULL;
	pos->state->castling_rights = 0;
	pos->piece_psq_eval[WHITE]  = 0;
	pos->piece_psq_eval[BLACK]  = 0;
}

int set_pos(struct Position* pos, char* fen)
{
	u32 piece, pt, sq, pc,
	    tsq   = 0,
	    index = 0;
	char c;
	pos->state->pos_key = 0ULL;
	pos->state->pawn_key = 0ULL;
	for (sq = 0; sq < 64; ++sq)
		castle_perms[sq] = 15;
	while (tsq < 64) {
		sq = tsq ^ 56;
		c  = fen[index++];
		if (c == ' ') {
			break;
		} else if (c > '0' && c < '9') {
			tsq += (c - '0');
		} else if (c == '/') {
			continue;
		} else {
			piece = get_piece_from_char(c);
			pt = piece & 7;
			pc = piece >> 3;
			put_piece(pos, sq, pt, pc);
			if (pt == KING) {
				pos->king_sq[pc] = sq;
				castle_perms[sq] = pc == WHITE ? 12 : 3;
			} else if (pt == ROOK && !is_frc) {
				if (sq == H1 || sq == H8) {
					castling_rook_pos[pc][KINGSIDE] = sq;
					castle_perms[sq] = pc == WHITE ? 14 : 11;
				} else if (sq == A1 || sq == A8) {
					castling_rook_pos[pc][QUEENSIDE] = sq;
					castle_perms[sq] = pc == WHITE ? 13 : 7;
				}
			}
			++tsq;
		}
	}

	++index;
	pos->stm = fen[index] == 'w' ? WHITE : BLACK;
	index   += 2;
	int file, rank, color;
	while ((c = fen[index++]) != ' ') {
		if (c == '-') {
			++index;
			break;
		} else if (!is_frc) {
			pos->state->castling_rights |= get_cr_from_char(c);
		} else {
			if (c >= 'a' && c <= 'z') {
				color = BLACK;
				rank = RANK_8;
				file = c - 'a';
			} else if (c >= 'A' && c <= 'Z') {
				color = WHITE;
				rank = RANK_1;
				file = c - 'A';
			}
			u32 cr = get_cr_from_char(c);
			if (cr != -1) {
				u64 r_bb = pos->bb[ROOK] & pos->bb[color];
				int ksq = pos->king_sq[color];
				int kside_sq, qside_sq = 0;
				while (r_bb) {
					kside_sq = bitscan(r_bb);
					r_bb &= r_bb - 1;
					if (is_queenside(ksq, kside_sq))
						qside_sq = kside_sq;
					if (qside_sq && kside_sq != qside_sq)
						break;
				}
				if (cr & (WKC | BKC)) {
					castling_rook_pos[color][KINGSIDE] = kside_sq;
					castle_perms[kside_sq] = color == WHITE ? 14 : 11;
					pos->state->castling_rights |= color == WHITE ? WKC : BKC;
				} else if (cr & (WQC | BQC)) {
					castling_rook_pos[color][QUEENSIDE] = qside_sq;
					castle_perms[qside_sq] = color == WHITE ? 13 : 7;
					pos->state->castling_rights |= color == WHITE ? WQC : BQC;
				}
			} else {
				sq = get_sq(rank, file);
				if (is_kingside(pos->king_sq[color], sq)) {
					castling_rook_pos[color][KINGSIDE] = sq;
					castle_perms[sq] = color == WHITE ? 14 : 11;
					pos->state->castling_rights |= color == WHITE ? WKC : BKC;
				} else if (is_queenside(pos->king_sq[color], sq)) {
					castling_rook_pos[color][QUEENSIDE] = sq;
					castle_perms[sq] = color == WHITE ? 13 : 7;
					pos->state->castling_rights |= color == WHITE ? WQC : BQC;
				}
			}
		}
	}
	pos->state->pos_key ^= castle_keys[pos->state->castling_rights];
	u32 ep_sq            = 0;
	if ((c = fen[index++]) != '-') {
		ep_sq = (c - 'a') + ((fen[index++] - '1') << 3);
		pos->state->pos_key ^= psq_keys[0][0][ep_sq];
		pos->state->ep_sq_bb = BB(ep_sq);
	}

	++index;
	u32 x = 0;
	while ((c = fen[index++]) != '\n' && c != ' ' && c != '\0')
		x = x * 10 + (c - '0');
	pos->state->fifty_moves = x;

	x = 0;
	while ((c = fen[index++]) != '\n' && c != ' ' && c != '\0')
		x = x * 10 + (c - '0');
	pos->state->full_moves = x;

	return index;
}

static inline char get_char_from_piece(u32 piece, int c)
{
	char x;
	int pt = piece & 7;
	switch (pt) {
	case PAWN:
		x = 'P';
		break;
	case KNIGHT:
		x = 'N';
		break;
	case BISHOP:
		x = 'B';
		break;
	case ROOK:
		x = 'R';
		break;
	case QUEEN:
		x = 'Q';
		break;
	case KING:
		x = 'K';
		break;
	default:
		return -1;
	}

	if (c == BLACK)
		x += 32;
	return x;
}

void print_board(struct Position* pos)
{
	printf("Board:\n");
	int i, piece;
	for (i = 0; i != 64; ++i) {
		if (i && !(i & 7))
			printf("\n");
		piece = pos->board[i ^ 56];
		if (!piece)
			printf("- ");
		else
			printf("%c ", get_char_from_piece(piece, (BB((i ^ 56)) & pos->bb[WHITE] ? WHITE : BLACK)));
	}
	printf("\n");
	printf("PosKey: %llu\n", pos->state->pos_key);
	printf("PawnKey: %llu\n", pos->state->pawn_key);
}

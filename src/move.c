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
#include "position.h"

void do_null_move(struct Position* const pos)
{
	struct State* const curr = pos->state;
	struct State* const next = ++pos->state;

	pos->stm              ^= 1;
	curr->move             = 0;
	next->full_moves       = curr->full_moves + (pos->stm == BLACK);
	next->fifty_moves      = curr->fifty_moves + 1;
	next->ep_sq_bb         = 0ULL;
	next->castling_rights  = curr->castling_rights;
	next->pos_key          = curr->pos_key ^ stm_key;
	next->pawn_key         = curr->pawn_key;
	if (curr->ep_sq_bb)
		next->pos_key ^= psq_keys[0][0][bitscan(curr->ep_sq_bb)];
}

void undo_null_move(struct Position* const pos)
{
	--pos->state;
	pos->stm ^= 1;
}

void undo_move(struct Position* const pos)
{
	--pos->state;

	pos->stm ^= 1;
	u32 const c    = pos->stm,
	          m    = pos->state->move,
	          from = from_sq(m),
	          to   = to_sq(m),
	          mt   = move_type(m);

	switch (mt) {
	case NORMAL:
		{
			u32 const pt = pos->board[to];
			move_piece_no_key(pos, to, from, pt, c);
			u32 const captured_pt = cap_type(m);
			if (captured_pt)
				put_piece_no_key(pos, to, captured_pt, !c);
			if (pt == KING)
				pos->king_sq[c] = from;
		}
		break;
	case DOUBLE_PUSH:
		{
			move_piece_no_key(pos, to, from, PAWN, c);
		}
		break;
	case ENPASSANT:
		{
			put_piece_no_key(pos, (c == WHITE ? to - 8 : to + 8), PAWN, !c);
			move_piece_no_key(pos, to, from, PAWN, c);
		}
		break;
	case CASTLE:
		{
			int rfrom, rto;
			switch (to) {
			case C1:
				rto = D1;
				rfrom = castling_rook_pos[WHITE][QUEENSIDE];
				break;
			case G1:
				rto = F1;
				rfrom = castling_rook_pos[WHITE][KINGSIDE];
				break;
			case C8:
				rto = D8;
				rfrom = castling_rook_pos[BLACK][QUEENSIDE];
				break;
			case G8:
				rto = F8;
				rfrom = castling_rook_pos[BLACK][KINGSIDE];
				break;
			default:
				rto = rfrom = -1;
				break;
			}
			remove_piece_no_key(pos, rto, ROOK, c);
			remove_piece_no_key(pos, to, KING, c);
			put_piece_no_key(pos, rfrom, ROOK, c);
			put_piece_no_key(pos, from, KING, c);
			pos->king_sq[c] = from;
		}
		break;
	default:
		{
			remove_piece_no_key(pos, to, prom_type(m), c);
			put_piece_no_key(pos, from, PAWN, c);

			u32 const captured_pt = cap_type(m);
			if(captured_pt)
				put_piece_no_key(pos, to, captured_pt, !c);
		}
		break;
	}
}

void do_move(struct Position* const pos, u32 const m)
{
	struct State* const curr = pos->state;
	struct State* const next = ++pos->state;

	curr->move        = m;
	next->full_moves  = curr->full_moves + (pos->stm == BLACK);
	next->fifty_moves = 0;
	next->ep_sq_bb    = 0ULL;

	u32 const from = from_sq(m),
	          to   = to_sq(m),
	          c    = pos->stm,
	          mt   = move_type(m);

	next->pawn_key = curr->pawn_key;
	if (curr->ep_sq_bb)
		next->pos_key = curr->pos_key ^ psq_keys[0][0][bitscan(curr->ep_sq_bb)];
	else
		next->pos_key = curr->pos_key;

	switch (mt) {
	case NORMAL:
		{
			u32 const pt     = pos->board[from];
			u32 const cap_pt = cap_type(m);
			if (!cap_pt) {
				move_piece(pos, from, to, pt, c);
				if (pt != PAWN)
					next->fifty_moves = curr->fifty_moves + 1;
			} else {
				remove_piece(pos, to, pos->board[to], !c);
				move_piece(pos, from, to, pt, c);
			}
			if (pt == KING)
				pos->king_sq[c] = to;
		}
		break;
	case DOUBLE_PUSH:
		{
			move_piece(pos, from, to, PAWN, c);
			if (c == WHITE) {
				next->ep_sq_bb  = BB(from + 8);
				next->pos_key  ^= psq_keys[0][0][from + 8];
			} else if (c == BLACK) {
				next->ep_sq_bb  = BB(from - 8);
				next->pos_key  ^= psq_keys[0][0][from - 8];
			}
			break;
		}
	case ENPASSANT:
		{
			move_piece(pos, from, to, PAWN, c);
			remove_piece(pos, (c == WHITE ? to - 8 : to + 8), PAWN, !c);
		}
		break;
	case CASTLE:
		{
			int rfrom, rto;
			switch (to) {
			case C1:
				rto = D1;
				rfrom = castling_rook_pos[WHITE][QUEENSIDE];
				break;
			case G1:
				rto = F1;
				rfrom = castling_rook_pos[WHITE][KINGSIDE];
				break;
			case C8:
				rto = D8;
				rfrom = castling_rook_pos[BLACK][QUEENSIDE];
				break;
			case G8:
				rto = F8;
				rfrom = castling_rook_pos[BLACK][KINGSIDE];
				break;
			default:
				rto = rfrom = -1;
				break;
			}
			remove_piece(pos, rfrom, ROOK, c);
			remove_piece(pos, from, KING, c);
			put_piece(pos, rto, ROOK, c);
			put_piece(pos, to, KING, c);
			pos->king_sq[c]   = to;
			next->fifty_moves = curr->fifty_moves + 1;
		}
		break;
	default:
		{
			u32 captured_pt = cap_type(m);
			if (captured_pt)
				remove_piece(pos, to, captured_pt, !c);
			remove_piece(pos, from, PAWN, c);
			put_piece(pos, to, prom_type(m), c);
		}
		break;
	}

	pos->stm ^= 1;

	next->castling_rights = (curr->castling_rights & castle_perms[from]) & castle_perms[to];
	next->pos_key        ^=  stm_key
		               ^ castle_keys[curr->castling_rights]
		               ^ castle_keys[next->castling_rights];
}

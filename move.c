#include "defs.h"
#include "position.h"

void undo_move(Position* const pos, u32* m)
{
	--pos->ply;
	--pos->state;

	pos->stm ^= 1;
	u32 const c    = pos->stm,
	          from = from_sq(*m),
	          to   = to_sq(*m),
	          mt   = move_type(*m);

	switch (mt) {
	case NORMAL:
		{
			u32 const pt = piece_type(pos->board[to]);
			move_piece_no_key(pos, to, from, pt, c);
			u32 const captured_pt = cap_type(*m);
			if(captured_pt)
				put_piece_no_key(pos, to, captured_pt, !c);
			if(pt == KING)
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
			move_piece_no_key(pos, to, from, KING, c);
			pos->king_sq[c] = from;

			switch(to) {
			case C1:
				move_piece_no_key(pos, D1, A1, ROOK, c);
				break;
			case G1:
				move_piece_no_key(pos, F1, H1, ROOK, c);
				break;
			case C8:
				move_piece_no_key(pos, D8, A8, ROOK, c);
				break;
			case G8:
				move_piece_no_key(pos, F8, H8, ROOK, c);
				break;
			default:
				break;
			}
		}
		break;
	default:
		{
			remove_piece_no_key(pos, to, prom_type(*m), c);
			put_piece_no_key(pos, from, PAWN, c);

			u32 const captured_pt = cap_type(*m);
			if(captured_pt)
				put_piece_no_key(pos, to, captured_pt, !c);
		}
		break;
	}
}

u32 do_move(Position* const pos, u32* m)
{
	static u32 const castle_perms[64] = {
		13, 15, 15, 15, 12, 15, 15, 14,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		7, 15, 15, 15, 3,  15, 15, 11
	};

	State** const tmp        = &pos->state;
	State const * const curr = *tmp;
	State* const next        = ++*tmp;

	next->piece_psq_eval[WHITE] = curr->piece_psq_eval[WHITE];
	next->piece_psq_eval[BLACK] = curr->piece_psq_eval[BLACK];
	next->pos_key               = curr->pos_key;
	next->fifty_moves           = curr->fifty_moves + 1;
	next->ep_sq_bb              = 0ULL;

	u32 check_illegal = 0;
	u32 const from    = from_sq(*m),
	          to      = to_sq(*m),
	          c       = pos->stm,
	          mt      = move_type(*m);

	if (curr->ep_sq_bb)
		next->pos_key ^= psq_keys[0][0][bitscan(curr->ep_sq_bb)];

	switch (mt) {
	case NORMAL:
		{
			u32 const pt = piece_type(pos->board[from]);
			if (!pos->board[to]) {
				move_piece(pos, from, to, pt, c);
			} else {
				u32 const captured_pt = piece_type(pos->board[to]);
				remove_piece(pos, to, captured_pt, !c);
				move_piece(pos, from, to, pt, c);
				*m |= captured_pt << CAP_TYPE_SHIFT;
				next->fifty_moves = 0;
			}
			if (pt == KING) {
				pos->king_sq[c] = to;
				check_illegal = 1;
			}
		}
		break;
	case DOUBLE_PUSH:
		{
			move_piece(pos, from, to, PAWN, c);
			next->fifty_moves = 0;
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
			next->fifty_moves = 0;
			check_illegal     = 1;
		}
		break;
	case CASTLE:
		{
			move_piece(pos, from, to, KING, c);
			pos->king_sq[c] = to;

			switch (to) {
			case C1:
				move_piece(pos, A1, D1, ROOK, c);
				break;
			case G1:
				move_piece(pos, H1, F1, ROOK, c);
				break;
			case C8:
				move_piece(pos, A8, D8, ROOK, c);
				break;
			case G8:
				move_piece(pos, H8, F8, ROOK, c);
				break;
			default:
				break;
			}
		}
		break;
	default:
		{
			u32 captured_pt = pos->board[to];
			if (captured_pt) {
				captured_pt = piece_type(captured_pt);
				remove_piece(pos, to, captured_pt, !c);
				*m |= captured_pt << CAP_TYPE_SHIFT;
				next->fifty_moves = 0;
			}
			remove_piece(pos, from, PAWN, c);
			put_piece(pos, to, prom_type(*m), c);
		}
		break;
	}


	++pos->ply;
	pos->stm ^= 1;

	next->castling_rights =  (curr->castling_rights & castle_perms[from]) & castle_perms[to];
	next->pos_key        ^=   stm_key
		                ^ castle_keys[curr->castling_rights]
		                ^ castle_keys[next->castling_rights];

	if ( (check_illegal || (BB(from) & curr->pinned_bb) > 0)
	   && checkers(pos, pos->stm)) {
		undo_move(pos, m);
		return 0;
	}

	return 1;
}

u32 do_usermove(Position* const pos, u32 move) {
	if (!do_move(pos, &move))
		return 0;
	pos->ply = 0;
	++pos->hist_size;
	return 1;
}

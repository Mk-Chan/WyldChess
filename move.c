#include "defs.h"
#include "position.h"

const static u32 castle_perms[64] = {
  13, 15, 15, 15, 12, 15, 15, 14,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
   7, 15, 15, 15, 3,  15, 15, 11
};

void undo_move(Position* pos, u32* m) {
  --pos->ply;
  --pos->state;

  pos->stm ^= 1;
  const u32 c    = pos->stm,
            from = from_sq(*m),
            to   = to_sq(*m),
            mt   = move_type(*m);

  switch(mt) {
  case NORMAL:
    {
      const u32 pt = piece_type(pos->board[to]);
      move_piece_no_key(pos, to, from, pt, c);
      const u32 captured_pt = cap_type(*m);
      if(pt == KING)
        pos->king_sq[c] = from;
      if(captured_pt)
        put_piece_no_key(pos, to, captured_pt, !c);
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

      const u32 captured_pt = cap_type(*m);
      if(captured_pt)
        put_piece_no_key(pos, to, captured_pt, !c);
    }
    break;
  }
}

u32 do_move(Position* pos, u32* m) {
  State** const tmp        = &pos->state;
  State const * const curr = *tmp;
  State* const next        = ++*tmp;

  next->pos_key     = curr->pos_key;
  next->fifty_moves = curr->fifty_moves + 1;
  next->ep_sq_bb    = 0ULL;

  u32 check_illegal = 0;
  const u32 from = from_sq(*m),
            to = to_sq(*m),
            c  = pos->stm,
            mt = move_type(*m);

  if(curr->ep_sq_bb)
    next->pos_key ^= psq_keys[0][0][bitscan(curr->ep_sq_bb)];

  switch(mt) {
  case NORMAL:
    {
      const u32 pt          = piece_type(pos->board[from]),
                captured_pt = piece_type(pos->board[to]);
      if(captured_pt) {
        remove_piece(pos, to, captured_pt, !c);
        *m |= captured_pt << CAP_TYPE_SHIFT;
        move_piece(pos, from, to, pt, c);
        next->fifty_moves = 0;
        goto next;
      }
      else if(pt == PAWN) {
        if(c == WHITE && to == from + 16) {
          next->ep_sq_bb  = BB(from + 8);
          next->pos_key  ^= psq_keys[0][0][from + 8];
        }
        else if(c == BLACK && to == from - 16) {
          next->ep_sq_bb  = BB(from - 8);
          next->pos_key  ^= psq_keys[0][0][from - 8];
        }

        next->fifty_moves = 0;
      }

      move_piece(pos, from, to, pt, c);
next:
      if(pt == KING) {
        pos->king_sq[c] = to;
        check_illegal = 1;
      }
    }
    break;
  case ENPASSANT:
    {
      move_piece(pos, from, to, PAWN, c);
      remove_piece(pos, (c == WHITE ? to - 8 : to + 8), PAWN, !c);
      next->fifty_moves = 0;
      check_illegal = 1;
    }
    break;
  case CASTLE:
    {
      move_piece(pos, from, to, KING, c);
      pos->king_sq[c] = to;

      switch(to) {
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
      const u32 captured_pt = piece_type(pos->board[to]);
      if(captured_pt) {
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

  next->pos_key ^= castle_keys[curr->castling_rights];
  next->castling_rights = (curr->castling_rights & castle_perms[from]) & castle_perms[to];
  next->pos_key ^= castle_keys[next->castling_rights];
  next->pos_key ^= stm_key;

  if(    (check_illegal || (BB(from) & curr->pinned_bb) > 0)
      && checkers(pos, pos->stm)) {
    undo_move(pos, m);
    return 0;
  }

  return 1;
}

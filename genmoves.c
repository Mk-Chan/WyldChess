#include "defs.h"
#include "position.h"
#include "magicmoves.h"

static inline void add_move(u32 m, Movelist* list) {
  *list->end = m;
  ++list->end;
}

void extract_moves(u32 from, u64 atks, Movelist* list) {
  u32 to;
  while(atks) {
    to = bitscan(atks);
    add_move(move_normal(from, to), list);
    atks &= atks - 1;
  }
}

void gen_check_blocks(Position* pos, u64 blocking_poss_bb, Movelist* list) {
  const u32 c = pos->stm;
  u32 blocking_sq, blocker;
  u64 blockers_poss, pawn_block_poss,
      pawns_bb            = pos->bb[PAWN] & pos->bb[c];
  const u64 inlcusion_mask = ~(pawns_bb | pos->bb[KING] | pos->state->pinned_bb),
            vacancy_mask   = ~pos->bb[FULL];
  while(blocking_poss_bb) {
    blocking_sq       = bitscan(blocking_poss_bb);
    blocking_poss_bb &= blocking_poss_bb - 1;

    pawn_block_poss   = pawn_shift(BB(blocking_sq), !c);
    if(pawn_block_poss & pawns_bb) {
      blocker = bitscan(pawn_block_poss);
      if(blocking_sq < 8 || blocking_sq > 55) {
        add_move(move_prom(blocker, blocking_sq, TO_QUEEN), list);
        add_move(move_prom(blocker, blocking_sq, TO_KNIGHT), list);

        add_move(move_prom(blocker, blocking_sq, TO_ROOK), list);
        add_move(move_prom(blocker, blocking_sq, TO_BISHOP), list);
      }
      else
        add_move(move_normal(blocker, blocking_sq), list);
    }
    else if((( c == WHITE && rank_of(blocking_sq) == RANK_4)
            || (c == BLACK && rank_of(blocking_sq) == RANK_5))
              && (pawn_block_poss & vacancy_mask)
              && (pawn_block_poss = pawn_shift(pawn_block_poss, !c) & pawns_bb))
      add_move(move_normal(bitscan(pawn_block_poss), blocking_sq), list);

    blockers_poss = atkers_to_sq(pos, blocking_sq, c) & inlcusion_mask;
    while(blockers_poss) {
      add_move(move_normal(bitscan(blockers_poss), blocking_sq), list);
      blockers_poss &= blockers_poss - 1;
    }
  }
}

void gen_checker_caps(Position* pos, u64 checkers_bb, Movelist* list) {
  const u32 c = pos->stm;
  u32 checker, atker;
  u64 atkers_bb;
  const u64 pawns_bb      = pos->bb[PAWN] & pos->bb[c],
            non_king_mask = ~pos->bb[KING],
            ep_sq_bb      = pos->state->ep_sq_bb;
  if(    ep_sq_bb
      && (pawn_shift(ep_sq_bb, !c) & checkers_bb)) {
    const u32 ep_sq = bitscan(ep_sq_bb);
    u64 ep_poss     = pawns_bb & p_atks[!c][ep_sq];
    while(ep_poss) {
      atker    = bitscan(ep_poss);
      ep_poss &= ep_poss - 1;
      add_move(move_ep(atker, ep_sq), list);
    }
  }
  while(checkers_bb) {
    checker      = bitscan(checkers_bb);
    checkers_bb &= checkers_bb - 1;
    atkers_bb    = atkers_to_sq(pos, checker, c) & non_king_mask;
    while(atkers_bb) {
      atker      = bitscan(atkers_bb);
      atkers_bb &= atkers_bb - 1;
      if(   (BB(atker) & pawns_bb)
          && (checker < 8 || checker > 55)) {
        add_move(move_prom(atker, checker, TO_QUEEN), list);
        add_move(move_prom(atker, checker, TO_KNIGHT), list);

        add_move(move_prom(atker, checker, TO_ROOK), list);
        add_move(move_prom(atker, checker, TO_BISHOP), list);
      }
      else
        add_move(move_normal(atker, checker), list);
    }
  }
}

void gen_check_evasions(Position* pos, Movelist* list) {
  const u32 c     = pos->stm,
        ksq       = pos->king_sq[c];
  const u64 k_bb  = BB(ksq);
  u64 checkers_bb = pos->state->checkers_bb;
  u64 evasions_bb = k_atks[ksq] & ~pos->bb[c];

  u32 sq;
  pos->bb[FULL] ^= k_bb;
  while(evasions_bb) {
    sq = bitscan(evasions_bb);
    evasions_bb &= evasions_bb - 1;
    if(!atkers_to_sq(pos, sq, !c))
      add_move(move_normal(ksq, sq), list);
  }
  pos->bb[FULL] ^= k_bb;

  if(checkers_bb & (checkers_bb - 1))
    return;

  gen_checker_caps(pos, checkers_bb, list);

  if(checkers_bb & k_atks[ksq])
    return;

  const u64 blocking_poss_bb = intervening_sqs[bitscan(checkers_bb)][ksq];
  if(blocking_poss_bb)
    gen_check_blocks(pos, blocking_poss_bb, list);
}

void gen_pawn_captures(Position* pos, Movelist* list) {
  const u32 c = pos->stm;
  u32 from, to;
  u64 cap_candidates,
      pawns_bb = pos->bb[PAWN] & pos->bb[c];
  if(pos->state->ep_sq_bb) {
    const u32 ep_sq = bitscan(pos->state->ep_sq_bb);
    u64 ep_poss     = pawns_bb & p_atks[!c][ep_sq];
    while(ep_poss) {
      from     = bitscan(ep_poss);
      ep_poss &= ep_poss - 1;
      add_move(move_ep(from, ep_sq), list);
    }
  }
  while(pawns_bb) {
    from      = bitscan(pawns_bb);
    pawns_bb &= pawns_bb - 1;

    cap_candidates = p_atks[c][from] & pos->bb[!c];
    while(cap_candidates) {
      to              = bitscan(cap_candidates);
      cap_candidates &= cap_candidates - 1;
      if(to < 8 || to > 55) {
        add_move(move_prom(from, to, TO_QUEEN), list);
        add_move(move_prom(from, to, TO_KNIGHT), list);

        add_move(move_prom(from, to, TO_ROOK), list);
        add_move(move_prom(from, to, TO_BISHOP), list);
      }
      else
        add_move(move_normal(from, to), list);
    }
  }
}

void gen_captures(Position* pos, Movelist* list) {
  u32 from, pt;
  const u32 c = pos->stm;
  u64 curr_piece_bb;
  gen_pawn_captures(pos, list);
  for(pt = KNIGHT; pt != KING; ++pt) {
    curr_piece_bb = pos->bb[pt] & pos->bb[c];
    while(curr_piece_bb) {
      from           = bitscan(curr_piece_bb);
      curr_piece_bb &= curr_piece_bb - 1;
      extract_moves(
          from,
          get_atks(from, pt, c, pos->bb[FULL]) & pos->bb[!c],
          list
      );
    }
  }
  from = pos->king_sq[c];
  extract_moves(from, k_atks[from] & pos->bb[!c], list);
}

void gen_castling(Position* pos, Movelist* list) {
  static const u32 castling_poss[2][2] = {
    { WKC, WQC },
    { BKC, BQC }
  };
  static const u32 castling_intermediate_sqs[2][2][2] = {
    { { F1, G1 }, { D1, C1 } },
    { { F8, G8 }, { D8, C8 } }
  };
  static const u32 castling_king_sqs[2][2][2] = {
    { { E1, G1 }, { E1, C1 } },
    { { E8, G8 }, { E8, C8 } }
  };
  static const u64 castle_mask[2][2] = {
    { (BB(F1) | BB(G1)), (BB(D1) | BB(C1) | BB(B1)) },
    { (BB(F8) | BB(G8)), (BB(D8) | BB(C8) | BB(B8)) }
  };

  const u32 c = pos->stm;

  if(    (castling_poss[c][0] & pos->state->castling_rights)
      && !(castle_mask[c][0] & pos->bb[FULL])
      && !(atkers_to_sq(pos, castling_intermediate_sqs[c][0][0], !c))
      && !(atkers_to_sq(pos, castling_intermediate_sqs[c][0][1], !c)))
    add_move(move_castle(castling_king_sqs[c][0][0], castling_king_sqs[c][0][1]), list);

  if(    (castling_poss[c][1] & pos->state->castling_rights)
      && !(castle_mask[c][1] & pos->bb[FULL])
      && !(atkers_to_sq(pos, castling_intermediate_sqs[c][1][0], !c))
      && !(atkers_to_sq(pos, castling_intermediate_sqs[c][1][1], !c)))
    add_move(move_castle(castling_king_sqs[c][1][0], castling_king_sqs[c][1][1]), list);
}

void gen_pawn_quiets(Position* pos, Movelist* list) {
  const u32 c = pos->stm;
  u32 single_push, from;
  const u64 vacancy_mask = ~pos->bb[FULL];
  u64 pawns_bb           = pos->bb[PAWN] & pos->bb[c];
  while(pawns_bb) {
    from        = bitscan(pawns_bb);
    pawns_bb   &= pawns_bb - 1;
    single_push = (c == WHITE ? from + 8 : from - 8);
    if(BB(single_push) & vacancy_mask) {
      if(single_push < 8 || single_push > 55) {
        add_move(move_prom(from, single_push, TO_QUEEN), list);
        add_move(move_prom(from, single_push, TO_KNIGHT), list);

        add_move(move_prom(from, single_push, TO_ROOK), list);
        add_move(move_prom(from, single_push, TO_BISHOP), list);
      }
      else {
        add_move(move_normal(from, single_push), list);
        if(    (c == WHITE && rank_of(from) == RANK_2)
            || (c == BLACK && rank_of(from) == RANK_7)) {
          const u32 double_push = (c == WHITE ? single_push + 8 : single_push - 8);
          if(BB(double_push) & vacancy_mask)
            add_move(move_normal(from, double_push), list);
        }
      }
    }
  }
}

void gen_quiets(Position* pos, Movelist* list) {
  u32 from, pt;
  const u32 c = pos->stm;
  u64 curr_piece_bb;
  gen_pawn_quiets(pos, list);
  for(pt = KNIGHT; pt != KING; ++pt) {
    curr_piece_bb = pos->bb[pt] & pos->bb[c];
    while(curr_piece_bb) {
      from           = bitscan(curr_piece_bb);
      curr_piece_bb &= curr_piece_bb - 1;
      extract_moves(
          from,
          get_atks(from, pt, c, pos->bb[FULL]) & ~pos->bb[FULL],
          list
      );
    }
  }
  from = pos->king_sq[c];
  extract_moves(from, k_atks[from] & ~pos->bb[FULL], list);
  gen_castling(pos, list);
}

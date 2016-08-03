#ifndef POSITION_H
#define POSITION_H

#include "defs.h"
#include "random.h"
#include "bitboard.h"
#include "magicmoves.h"

typedef struct Stats_s {

  // Hash table stats
  u64 hash_stores;
  u64 hash_probes;
  u64 hash_hits;

  // Search tree stats
  u64 total_nodes;

} Stats;

typedef struct Movelist_s {
  
  u32  moves[218];
  u32* end;

} Movelist;

typedef struct State_s {

  u32      castling_rights;
  u32      fifty_moves;
  u64      pinned_bb;
  u64      checkers_bb;
  u64      ep_sq_bb;
  HashKey  pos_key;
  Movelist list;

#ifdef STATS
  Stats    stats;
#endif

} State;

typedef struct Position_s {

  u32      ply;
  u32      stm;
  u64      bb[10];
  u32      king_sq[2];
  u32      board[64];
  State*   state;
  State    state_list[MAX_PLY];

} Position;

inline void move_piece_no_key(Position* pos, u32 from, u32 to, u32 pt, u32 c) {
  u64 from_to          = BB(from) ^ BB(to);
  pos->bb[FULL]       ^= from_to;
  pos->bb[c]          ^= from_to;
  pos->bb[pt]         ^= from_to;
  pos->board[to]       = pos->board[from];
  pos->board[from]     = 0;
}

inline void put_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c) {
  u64 set              = BB(sq);
  pos->bb[FULL]       |= set;
  pos->bb[c]          |= set;
  pos->bb[pt]         |= set;
  pos->board[sq]       = make_piece(pt, c);
}

inline void remove_piece_no_key(Position* pos, u32 sq, u32 pt, u32 c) {
  u64 clr              = BB(sq);
  pos->bb[FULL]       ^= clr;
  pos->bb[c]          ^= clr;
  pos->bb[pt]         ^= clr;
  pos->board[sq]       = 0;
}

inline void move_piece(Position* pos, u32 from, u32 to, u32 pt, u32 c) {
  u64 from_to          = BB(from) ^ BB(to);
  pos->bb[FULL]       ^= from_to;
  pos->bb[c]          ^= from_to;
  pos->bb[pt]         ^= from_to;
  pos->board[to]       = pos->board[from];
  pos->board[from]     = 0;
  pos->state->pos_key ^= psq_keys[c][pt][from] ^ psq_keys[c][pt][to];
}

inline void put_piece(Position* pos, u32 sq, u32 pt, u32 c) {
  u64 set              = BB(sq);
  pos->bb[FULL]       |= set;
  pos->bb[c]          |= set;
  pos->bb[pt]         |= set;
  pos->board[sq]       = make_piece(pt, c);
  pos->state->pos_key ^= psq_keys[c][pt][sq];
}

inline void remove_piece(Position* pos, u32 sq, u32 pt, u32 c) {
  u64 clr              = BB(sq);
  pos->bb[FULL]       ^= clr;
  pos->bb[c]          ^= clr;
  pos->bb[pt]         ^= clr;
  pos->board[sq]       = 0;
  pos->state->pos_key ^= psq_keys[c][pt][sq];
}

inline u64 pawn_shift(u64 bb, u32 c) {
  return (c == WHITE ? bb << 8 : bb >> 8);
}

inline u64 pawn_double_shift(u64 bb, u32 c) {
  return (c == WHITE ? bb << 16 : bb >> 16);
}

inline void set_pinned(Position* pos, u32 to_color) {
  const int ksq = pos->king_sq[to_color];
  u64* pinned_bb = &pos->state->pinned_bb;
  *pinned_bb= 0ULL;

  u32 sq;
  u64 bb,
      pinners_bb = ((pos->bb[ROOK] | pos->bb[QUEEN]) 
                   & pos->bb[!to_color]
                   & r_pseudo_atks[ksq])
                 | ((pos->bb[BISHOP] | pos->bb[QUEEN])
                   & pos->bb[!to_color]
                   & b_pseudo_atks[ksq]);
  while(pinners_bb) {
    sq = bitscan(pinners_bb);
    pinners_bb &= pinners_bb - 1;
    bb = intervening_sqs[sq][ksq] & pos->bb[FULL];
    if(!(bb & (bb - 1)))
      *pinned_bb ^= bb & pos->bb[to_color];
  }
}

inline u64 get_atks(u32 sq, u32 pt, u32 c, u64 occupancy) {
  switch(pt) {
  case PAWN:   return p_atks[c][sq];
  case KNIGHT: return n_atks[sq];
  case BISHOP: return Bmagic(sq, occupancy);
  case ROOK:   return Rmagic(sq, occupancy);
  case QUEEN:  return Qmagic(sq, occupancy);
  case KING:   return k_atks[sq];
  default:     return -1;
  }
}

inline u64 atkers_to_sq(Position* pos, u32 sq, u32 by_color) {
  return ((pos->bb[KNIGHT] & pos->bb[by_color] & n_atks[sq])
        | (pos->bb[PAWN] & pos->bb[by_color] & p_atks[!by_color][sq])
        | ((pos->bb[ROOK] | pos->bb[QUEEN]) & pos->bb[by_color] & Rmagic(sq, pos->bb[FULL]))
        | ((pos->bb[BISHOP] | pos->bb[QUEEN]) & pos->bb[by_color] & Bmagic(sq, pos->bb[FULL]))
        | (pos->bb[KING] & pos->bb[by_color] & k_atks[sq]));
}

inline u64 checkers(Position* pos, u32 by_color) {
  const u32 sq = pos->king_sq[!by_color];
  return ((pos->bb[KNIGHT] & pos->bb[by_color] & n_atks[sq])
       | (pos->bb[PAWN] & pos->bb[by_color] & p_atks[!by_color][sq])
       | ((pos->bb[ROOK] | pos->bb[QUEEN]) & pos->bb[by_color] & Rmagic(sq, pos->bb[FULL]))
       | ((pos->bb[BISHOP] | pos->bb[QUEEN]) & pos->bb[by_color] & Bmagic(sq, pos->bb[FULL]))
       | (pos->bb[KING] & pos->bb[by_color] & k_atks[sq]));
}

inline void move_str(u32 move, char str[5]) {
  u32 from = from_sq(move),
      to   = to_sq(move);
  str[0] = file_of(from) + 'a';
  str[1] = rank_of(from) + '1';
  str[2] = file_of(to) + 'a';
  str[3] = rank_of(to) + '1';
  if(move_type(move) == PROMOTION) {
    const u32 prom = prom_type(move);
    switch(prom) {
    case QUEEN:
      str[4] = 'q';
      break;
    case KNIGHT:
      str[4] = 'n';
      break;
    case BISHOP:
      str[4] = 'b';
      break;
    case ROOK:
      str[4] = 'r';
      break;
    }
  }
  else
    str[4] = 0;
}

extern u32 initial_stm;
extern void print_board(Position* pos);

extern void init_pos(Position* pos);
extern void set_pos(Position* pos, char* fen);

extern u32  do_move(Position* pos, u32* m);
extern void undo_move(Position* pos, u32* m);

extern void gen_quiets(Position* pos, Movelist* list);
extern void gen_captures(Position* pos, Movelist* list);

extern void gen_check_evasions(Position* pos, Movelist* list);

extern u64 perft(Position* pos, u32 depth);

#endif

#include <stdio.h>
#include "position.h"
#include "random.h"

static inline u32 get_piece_from_char(char c) {
  switch(c) {
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

static inline u32 get_cr_from_char(char c) {
  switch(c) {
  case 'K': return WKC;
  case 'Q': return WQC;
  case 'k': return BKC;
  case 'q': return BQC;
  default : return -1;
  }
}

void init_pos(Position* pos) {
  u32 i;
  for(i = 0; i != 64; ++i)
    pos->board[i] = 0;
  for(i = 0; i != 10; ++i)
    pos->bb[i] = 0ULL;
  pos->ply                    = 0;
  pos->stm                    = WHITE;
  pos->state                  = pos->state_list;
  pos->state->pos_key         = 0ULL;
  pos->state->pinned_bb       = 0ULL;
  pos->state->fifty_moves     = 0;
  pos->state->castling_rights = 0;
  pos->state->ep_sq_bb        = 0ULL;
}

void set_pos(Position* pos, char* fen) {
  init_pos(pos);
  u32 piece, pt, sq, pc,
      tsq = 0,
      index = 0;
  char c;
  while(tsq < 64) {
    sq = tsq ^ 56;
    c = fen[index++];
    if(c == ' ')
      break;
    else if(c > '0' && c < '9')
      tsq += (c - '0');
    else if(c == '/')
      continue;
    else {
      piece = get_piece_from_char(c);
      pt = piece_type(piece);
      pc = piece_color(piece);
      put_piece(pos, sq, pt, pc);
      if(pt == KING)
        pos->king_sq[pc] = sq;
      ++tsq;
    }
  }

  ++index;
  pos->stm = fen[index] == 'w' ? WHITE : BLACK;
  index += 2;
  while((c = fen[index++]) != ' ') {
    if(c == '-') {
      ++index;
      break;
    }
    else
      pos->state->castling_rights |= get_cr_from_char(c);
  }
  pos->state->pos_key ^= castle_keys[pos->state->castling_rights];
  u32 ep_sq = 0;
  if((c = fen[index++]) != '-') {
    ep_sq = (c - 'a') + ((fen[index++] - '1') << 3);
    pos->state->pos_key ^= psq_keys[0][0][ep_sq];
    pos->state->ep_sq_bb = BB(ep_sq);
  }

  ++index;
  u32 x = 0;
  while((c = fen[index++] != '\0' && c != ' '))
    x = x * 10 + (c - '0');
  pos->state->fifty_moves = x;

  // Ignoring full game moves for now

  set_pinned(pos, pos->stm);
  pos->state->checkers_bb = checkers(pos, !pos->stm);
}

static inline char get_char_from_piece(int piece) {
  char x;
  int pt = piece_type(piece);
  switch(pt) {
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
  }

  if(piece_color(piece) == BLACK)
    x += 32;
  return x;
}

void print_board(Position* pos) {
  printf("Board:\n");
  int i, piece;
  for(i = 0; i != 64; ++i) {
    if(i && !(i & 7))
      printf("\n");
    piece = pos->board[i ^ 56];
    if(!piece)
      printf("- ");
    else
      printf("%c ", get_char_from_piece(piece));
  }
  printf("\n");
}

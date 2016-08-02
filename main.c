#include <stdio.h>
#include <time.h>
#include "defs.h"
#include "bitboard.h"
#include "magicmoves.h"
#include "position.h"
#include "random.h"

char* fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char* fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
char* fen3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
char* fen4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
char* fen5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
char* fen6 = "8/8/8/K7/1R3p1k/8/6P1/8 w - - 1 1";
char* fen7 = "8/8/8/K7/7k/1R3p2/6P1/8 w - - 1 1";

int main() {
  // Initialization
  init_genrand64(time(0));
  init_zobrist_keys();
  initmagicmoves();
  init_atks();
  init_intervening_sqs();

  Position pos;
  set_pos(&pos, fen1);
  print_board(&pos);

  u64 count;
  u32 d;
  for(d = 1; d <= 6; ++d) {
    count = perft(&pos, d);
    printf("perft(%d) = %10llu\n", d, count);
  }
  /*printf("cr=%d\n", pos.state->castling_rights);
  printf("stm=%d\n", pos.stm);
  u32 move = move_normal(E2, E4);
  do_move(&pos, &move);
  move = move_normal(F7, F5);
  do_move(&pos, &move);
  move = move_normal(E4, F5);
  do_move(&pos, &move);
  undo_move(&pos, &move);
  for(int i = 0; i != 10; ++i)
    print_bb(pos.bb[i]);
  printf("ep=%d\n", pos.state->ep_sq);
  printf("cr=%d\n", pos.state->castling_rights);

  Movelist list;
  list.end = list.moves;
  gen_quiets(&pos, &list);
  gen_captures(&pos, &list);
  for(u32* move = list.moves; move != list.end; ++move)
    printf("from=%d, to=%d\n", from_sq(*move), to_sq(*move));*/

  /*u128 x = rand_u128();
  for(int i = 0; i != 128; ++i) {
    if(i && !(i & 15))
      printf("\n");
    if(x & ((u128)1 << i))
      printf("X ");
    else
      printf("- ");
  }
  printf("\n");*/

  return 0;
}

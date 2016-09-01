#include <stdio.h>
#include <time.h>
#include "defs.h"
#include "bitboard.h"
#include "magicmoves.h"
#include "position.h"
#include "random.h"

char* fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
char* fen2 = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

int main() {
  init_genrand64(time(0));
  init_zobrist_keys();
  initmagicmoves();
  init_atks();
  init_intervening_sqs();

  Position pos;
  set_pos(&pos, fen1);

  u64 count;
  u32 d;
  for(d = 1; d <= 6; ++d) {
    count = perft(&pos, d);
    printf("perft(%d) = %10llu\n", d, count);
  }

  return 0;
}

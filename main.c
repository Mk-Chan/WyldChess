#include <time.h>
#include "magicmoves.h"
#include "position.h"
#include "random.h"
#include "tt.h"
#include "engine.h"

int main()
{
	init_genrand64(time(0));
	init_zobrist_keys();
	initmagicmoves();
	init_atks();
	init_intervening_sqs();
	tt_init(&tt, 100000);
	cecp_loop();
	tt_destroy(&tt);
	/*Position pos;
	init_pos(&pos);
	set_pos(&pos, fen1);
	u32 move = move_normal(D2, D4);
	do_usermove(&pos, move);
	move = move_normal(D7, D5);
	do_usermove(&pos, move);
	move = move_normal(B1, C3);
	do_usermove(&pos, move);
	move = move_normal(B8, C6);
	do_usermove(&pos, move);
	printf("Searching:\n");
	print_board(&pos);
	begin_search(&pos);
	printf("Done!\n");*/
	//printf("Eval = %d\n", evaluate(&pos));
	/*u64 count;
	u32 d;
	for (d = 1; d <= 6; ++d) {
	  count = perft(&pos, d);
	  printf("perft(%d) = %10llu\n", d, count);
	}*/
	return 0;
}

#include <time.h>
#include "magicmoves.h"
#include "position.h"
#include "random.h"
#include "tt.h"
#include "engine.h"
#include "timer.h"

int main()
{
	init_timer();
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	init_genrand64(time(0));
	init_zobrist_keys();
	initmagicmoves();
	init_atks();
	init_intervening_sqs();
	tt_init(&tt, 1000000);
	fprintf(stdout, "WyldChess started!\n");
	cecp_loop();
	tt_destroy(&tt);
	/*Position pos;
	init_pos(&pos);
	set_pos(&pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	printf("%llu\n", perft(&pos, 6));*/
	return 0;
}

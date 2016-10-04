#include <time.h>
#include "magicmoves.h"
#include "position.h"
#include "random.h"
#include "tt.h"
#include "engine.h"
#include "timer.h"

int main()
{
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	init_timer();
	init_genrand64(time(0));
	init_zobrist_keys();
	initmagicmoves();
	init_atks();
	init_intervening_sqs();
	init_masks();
	tt_init(&tt, 1000000);
	fprintf(stdout, "WyldChess started!\n");
	cecp_loop();
	tt_destroy(&tt);
	return 0;
}

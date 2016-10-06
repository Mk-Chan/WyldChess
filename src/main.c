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

	char input[8];
	fgets(input, 8, stdin);
	if (!strncmp(input, "xboard", 6))
		cecp_loop();
	else
		fprintf(stdout, "Protocol not found!\n");

	tt_destroy(&tt);
	return 0;
}

#include "defs.h"
#include "eval.h"
#include "position.h"

int piece_val[7] = {
	0,
	0,
	100,
	300,
	310,
	500,
	900
};

int evaluate(Position* const pos)
{
	return pos->state->piece_psq_eval;
}

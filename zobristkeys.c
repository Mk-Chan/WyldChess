#include "random.h"

HashKey psq_keys[2][8][64];
HashKey castle_keys[16];
HashKey stm_key;

void init_zobrist_keys() {
	int i, j, k;
	for (i = 0; i != 2; ++i)
		for (j = 0; j != 8; ++j)
			for (k = 0; k != 64; ++k)
				psq_keys[i][j][k] = rng();
	for (i = 0; i != 16; ++i)
		castle_keys[i] = rng();
	stm_key = rng();
}

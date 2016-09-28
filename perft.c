#include <stdio.h>
#include "defs.h"
#include "position.h"
#include "timer.h"

typedef struct perft_table_s {

	u32     depth;
	u64     nodes;
	HashKey key;

} perft_entry;

#define PFT_SIZE (0x100000)
perft_entry pft[PFT_SIZE];

static inline perft_entry* probe(State* const s)
{
	u32 index = s->pos_key < PFT_SIZE ? s->pos_key : s->pos_key % PFT_SIZE;
	return pft + index;
}

static inline void store(State* const s, u64 nodes, u32 depth)
{
	perft_entry* pe = probe(s);
	pe->depth       = depth;
	pe->nodes       = nodes;
	pe->key         = s->pos_key;
}

u64 total, hits, stores;

u64 perft(Position* const pos, u32 depth)
{
	u32 use_pft = 0 && (depth > 1 && pos->ply >= 3);
	if (use_pft && !pos->ply) {
		total  = 0ULL;
		hits   = 0ULL;
		stores = 0ULL;
		perft_entry* pe;
		u32 x;
		for(x = 0; x != PFT_SIZE; ++x) {
			pe = pft + x;
			pe->key = 0;
			pe->depth = 0;
			pe->nodes = 0;
		}
	}
	if (use_pft) {
		perft_entry* pe = probe(pos->state);
		++total;
		if (pe->key == pos->state->pos_key
		       && pe->depth == depth) {
			++hits;
			return pe->nodes;
		}
	}

	Movelist* list = pos->list + pos->ply;
	list->end = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	}
	else {
		gen_quiets(pos, list);
		gen_captures(pos, list);
	}

	u64 count = 0ULL;
	Move* move;
	if (depth == 1) {
		for(move = list->moves; move < list->end; ++move) {
			if(!do_move(pos, *move)) continue;
			undo_move(pos, *move);
			++count;
		}
	} else {
		for(move = list->moves; move < list->end; ++move) {
			if(!do_move(pos, *move)) continue;
			count += perft(pos, depth - 1);
			undo_move(pos, *move);
		}
	}

	if (use_pft) {
		store(pos->state, count, depth);
		++stores;
	}

	/*if(!pos->ply) {
	  printf("stores=%llu, probes=%llu, hits=%llu\n", stores, total, hits);
	  }*/

	return count;
}

void performance_test(Position* const pos, u32 max_depth)
{
	u32 depth;
	u64 count;
	u64 t1, t2;
	for (depth = 1; depth <= max_depth; ++depth) {
		t1 = curr_time();
		count = perft(pos, depth);
		t2 = curr_time();
		fprintf(stdout, "Perft(%2d) = %20llu (%10llu ms)\n", depth, count, (t2 - t1));
	}
}

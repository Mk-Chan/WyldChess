#ifndef TT_H
#define TT_H

/*
 * WyldChess, a free UCI/Xboard compatible chess engine
 * Copyright (C) 2016-2017 Manik Charan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "defs.h"

#define FLAG_SHIFT  (21)
#define DEPTH_SHIFT (23)
#define SCORE_SHIFT (32)

#define FLAG_EXACT  ((1ULL << FLAG_SHIFT))
#define FLAG_UPPER  ((2ULL << FLAG_SHIFT))
#define FLAG_LOWER  ((3ULL << FLAG_SHIFT))
#define FLAG_MASK   ((3ULL << FLAG_SHIFT))

#define FLAG(data)  ((data) & FLAG_MASK)
#define DEPTH(data) ((int)((data) >> DEPTH_SHIFT) & 0x7f)
#define SCORE(data) ((int)((data) >> SCORE_SHIFT))

struct TTEntry
{
	u64 data;
	u64 key;
};

struct TT
{
	struct TTEntry* table;
	u32 size;
};

extern struct TT tt;

static inline int val_to_tt(int val, int ply)
{
	if (val >= MAX_MATE_VAL)
		val += ply;
	else if (val <= -MAX_MATE_VAL)
		val -= ply;
	return val;
}

static inline int val_from_tt(int val, int ply)
{
	if (val >= MAX_MATE_VAL)
		val -= ply;
	else if (val <= -MAX_MATE_VAL)
		val += ply;
	return val;
}

static inline void tt_clear(struct TT* tt)
{
	memset(tt->table, 0, sizeof(struct TTEntry) * tt->size);
}

static inline void tt_alloc_MB(struct TT* tt, u32 size)
{
	size     *= 0x10000;
	size      = max(size, 1);
	tt->table = (struct TTEntry*) realloc(tt->table, sizeof(struct TTEntry) * size);
	tt->size  = size;
	tt_clear(tt);
}

static inline void tt_destroy(struct TT* tt)
{
	free(tt->table);
}

// Maybe change to a 4-slot bucket scheme
static inline void tt_store(struct TT* tt, u64 score, u64 flag, u64 depth, u64 move, u64 key)
{
	u32 index = key < tt->size ? key : key % tt->size;
	struct TTEntry* entry = tt->table + index;
	entry->data = move | flag | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
	entry->key  = key ^ entry->data;
}

// Return a value instead of reference for thread safety
static inline struct TTEntry tt_probe(struct TT* tt, u64 key)
{
	return tt->table[(key < tt->size ? key : key % tt->size)];
}

#endif

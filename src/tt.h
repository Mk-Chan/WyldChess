#ifndef TT_H
#define TT_H

/*
 * WyldChess, a free UCI/Xboard compatible chess engine
 * Copyright (C) 2016  Manik Charan
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

struct PVEntry
{
	u64 move;
	u64 key;
};

struct PVT
{
	PVEntry* table;
	u32 size;
};

extern PVT pvt;

struct TTEntry
{
	u64 data;
	u64 key;
};

struct TT
{
	TTEntry* table;
	u32 size;
};

extern TT tt;

inline int val_to_tt(int val, int ply)
{
	return    val >= MAX_MATE_VAL  ? val + ply
		: val <= -MAX_MATE_VAL ? val - ply
		: val;
}

inline int val_from_tt(int val, int ply)
{
	return    val >= MAX_MATE_VAL  ? val - ply
		: val <= -MAX_MATE_VAL ? val + ply
		: val;
}

inline void tt_clear(TT* tt)
{
	memset(tt->table, 0, sizeof(TTEntry) * tt->size);
}

inline void tt_alloc_MB(TT* tt, u32 size)
{
	size     *= 0x10000;
	tt->table = (TTEntry*) realloc(tt->table, sizeof(TTEntry) * size);
	tt->size  = size;
	tt_clear(tt);
}

inline void tt_destroy(TT* tt)
{
	free(tt->table);
}

// Maybe change to a 4-slot bucket scheme
inline void tt_store(TT* tt, u64 score, u64 flag, u64 depth, u64 move, u64 key)
{
	u32 index      = key < tt->size ? key : key % tt->size;
	TTEntry* entry = tt->table + index;
	entry->data    = move | flag | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
	entry->key     = key ^ entry->data;
}

// Return a value instead of reference for thread safety
inline TTEntry tt_probe(TT* tt, u64 key)
{
	return tt->table[(key < tt->size ? key : key % tt->size)];
}

inline void pvt_clear(PVT* pvt)
{
	memset(pvt->table, 0, sizeof(PVEntry) * pvt->size);
}

inline void pvt_init(PVT* pvt, u32 size)
{
	pvt->table = (PVEntry*) malloc(sizeof(PVEntry) * size);
	pvt->size  = size;
	pvt_clear(pvt);
}

inline void pvt_destroy(PVT* pvt)
{
	free(pvt->table);
}

inline void pvt_store(PVT* pvt, u64 move, u64 key, u64 depth)
{
	u32 index       = key < pvt->size ? key : key % pvt->size;
	PVEntry* entry  = pvt->table + index;
	u32 entry_depth = entry->move >> 32;
	if (entry_depth > depth)
		return;
	entry->move     = move | (depth << 32);
	entry->key      = key;
}

inline PVEntry pvt_probe(PVT* pvt, u64 key)
{
	return pvt->table[(key < pvt->size ? key : key % pvt->size)];
}

#endif

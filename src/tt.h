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

#define FLAG(data)  ( (data) & FLAG_MASK)
#define DEPTH(data) (((data) >> DEPTH_SHIFT) & 0x7f)
#define SCORE(data) ((data) >> SCORE_SHIFT)

typedef struct PV_Entry_s {

	u64 move;
	u64 key;

} PV_Entry;

typedef struct PV_Table_s {

	PV_Entry* table;
	u32 size;

} PVT;

PVT pvt;

typedef struct TT_Entry_s {

	u64 data;
	u64 key;

} TT_Entry;

typedef struct TT_s {

	TT_Entry* table;
	u32 size;

} TT;

TT tt;

static inline void tt_clear(TT* tt)
{
	memset(tt->table, 0, sizeof(TT_Entry) * tt->size);
}

static inline void tt_age(TT* tt, int plies)
{
	TT_Entry* curr;
	TT_Entry* end = tt->table + tt->size;
	int depth;
	for (curr = tt->table; curr != end; ++curr) {
		depth       = DEPTH(curr->data);
		curr->data &= ~(0x7f << DEPTH_SHIFT);
		curr->data ^= max(depth - plies, 0) << DEPTH_SHIFT;
	}
}

static inline void tt_alloc_MB(TT* tt, u32 size)
{
	size     *= 0x10000;
	tt->table = realloc(tt->table, sizeof(TT_Entry) * size);
	tt->size  = size;
	tt_clear(tt);
}

static inline void tt_destroy(TT* tt)
{
	free(tt->table);
}

static inline void tt_store(TT* tt, u64 score, u64 flag, u64 depth, u64 move, u64 key)
{
	u32 index       = key < tt->size ? key : key % tt->size;
	TT_Entry* entry = tt->table + index;
	entry->data     = move | flag | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
	entry->key      = key ^ entry->data;
}

// Return a value instead of reference for thread safety
static inline TT_Entry tt_probe(TT* tt, u64 key)
{
	return tt->table[(key < tt->size ? key : key % tt->size)];
}

static inline void pvt_clear(PVT* pvt)
{
	memset(pvt->table, 0, sizeof(PV_Entry) * pvt->size);
}

static inline void pvt_init(PVT* pvt, u32 size)
{
	pvt->table = malloc(sizeof(PV_Entry) * size);
	pvt->size  = size;
	pvt_clear(pvt);
}

static inline void pvt_destroy(PVT* pvt)
{
	free(pvt->table);
}

static inline void pvt_store(PVT* pvt, u64 move, u64 key, u64 depth)
{
	u32 index       = key < pvt->size ? key : key % pvt->size;
	PV_Entry* entry = pvt->table + index;
	u32 entry_depth = entry->move >> 32;
	if (entry_depth > depth)
		return;
	entry->move     = move | (depth << 32);
	entry->key      = key;
}

static inline PV_Entry pvt_probe(PVT* pvt, u64 key)
{
	return pvt->table[(key < pvt->size ? key : key % pvt->size)];
}

#endif

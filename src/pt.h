#ifndef PT_H
#define PT_H

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

struct PTEntry
{
	int score_white;
	int score_black;
	u64 passed_pawn_white_bb;
	u64 passed_pawn_black_bb;
	u64 pawn_atks_white_bb;
	u64 pawn_atks_black_bb;
	u64 key;
};

struct PT
{
	struct PTEntry* table;
	u32 size;
};

extern struct PT pt;

static inline void pt_clear(struct PT* pt)
{
	memset(pt->table, 0, sizeof(struct PTEntry) * pt->size);
}

static inline void pt_alloc_MB(struct PT* pt, u32 size)
{
	size     *= 0x10000 * 2 / 3;
	size      = max(size, 1);
	pt->table = (struct PTEntry*) realloc(pt->table, sizeof(struct PTEntry) * size);
	pt->size  = size;
	pt_clear(pt);
}

static inline void pt_destroy(struct PT* pt)
{
	free(pt->table);
}

static inline void pt_store(struct PT* pt, int score_white, int score_black,
			    u64 passed_pawn_white_bb, u64 passed_pawn_black_bb,
			    u64 pawn_atks_white_bb, u64 pawn_atks_black_bb,
			    u64 key)
{
	u32 index = key < pt->size ? key : key % pt->size;
	struct PTEntry* entry = pt->table + index;
	entry->score_white = score_white;
	entry->score_black = score_black;
	entry->passed_pawn_white_bb = passed_pawn_white_bb;
	entry->passed_pawn_black_bb = passed_pawn_black_bb;
	entry->pawn_atks_white_bb = pawn_atks_white_bb;
	entry->pawn_atks_black_bb = pawn_atks_black_bb;
	entry->key = key ^ pawn_atks_white_bb ^ pawn_atks_black_bb;
}

// Return a value instead of reference for thread safety
static inline struct PTEntry pt_probe(struct PT* pt, u64 key)
{
	return pt->table[(key < pt->size ? key : key % pt->size)];
}

#endif

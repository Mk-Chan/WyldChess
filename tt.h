#ifndef TT_H
#define TT_H

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

typedef struct Entry_s {
	u64 data;
	u64 key; // Try implementing HashKey instead of just u64 later
} Entry;

typedef struct TT_s {
	Entry* table;
	u32    size;
} TT;

TT tt;

inline void tt_init(TT* tt, u32 size)
{
	tt->table = malloc(sizeof(Entry) * size);
	tt->size  = size;
}

inline void tt_destroy(TT* tt)
{
	free(tt->table);
}

inline void tt_store(TT* tt, u64 score, u64 flag, u64 depth, u64 move, u64 key)
{
	u32 index    = key < tt->size ? key : key % tt->size;
	Entry* entry = tt->table + index;
	entry->data  = move | flag | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
	entry->key   = key ^ entry->data;
}

// Return a value instead of reference for thread safety
inline Entry tt_probe(TT* tt, u64 key)
{
	return tt->table[(key < tt->size ? key : key % tt->size)];
}

#endif

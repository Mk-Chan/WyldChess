#ifndef ENGINE_H
#define ENGINE_H

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

#include <pthread.h>
#include "defs.h"
#include "position.h"
#include "misc.h"
#include "options.h"

enum Protocols {
	XBOARD,
	UCI
};

enum States {
	WAITING,
	THINKING,
	ANALYZING,
	QUITTING
};

enum SearchUnitType {
	MAIN,
	HELPER
};

struct SearchStack
{
	int node_type;
	int forward_prune;
	u32 ply;
	u32 killers[2];
	int order_arr[MAX_MOVES_PER_POS];
	struct Movelist list;
	int pv_depth;
	u32 pv[MAX_PLY];
};

struct SearchLocals
{
	u64 tb_hits;
	int history[8][64];
	u32 counter_move_table[64][64];
};

struct SearchUnit
{
	struct Position pos;
	struct SearchLocals sl;
	pthread_mutex_t mutex;
	pthread_cond_t sleep_cv;
	u32 max_searched_ply;
	int type;
	int protocol;
	int side;
	int game_over;
	int counter;
	u32 ponder_allowed;
	u32 ponder_move;
	u32 limited_moves_num;
	u32 limited_moves[MAX_MOVES_PER_POS];
	int volatile target_state;
	int volatile curr_state;
};

struct SearchParams
{
	struct SearchUnit* su;
	struct SearchStack* ss;
	int alpha;
	int beta;
	int depth;
	int result;
};

struct Controller
{
	volatile int is_stopped;
	volatile int analyzing;
	volatile int time_dependent;
	u32 depth;
	u32 moves_left;
	u32 moves_per_session;
	u64 increment;
	u64 time_left;
	u64 search_start_time;
	u64 search_end_time;
	u64 nodes_searched[MAX_THREADS];
};

extern struct Controller controller;

extern pthread_t search_threads[MAX_THREADS];
extern struct SearchUnit search_units[MAX_THREADS];
extern struct SearchStack search_stacks[MAX_THREADS][MAX_PLY];
extern struct SearchParams search_params[MAX_THREADS];

extern void init_search(struct SearchLocals* const sl);
extern int begin_search(struct SearchUnit* const su);
extern void xboard_loop();
extern void uci_loop();

static inline void init_search_unit(struct SearchUnit* const su)
{
	pthread_mutex_init(&su->mutex, NULL);
	pthread_cond_init(&su->sleep_cv, NULL);
	su->type = MAIN;
	su->target_state = WAITING;
	su->ponder_allowed = 1;
	su->ponder_move = 0;
	su->limited_moves_num = 0;
	init_search(&su->sl);
	init_pos(&su->pos);
	set_pos(&su->pos, INITIAL_POSITION);
}

static inline void get_search_locals_copy(struct SearchLocals const * const sl, struct SearchLocals* const sl_copy)
{
	memcpy(sl_copy, sl, sizeof(struct SearchLocals));
}

static inline void get_search_unit_copy(struct SearchUnit const * const su, struct SearchUnit* const copy_su)
{
	get_position_copy(&su->pos, &copy_su->pos);
	get_search_locals_copy(&su->sl, &copy_su->sl);
	pthread_mutex_init(&copy_su->mutex, NULL);
	pthread_cond_init(&copy_su->sleep_cv, NULL);
	memcpy(copy_su->limited_moves, su->limited_moves, sizeof(u32) * su->limited_moves_num);
	copy_su->limited_moves_num = su->limited_moves_num;
	copy_su->ponder_allowed    = su->ponder_allowed;
	copy_su->ponder_move       = su->ponder_move;
	copy_su->type              = su->type;
	copy_su->target_state      = WAITING;
	copy_su->counter           = 0;
	copy_su->protocol          = su->protocol;
	copy_su->max_searched_ply  = 0;
	copy_su->side              = su->side;
	copy_su->game_over         = su->game_over;
}

static inline u64 total_nodes_searched()
{
	u64* thread_nodes = controller.nodes_searched;
	static u64* end = controller.nodes_searched + MAX_THREADS;
	u64 count = *thread_nodes;
	for (++thread_nodes; thread_nodes < end; ++thread_nodes)
		count += *thread_nodes;
	return count;
}

static inline void sync(struct SearchUnit const * const su)
{
	while (su->curr_state != su->target_state)
		continue;
}

static inline void transition(struct SearchUnit* const su, int target_state)
{
	if (su->curr_state == WAITING) {
		pthread_mutex_lock(&su->mutex);
		su->target_state = target_state;
		pthread_cond_signal(&su->sleep_cv);
		pthread_mutex_unlock(&su->mutex);
	}
	else {
		su->target_state = target_state;
	}
	sync(su);
}

static inline void start_thinking(struct SearchUnit* const su)
{
	struct Controller* const ctlr = &controller;
	ctlr->search_start_time = curr_time();
	ctlr->time_left += (ctlr->moves_left - 1) * ctlr->increment;
	ctlr->search_end_time = ctlr->search_start_time
			     + (ctlr->time_left / ctlr->moves_left)
			     -  spin_options[MOVE_OVERHEAD].curr_val;
	transition(su, THINKING);
	if (ctlr->moves_per_session) {
		--ctlr->moves_left;
		if (ctlr->moves_left < 1)
			ctlr->moves_left = ctlr->moves_per_session;
	}
}

#endif

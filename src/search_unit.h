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
	u32 pv[MAX_PLY];
	u32 pv_depth;
};

struct SearchLocals
{
	u64 tb_hits;
	int history[8][64];
	u32 counter_move_table[64][64];
};

struct SearchWorker
{
	int side;
	int game_over;
	int volatile target_state;
	int volatile curr_state;
	pthread_mutex_t mutex;
	pthread_cond_t sleep_cv;
};

struct SearchUnit
{
	struct Position pos;
	struct SearchLocals sl;
	u64 counter;
	u32 max_ply;
	int type;
	struct SearchWorker sw;
};

struct Controller
{
	int is_stopped;
	int analyzing;
	int time_dependent;
	u32 depth;
	u32 moves_left;
	u32 moves_per_session;
	u64 increment;
	u64 time_left;
	u64 search_start_time;
	u64 search_end_time;
	u64 nodes_searched;
};

struct SplitPoint
{
	struct Position pos;
	struct SearchLocals sl;
	struct Movelist list;
	u32 counter_move;
	u32 ply;
	u32* pv;
	u32* pv_depth;
	int* best_val;
	u32* best_move;
	int* alpha;
	int beta;
	int depth;
	int checked;
	int move_num;
	int node_type;
	int static_eval;
	int moves_left;
	int volatile num_threads;
	int volatile in_use;
	int volatile finished;
	int volatile joinable;
	int owner_num;
	pthread_mutex_t mutex;
};

extern int protocol;
extern struct Controller controller;

extern struct SearchUnit search_units[MAX_THREADS];
extern struct SearchStack search_stacks[MAX_THREADS][MAX_PLY];
extern struct SplitPoint split_points[MAX_SPLIT_POINTS];
extern pthread_t threads[MAX_THREADS];

extern void search_split_point(struct SplitPoint* sp, int thread_num);
extern void init_search(struct SearchLocals* const sl);
extern int begin_search(struct SearchUnit* const su);
extern void xboard_loop();
extern void uci_loop();

static inline void init_search_unit(struct SearchUnit* const su)
{
	pthread_mutex_init(&su->sw.mutex, NULL);
	pthread_cond_init(&su->sw.sleep_cv, NULL);
	su->type = MAIN;
	su->sw.target_state = WAITING;
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
	pthread_mutex_init(&copy_su->sw.mutex, NULL);
	pthread_cond_init(&copy_su->sw.sleep_cv, NULL);
	copy_su->counter          = 0ULL;
	copy_su->type             = su->type;
	copy_su->max_ply          = su->max_ply;
	copy_su->sw.target_state  = WAITING;
	copy_su->sw.side          = su->sw.side;
	copy_su->sw.game_over     = su->sw.game_over;
}

static inline void sync(struct SearchUnit const * const su)
{
	while (su->sw.curr_state != su->sw.target_state)
		continue;
}

static inline void transition(struct SearchUnit* const su, int target_state)
{
	if (su->sw.curr_state == WAITING) {
		pthread_mutex_lock(&su->sw.mutex);
		su->sw.target_state = target_state;
		pthread_cond_signal(&su->sw.sleep_cv);
		pthread_mutex_unlock(&su->sw.mutex);
	}
	else {
		su->sw.target_state = target_state;
	}
	sync(su);
}

static inline void start_thinking(struct SearchUnit* const su)
{
	struct Controller* const ctlr = &controller;
	ctlr->search_start_time = curr_time();
	if (ctlr->time_dependent) {
		ctlr->time_left += (ctlr->moves_left - 1) * ctlr->increment;
		ctlr->search_end_time = ctlr->search_start_time
			             + (ctlr->time_left / ctlr->moves_left)
				     -  spin_options[MOVE_OVERHEAD].curr_val;
	}
	transition(su, THINKING);
	if (ctlr->moves_per_session) {
		--ctlr->moves_left;
		if (ctlr->moves_left < 1)
			ctlr->moves_left = ctlr->moves_per_session;
	}
}

#endif

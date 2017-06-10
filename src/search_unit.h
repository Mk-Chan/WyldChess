#ifndef ENGINE_H
#define ENGINE_H

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

#include <pthread.h>
#include "defs.h"
#include "position.h"
#include "misc.h"

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

struct SearchUnit
{
	struct Position*   pos;
	struct Controller* ctlr;
	u32                max_searched_ply;
	pthread_mutex_t    mutex;
	pthread_cond_t     sleep_cv;
	int                protocol;
	int                side;
	int                game_over;
	int volatile       target_state;
	int volatile       curr_state;
};

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
	struct Controller* const ctlr  = su->ctlr;
	ctlr->search_start_time = curr_time();
	if (ctlr->time_dependent) {
		ctlr->time_left += (ctlr->moves_left - 1) * ctlr->increment;
		ctlr->search_end_time = ctlr->search_start_time
			             + (ctlr->time_left / ctlr->moves_left)
				     -  10;
	}
	transition(su, THINKING);
	if (ctlr->moves_per_session) {
		--ctlr->moves_left;
		if (ctlr->moves_left < 1)
			ctlr->moves_left = ctlr->moves_per_session;
	}
}

extern int begin_search(struct SearchUnit* const su);
extern void xboard_loop();
extern void uci_loop();

#endif

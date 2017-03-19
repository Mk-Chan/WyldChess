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

typedef struct Controller_s {

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

} Controller;

typedef struct Engine_s {

	Position*   volatile pos;
	Controller* volatile ctlr;
	u32         volatile side;
	int         volatile target_state;
	int         volatile curr_state;
	int         volatile game_over;
	int                  protocol;
	pthread_mutex_t      mutex;
	pthread_cond_t       sleep_cv;

} Engine;

static inline void sync(Engine const * const engine)
{
	while (engine->curr_state != engine->target_state)
		continue;
}

static inline void transition(Engine* const engine, int target_state)
{
	if (engine->curr_state == WAITING) {
		pthread_mutex_lock(&engine->mutex);
		engine->target_state = target_state;
		pthread_cond_signal(&engine->sleep_cv);
		pthread_mutex_unlock(&engine->mutex);
	}
	else
		engine->target_state = target_state;
	sync(engine);
}

static inline void start_thinking(Engine* const engine)
{
	Controller* const ctlr  = engine->ctlr;
	ctlr->search_start_time = curr_time();
	if (ctlr->time_dependent) {
		ctlr->time_left += (ctlr->moves_left - 1) * ctlr->increment;
		ctlr->search_end_time = ctlr->search_start_time
			             + (ctlr->time_left / ctlr->moves_left)
				     -  10;
	}
	transition(engine, THINKING);
	if (ctlr->moves_per_session) {
		--ctlr->moves_left;
		if (ctlr->moves_left < 1)
			ctlr->moves_left = ctlr->moves_per_session;
	}
}

extern int begin_search(Engine* const engine);
extern void xboard_loop();
extern void uci_loop();

#endif

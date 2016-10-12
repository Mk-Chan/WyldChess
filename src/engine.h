#ifndef ENGINE_H
#define ENGINE_H

/*
 * WyldChess, a free Xboard/Winboard compatible chess engine
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

enum State {
	WAITING,
	THINKING,
//	ANALYZING,
	QUITTING
};

typedef struct Controller_s {

	int is_stopped;
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
	int                  game_over;
	pthread_mutex_t      mutex;
	pthread_cond_t       sleep_cv;

} Engine;

extern int begin_search(Engine* const engine);

#endif

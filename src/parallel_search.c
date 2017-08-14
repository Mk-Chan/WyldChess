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

#include "search.h"
#include "syzygy/tbprobe.h"

struct SearchUnit search_units[MAX_THREADS];
struct SearchStack search_stacks[MAX_THREADS][MAX_PLY];
struct SplitPoint split_points[MAX_SPLIT_POINTS];
pthread_mutex_t split_mutex;
pthread_t threads[MAX_THREADS];

u32 get_next_move(struct SplitPoint* sp, int* move_num)
{
	struct Movelist* list = &sp->list;
	if (list->end == list->moves)
		return 0;
	--list->end;
	u32 move = *list->end;
	*move_num = sp->move_num;
	++sp->move_num;
	return move;
}

void search_split_point(struct SplitPoint* sp, int thread_num)
{
	u32 move;
	int move_num, alpha, best_val;
	struct Controller* ctlr = &controller;
	struct SearchUnit* su = search_units + thread_num;
	struct SearchStack* ss = search_stacks[thread_num] + sp->ply;
	su->type = thread_num ? HELPER : MAIN;

	// Select move
	if (thread_num == sp->owner_num) {
		move = get_next_move(sp, &move_num);
		++sp->num_threads;
		sp->finished = 0;
		sp->joinable = 1;
	} else {
		pthread_mutex_lock(&sp->mutex);
		if (  !sp->joinable
		    || sp->finished
		    || sp->num_threads >= MAX_THREADS_PER_SP) {
			pthread_mutex_unlock(&sp->mutex);
			return;
		}
		move = get_next_move(sp, &move_num);
		if (!move) {
			sp->joinable = 0;
			pthread_mutex_unlock(&sp->mutex);
			return;
		}
		++sp->num_threads;
		pthread_mutex_unlock(&sp->mutex);
	}

	get_position_copy(&sp->pos, &su->pos);
	memcpy(&su->sl, &sp->sl, sizeof(struct SearchLocals));

	su->counter = 0ULL;
	su->max_ply = 0;
	while (1) {
		pthread_mutex_lock(&sp->mutex);
		alpha = *sp->alpha;
		best_val = *sp->best_val;
		pthread_mutex_unlock(&sp->mutex);

		int val = search_move(su, ss, best_val, alpha, sp->beta, sp->depth,
				      move, sp->counter_move, move_num, sp->static_eval, sp->checked, sp->node_type);

		if (ctlr->is_stopped || abort_search) {
			pthread_mutex_lock(&sp->mutex);
			sp->joinable = 0;
			sp->finished = 1;
			--sp->num_threads;
			pthread_mutex_unlock(&sp->mutex);
			break;
		}

		if (val > *sp->best_val) {
			pthread_mutex_lock(&sp->mutex);
			if (val > *sp->best_val) {
				*sp->best_val = val;
				*sp->best_move = move;
				if (val > *sp->alpha) {
					*sp->alpha = val;

					if (sp->node_type == PV_NODE) {
						*sp->pv = move;
						memcpy(sp->pv + 1, ss[1].pv, sizeof(u32) * ss[1].pv_depth);
						*sp->pv_depth = ss[1].pv_depth + 1;
					}
				}
			}
			pthread_mutex_unlock(&sp->mutex);
		}

		pthread_mutex_lock(&sp->mutex);
		--sp->moves_left;
		if (!sp->moves_left) {
			--sp->num_threads;
			sp->joinable = 0;
			sp->finished = 1;
			pthread_mutex_unlock(&sp->mutex);
			break;
		}

		move = get_next_move(sp, &move_num);
		if (!move) {
			--sp->num_threads;
			sp->joinable = 0;
			pthread_mutex_unlock(&sp->mutex);
			break;
		}
		pthread_mutex_unlock(&sp->mutex);
	}
}

void* work_loop(void* arg)
{
	int min_depth;
	int thread_num = ((pthread_t*) arg) - threads;

	struct SplitPoint* sp;
	struct SplitPoint* curr = split_points;
	static struct SplitPoint* end = split_points + MAX_SPLIT_POINTS;
	while (!abort_search) {
		// Choose split point
		sp = NULL;
		curr = split_points;
		min_depth = INFINITY;
		for (; curr != end; ++curr) {
			if (curr->in_use && curr->joinable) {
				if (curr->num_threads < MAX_THREADS_PER_SP) {
					sp = curr;
					break;
				}
			}
		}

		if (!sp || sp == end)
			continue;

		search_split_point(sp, thread_num);

	}
	return NULL;
}


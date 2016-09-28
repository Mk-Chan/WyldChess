#ifndef ENGINE_H
#define ENGINE_H

#include "defs.h"
#include "position.h"

enum State {
	GAME_OVER,
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
	u32                  move_list[MAX_MOVES];
	u32*                 move_list_end;

} Engine;

extern int begin_search(Engine* const engine);

#endif

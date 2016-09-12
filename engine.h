#ifndef ENGINE_H
#define ENGINE_H

#include "defs.h"
#include "position.h"

enum State {
	WAITING,
	THINKING,
	ANALYZING,
	QUITTING
};

typedef struct Controller_s {

	int time_left;
	int depth_left;
	int nodes_left;
	int moves_left;
	u64 nodes_searched;

} Controller;

typedef struct Engine_s {

	Position*   volatile pos;
	Controller* volatile ctlr;
	u32         volatile side;
	int         volatile state;

} Engine;

extern int begin_search(Engine* const engine);

#endif

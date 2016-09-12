#include "engine.h"
#include "tt.h"

static int stopped(Engine* const engine)
{
	if (engine->state != THINKING)
		return 1;

	// Controller logic to be added
	return 0;
}

static int is_repeat(Position* const pos)
{
	State* curr = pos->state;
	State* end  = curr - pos->state->fifty_moves;
	for (; end != curr; ++end) {
		if (curr->pos_key == end->pos_key)
			return 1;
	}
	return 0;
}

static int qsearch(Engine* const engine, int alpha, int beta)
{
	Position* const pos = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if ( !(ctlr->nodes_searched & 4095)
	    && stopped(engine))
		return INFINITY;

	if (pos->state->fifty_moves > 99)
		return 0;

	if (   pos->ply
	    && is_repeat(pos))
		return 0;

	if (pos->ply >= MAX_PLY)
		return evaluate(pos);

	int eval = evaluate(pos);
	if (eval > alpha) {
		if (eval >= beta)
			return beta;
		alpha = eval;
	}

	++ctlr->nodes_searched;

	int val;
	Movelist* list = pos->list + pos->ply;
	list->end      = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb)
		gen_check_evasions(pos, list);
	else
		gen_captures(pos, list);

	for (u32* move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, move))
			continue;
		val = -qsearch(engine, -beta, -alpha);
		undo_move(pos, move);
		if (val > alpha) {
			if (val >= beta) {
				return beta;
			}
			alpha = val;
		}
	}

	return alpha;
}

static int search(Engine* const engine, int alpha, int beta, u32 depth)
{
	Position* const pos = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if ( !(ctlr->nodes_searched & 4095)
	    && stopped(engine))
		return INFINITY;

	if (!depth)
		return qsearch(engine, alpha, beta);

	if (pos->state->fifty_moves > 99)
		return 0;

	if (pos->ply) {
		if (   pos->ply
		    && is_repeat(pos))
			return 0;

		if (pos->ply >= MAX_PLY)
			return evaluate(pos);
	}

	++ctlr->nodes_searched;

	Entry entry = tt_probe(&tt, pos->state->pos_key);
	if (  (entry.key ^ entry.data) == pos->state->pos_key
	    && DEPTH(entry.data) >= depth) {
		int val  = SCORE(entry.data);
		u64 flag = FLAG(entry.data);
		if (flag == FLAG_LOWER && val >= beta)
			return beta;
		else if (flag == FLAG_UPPER && val <= alpha)
			return alpha;
	}

	int val,
	    old_alpha   = alpha,
	    best_move   = 0,
	    best_val    = -INFINITY,
	    legal_moves = 0;

	Movelist* list = pos->list + pos->ply;
	list->end      = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	} else {
		gen_quiets(pos, list);
		gen_captures(pos, list);
	}

	for (u32* move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, move))
			continue;
		++legal_moves;
		val = -search(engine, -beta, -alpha, depth - 1);
		undo_move(pos, move);
		if (val > best_val) {
			if (val > alpha) {
				if (val >= beta) {
					tt_store_lower(&tt, beta, depth, *move, pos->state->pos_key);
					return beta;
				}
				alpha = val;
			}
			best_val  = val;
			best_move = *move;
		}
	}

	if (!legal_moves) {
		if (pos->state->checkers_bb)
			return -MATE_VAL + pos->ply;
		else
			return 0;
	}

	if (old_alpha == alpha)
		tt_store_upper(&tt, best_val, depth, best_move, pos->state->pos_key);
	else {
		tt_store_exact(&tt, alpha, depth, best_move, pos->state->pos_key);
	}

	return alpha;
}

static int valid_move(Position* const pos, u32* move)
{
	u32 from = from_sq(*move),
	    to   = to_sq(*move),
	    mt   = move_type(*move),
	    prom = prom_type(*move);
	Movelist* list = pos->list;
	list->end = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	} else {
		gen_quiets(pos, list);
		gen_captures(pos, list);
	}
	for (u32* m = list->moves; m != list->end; ++m) {
		if (   from_sq(*m) == from
		    && to_sq(*m) == to
		    && move_type(*m) == mt
		    && prom_type(*m) == prom)
			return 1;
	}
	return 0;
}

static int get_stored_move(Position* const pos)
{
	Entry entry = tt_probe(&tt, pos->state->pos_key);
	if ((entry.data ^ entry.key) == pos->state->pos_key) {
		u32 move = MOVE(entry.data);
		if (  !valid_move(pos, &move)
		   || !do_move(pos, &move))
			return 0;
		write(1, move_str(move), 6);
		write(1, " ", 1);
		get_stored_move(pos);
		undo_move(pos, &move);
		return move;
	}
	return 0;
}

int begin_search(Engine* const engine)
{
	int val, depth;
	int best_move = 0;
	char buff[20];
	int len;
	Position* const pos = engine->pos;
	Controller* const ctlr = engine->ctlr;
	ctlr->nodes_searched = 0ULL;
	for (depth = 1; depth <= MAX_PLY; ++depth) {
		val = search(engine, -INFINITY, +INFINITY, depth);
		if (stopped(engine))
			break;
		len = snprintf(buff, 20, "%d", depth);
		write(1, buff, len);
		write(1, " ", 1);

		if (val < 0) {
			write(1, "-", 1);
			val = -val;
		}
		snprintf(buff, 20, "%d", val);
		write(1, buff, len);
		write(1, " ", 1);

		snprintf(buff, 20, "%d", -1);
		write(1, buff, len);
		write(1, " ", 1);

		snprintf(buff, 20, "%llu", ctlr->nodes_searched);
		write(1, buff, len);
		write(1, " ", 1);

		best_move = get_stored_move(pos);
		write(1, "\n", 1);
	}
	return best_move;
}

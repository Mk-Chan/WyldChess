#include "engine.h"
#include "tt.h"
#include "timer.h"

static inline void swap_moves(u32* move_1, u32* move_2)
{
	u32 move_tmp = *move_1;
	*move_1  = *move_2;
	*move_2  = move_tmp;
}

static int stopped(Engine* const engine)
{
	if (engine->target_state != THINKING)
		return 1;
	if (curr_time() >= engine->ctlr->search_end_time)
		return 1;

	return 0;
}

static int is_repeat(Position* const pos)
{
	State* curr = pos->state;
	State* ptr  = curr - 2;
	State* end  = ptr - curr->fifty_moves;
	if (end < pos->hist)
		end = pos->hist;
	for (; ptr >= end; ptr -= 2)
		if (ptr->pos_key == curr->pos_key)
			return 1;
	return 0;
}

static int qsearch(Engine* const engine, int alpha, int beta)
{
	if ( !(engine->ctlr->nodes_searched & 4095)
	    && stopped(engine))
		return 0;

	Position* const pos = engine->pos;

	if (pos->state->fifty_moves > 99 || is_repeat(pos))
		return 0;

	if (pos->ply >= MAX_PLY)
		return evaluate(pos);

	++engine->ctlr->nodes_searched;

	int eval = evaluate(pos);
	if (eval >= beta)
		return beta;
	if (eval > alpha)
		alpha = eval;

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
			if (val >= beta)
				return beta;
			alpha = val;
		}
	}

	return alpha;
}

static int search(Engine* const engine, int alpha, int beta, u32 depth)
{
	if (!depth)
		return qsearch(engine, alpha, beta);

	if ( !(engine->ctlr->nodes_searched & 4095)
	    && stopped(engine))
		return 0;

	Position* const pos = engine->pos;

	if (pos->ply) {
		if (pos->state->fifty_moves > 99 || is_repeat(pos))
			return 0;

		if (pos->ply >= MAX_PLY)
			return evaluate(pos);
	}

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

	++engine->ctlr->nodes_searched;

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

	u32* move;
	u32 tt_move = MOVE(entry.data);
	if (tt_move) {
		for (move = list->moves; move != list->end; ++move) {
			if (*move == tt_move) {
				swap_moves(list->moves, move);
				break;
			}
		}
	}

	for (move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, move))
			continue;
		++legal_moves;
		val = -search(engine, -beta, -alpha, depth - 1);
		undo_move(pos, move);
		if (val >= beta) {
#ifdef STATS
			if (legal_moves == 1)
				++pos->stats.first_beta_cutoffs;
			++pos->stats.beta_cutoffs;
#endif
			tt_store(&tt, val, FLAG_LOWER, depth, *move, pos->state->pos_key);
			return beta;
		}
		if (val > best_val) {
			if (val > alpha) {
				alpha = val;
			}
			best_val  = val;
			best_move = *move;
		}
	}

	if (!legal_moves) {
		if (pos->state->checkers_bb)
			return -INFINITY + pos->ply;
		else
			return 0;
	}

	if (old_alpha == alpha)
		tt_store(&tt, alpha, FLAG_UPPER, depth, best_move, pos->state->pos_key);
	else
		tt_store(&tt, alpha, FLAG_EXACT, depth, best_move, pos->state->pos_key);

	return alpha;
}

static int valid_move(Position* const pos, u32* move)
{
	u32 from = from_sq(*move),
	    to   = to_sq(*move),
	    mt   = move_type(*move),
	    prom = prom_type(*move);
	Movelist* list = pos->list;
	list->end      = list->moves;
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

static int get_stored_moves(Position* const pos, int depth)
{
	static char mstr[6];
	if (!depth)
		return 0;
	Entry entry = tt_probe(&tt, pos->state->pos_key);
	if ((entry.data ^ entry.key) == pos->state->pos_key) {
		u32 move = MOVE(entry.data);
		if (  !valid_move(pos, &move)
		   || !do_move(pos, &move))
			return 0;
		fprintf(stdout, " ");
		move_str(move, mstr);
		fprintf(stdout, "%s", mstr);
		get_stored_moves(pos, depth - 1);
		undo_move(pos, &move);
		return move;
	}
	return 0;
}

int begin_search(Engine* const engine)
{
	int val, depth;
	int best_move = 0;
	Position* const pos = engine->pos;
	Controller* const ctlr = engine->ctlr;
	ctlr->nodes_searched = 0ULL;
	engine->pos->ply = 0;
#ifdef STATS
	pos->stats.beta_cutoffs = 0;
	pos->stats.first_beta_cutoffs = 0;
#endif
	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	for (depth = 1; depth <= max_depth; ++depth) {
		val = search(engine, -INFINITY, +INFINITY, depth);
		if (depth > 1 && stopped(engine))
			break;
		fprintf(stdout, "%d %d %llu %llu", depth, val,
			(curr_time() - ctlr->search_start_time) / 10, ctlr->nodes_searched);
		best_move = get_stored_moves(pos, depth);
		fprintf(stdout, "\n");
	}
#ifdef STATS
	fprintf(stdout, "ordering=%lf\n", ((double)pos->stats.first_beta_cutoffs) / (pos->stats.beta_cutoffs));
#endif

	return best_move;
}

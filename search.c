#include "engine.h"
#include "tt.h"
#include "timer.h"
#include "eval.h"

u64 const HASH_MOVE = 60000ULL;
u64 const GOOD_CAP  = 50000ULL;
u64 const KILLER    = 40000ULL;
u64 const PROM      = 30000ULL;
u64 const BAD_CAP   = 20000ULL;

static inline void order_cap(Position const * const pos, Move* const m)
{
	int tmp = piece_val[piece_type(pos->board[to_sq(*m)])]
		- piece_val[piece_type(pos->board[from_sq(*m)])];
	if (tmp >= -50)
		encode_order(*m, (GOOD_CAP + tmp));
	else
		encode_order(*m, (BAD_CAP + tmp));
}

static inline void sort_moves(Move* const start, Move* const end)
{
	Move* tmp;
	Move* m;
	Move to_shift;
	for (m = start + 1; m < end; ++m) {
		to_shift = *m;
		for (tmp = m; tmp > start && order(to_shift) > order(*(tmp - 1)); --tmp)
			*tmp = *(tmp - 1);
		*tmp = to_shift;
	}
}

static int stopped(Engine* const engine)
{
	if (engine->ctlr->is_stopped)
		return 1;
	if (engine->target_state != THINKING) {
		engine->ctlr->is_stopped = 1;
		return 1;
	}
	if (curr_time() >= engine->ctlr->search_end_time) {
		engine->ctlr->is_stopped = 1;
		return 1;
	}

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
	if ( !(engine->ctlr->nodes_searched & 2047)
	    && stopped(engine))
		return 0;

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

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
	Move* move;
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
	} else {
		gen_captures(pos, list);

		// Order captures by MVV-LVA
		for (move = list->moves; move < list->end; ++move)
			order_cap(pos, move);
		sort_moves(list->moves, list->end);
	}

	u32 legal = 0;
	for (move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, *move))
			continue;
		++legal;
		val = -qsearch(engine, -beta, -alpha);
		undo_move(pos, *move);
		if (ctlr->is_stopped)
			return 0;
		if (val > alpha) {
			if (val >= beta) {
#ifdef STATS
				if (legal == 1)
					++pos->stats.first_beta_cutoffs;
				++pos->stats.beta_cutoffs;
#endif
				return beta;
			}
			alpha = val;
		}
	}

	return alpha;
}

static int search(Engine* const engine, int alpha, int beta, u32 depth)
{
	if (!depth)
		return qsearch(engine, alpha, beta);

	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;

	if (pos->ply) {
		if ( !(engine->ctlr->nodes_searched & 2047)
		    && stopped(engine))
			return 0;

		if (pos->state->fifty_moves > 99 || is_repeat(pos))
			return 0;

		if (pos->ply >= MAX_PLY)
			return evaluate(pos);
	}

	Entry entry = tt_probe(&tt, pos->state->pos_key);
#ifdef STATS
	++pos->stats.hash_probes;
#endif
	if (  (entry.key ^ entry.data) == pos->state->pos_key
	    && DEPTH(entry.data) >= depth) {
#ifdef STATS
		++pos->stats.hash_hits;
#endif
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

	Movelist* list  = pos->list + pos->ply;
	list->end       = list->moves;
	set_pinned(pos);
	set_checkers(pos);
	Move* quiets;
	if (pos->state->checkers_bb) {
		gen_check_evasions(pos, list);
		quiets = list->moves;
	} else {
		gen_captures(pos, list);
		quiets = list->end;
		gen_quiets(pos, list);
	}

	Move* move;

	/*
	 *  Move ordering:
	 *  1. Hash move
	 *  2. Good/Equal captures
	 *  3. Killer moves
	 *  4. Promotions
	 *  5. Bad captures
	 *  6. Rest
	 */
	Move tt_move = get_move(entry.data);
	for (move = list->moves; move != list->end; ++move) {
		if (*move == tt_move) {
			encode_order(*move, HASH_MOVE);
		} else if (move < quiets) {
			order_cap(pos, move);
		} else {
			if (move_type(*move) == PROMOTION)
				encode_order(*move, PROM);
		}
	}
	sort_moves(list->moves, list->end);

	for (move = list->moves; move != list->end; ++move) {
		if (!do_move(pos, *move))
			continue;
		++legal_moves;

		// Principal Variation Search
		if (legal_moves == 1)
			val = -search(engine, -beta, -alpha, depth - 1);
		else {
			val = -search(engine, -alpha - 1, -alpha, depth - 1);
			if (val > alpha)
				val = -search(engine, -beta, -alpha, depth - 1);
		}

		undo_move(pos, *move);

		if (   pos->ply
		    && ctlr->is_stopped)
			return 0;

		if (val >= beta) {
#ifdef STATS
			if (legal_moves == 1)
				++pos->stats.first_beta_cutoffs;
			++pos->stats.beta_cutoffs;
#endif
			tt_store(&tt, val, FLAG_LOWER, depth, get_move(*move), pos->state->pos_key);
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
		tt_store(&tt, alpha, FLAG_UPPER, depth, get_move(best_move), pos->state->pos_key);
	else
		tt_store(&tt, alpha, FLAG_EXACT, depth, get_move(best_move), pos->state->pos_key);

	return alpha;
}

static int valid_move(Position* const pos, Move* move)
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
	for (Move* m = list->moves; m != list->end; ++m) {
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
		Move move = get_move(entry.data);
		if (  !valid_move(pos, &move)
		   || !do_move(pos, move))
			return 0;
		fprintf(stdout, " ");
		move_str(move, mstr);
		fprintf(stdout, "%s", mstr);
		get_stored_moves(pos, depth - 1);
		undo_move(pos, move);
		return move;
	}
	return 0;
}

static void clear_search(Engine* const engine)
{
	Controller* const ctlr = engine->ctlr;
	ctlr->is_stopped       = 0;
	ctlr->nodes_searched   = 0ULL;
#ifdef STATS
	Position* const pos           = engine->pos;
	pos->ply                      = 0;
	pos->stats.first_beta_cutoffs = 0;
	pos->stats.beta_cutoffs       = 0;
	pos->stats.hash_probes        = 0;
	pos->stats.hash_hits          = 0;
#endif
}

int begin_search(Engine* const engine)
{
	int val, depth;
	int best_move = 0;
	clear_search(engine);
	Position* const pos    = engine->pos;
	Controller* const ctlr = engine->ctlr;
	int max_depth = ctlr->depth > MAX_PLY ? MAX_PLY : ctlr->depth;
	for (depth = 1; depth <= max_depth; ++depth) {
		val = search(engine, -INFINITY, +INFINITY, depth);
		if (   depth > 1
		    && ctlr->is_stopped)
			break;
		fprintf(stdout, "%d %d %llu %llu", depth, val,
			(curr_time() - ctlr->search_start_time) / 10, ctlr->nodes_searched);
		best_move = get_stored_moves(pos, depth);
		fprintf(stdout, "\n");
	}
#ifdef STATS
	fprintf(stdout, "hash hit rate=%lf\n",
		((double)pos->stats.hash_hits) / pos->stats.hash_probes);
	fprintf(stdout, "ordering=%lf\n",
		((double)pos->stats.first_beta_cutoffs) / (pos->stats.beta_cutoffs));
#endif

	return best_move;
}

// ai.cpp

#include "ai.hpp"
#include "rules.hpp"
#include "heuristic.hpp"
#include "transposition.hpp"
#include <chrono>
#include <vector>
#include <algorithm>
#include <utility>

// Score d'une victoire CONFIRMÉE, volontairement bien plus élevé que
// n'importe quel score heuristique possible (le plus haut score défini
// dans PATTERN_SCORES est 10_000_000 pour un alignement de 5).
constexpr long long WIN_SCORE = 100'000'000;

// Bornes "infinies" sûres -- on évite LLONG_MIN/MAX pour ne jamais risquer
// un dépassement de capacité lors d'opérations comme -WIN_SCORE - depth.
constexpr long long NEG_INF = -1'000'000'000'000LL;
constexpr long long POS_INF =  1'000'000'000'000LL;

// Levée quand le temps imparti est dépassé PENDANT une recherche en cours.
// Traverse proprement toute la récursion Minimax jusqu'à ai_get_best_move.
struct TimeoutException : public std::exception {};

using Clock = std::chrono::steady_clock;


static int max_candidates_for_depth(int depth) {
    if (depth >= 8) return 10;
    if (depth >= 5) return 7;
    if (depth >= 2) return 5;
    return 3;
}

static int count_adjacent_stones(const GomokuBoard& board, int row, int col) {
    int count = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr, c = col + dc;
            if (board.in_bounds(r, c) && board.grid[r][c] != EMPTY) count++;
        }
    }
    return count;
}

struct ScoredMove { long long score; int row, col; };

static std::vector<std::pair<int,int>> order_moves(GomokuBoard& board, const MoveList& moves,
                                                     int player, bool use_quick_score) {
    std::vector<ScoredMove> scored;
    scored.reserve(moves.count);

    if (use_quick_score) {
        for (int i = 0; i < moves.count; i++) {
            int r = moves.rows[i], c = moves.cols[i];
            MoveRecord rec = apply_move(board, r, c);
            long long score = quick_move_score(board, r, c, player);
            undo_move(board, rec);
            scored.push_back({score, r, c});
        }
    } else {
        for (int i = 0; i < moves.count; i++) {
            int r = moves.rows[i], c = moves.cols[i];
            long long score = count_adjacent_stones(board, r, c);
            scored.push_back({score, r, c});
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    std::vector<std::pair<int,int>> result;
    result.reserve(scored.size());
    for (auto& s : scored) result.emplace_back(s.row, s.col);
    return result;
}

// Garantit que les coups critiques (victoire immédiate ou blocage
// obligatoire) sont toujours inclus, même si le beam search les aurait
// sinon exclus faute de bon classement.
static std::vector<std::pair<int,int>> ensure_critical_moves(
        const std::vector<std::pair<int,int>>& ordered,
        const ThreatCells& threats, int max_count) {

    std::vector<std::pair<int,int>> critical;
    for (int i = 0; i < threats.num_my_wins; i++) {
        critical.emplace_back(threats.my_wins_r[i], threats.my_wins_c[i]);
    }
    for (int i = 0; i < threats.num_opp_wins; i++) {
        std::pair<int,int> p(threats.opp_wins_r[i], threats.opp_wins_c[i]);
        bool dup = false;
        for (auto& q : critical) if (q == p) { dup = true; break; }
        if (!dup) critical.push_back(p);
    }

    if (critical.empty()) {
        int n = std::min((int)ordered.size(), max_count);
        return std::vector<std::pair<int,int>>(ordered.begin(), ordered.begin() + n);
    }

    std::vector<std::pair<int,int>> rest;
    for (auto& m : ordered) {
        bool in_critical = false;
        for (auto& c : critical) if (c == m) { in_critical = true; break; }
        if (!in_critical) rest.push_back(m);
    }

    int remaining = max_count - (int)critical.size();
    if (remaining < 0) remaining = 0;

    std::vector<std::pair<int,int>> result = critical;
    for (int i = 0; i < remaining && i < (int)rest.size(); i++) {
        result.push_back(rest[i]);
    }
    return result;
}

static void store_result(TranspositionTable& tt, uint64_t hash, long long value,
                          int depth, long long original_alpha, long long beta) {
    int type;
    if (value <= original_alpha) type = UPPER_BOUND;
    else if (value >= beta) type = LOWER_BOUND;
    else type = EXACT;
    tt.store(hash, value, depth, type);
}

static long long minimax(GomokuBoard& board, int depth, long long alpha, long long beta,
                          bool maximizing_player, int ai_player,
                          bool has_last_move, int last_row, int last_col,
                          Clock::time_point deadline,
                          ZobristHasher& hasher, TranspositionTable& trans_table,
                          uint64_t position_hash) {

    if (Clock::now() >= deadline) {
        throw TimeoutException();
    }

    long long original_alpha = alpha;
    long long cached_score;
    if (trans_table.lookup(position_hash, depth, alpha, beta, cached_score)) {
        return cached_score;
    }

    int current_player = board.current_player;

    if (has_last_move && check_win(board, last_row, last_col)) {
        int winner = board.grid[last_row][last_col];
        long long score = (winner == ai_player) ? (WIN_SCORE + depth) : (-WIN_SCORE - depth);
        trans_table.store(position_hash, score, depth, EXACT);
        return score;
    }

    if (depth == 0) {
        long long score = evaluate_position(board, ai_player);
        trans_table.store(position_hash, score, depth, EXACT);
        return score;
    }

    MoveList legal_moves;
    ThreatCells threats;
    get_legal_moves_and_threats(board, current_player, legal_moves, threats);

    if (legal_moves.count == 0) {
        long long score = evaluate_position(board, ai_player);
        trans_table.store(position_hash, score, depth, EXACT);
        return score;
    }

    bool use_quick = (depth >= 3);
    auto ordered = order_moves(board, legal_moves, current_player, use_quick);
    ordered = ensure_critical_moves(ordered, threats, max_candidates_for_depth(depth));

    if (maximizing_player) {
        long long max_eval = NEG_INF;
        for (auto& mv : ordered) {
            int row = mv.first, col = mv.second;
            MoveRecord record = apply_move(board, row, col);
            uint64_t child_hash = hasher.update_hash(position_hash, row, col, current_player);

            long long eval_score;
            try {
                eval_score = minimax(board, depth - 1, alpha, beta, false, ai_player,
                                      true, row, col, deadline, hasher, trans_table, child_hash);
            } catch (...) {
                undo_move(board, record);
                throw;
            }
            undo_move(board, record);

            if (eval_score > max_eval) max_eval = eval_score;
            if (eval_score > alpha) alpha = eval_score;
            if (beta <= alpha) break;
        }
        store_result(trans_table, position_hash, max_eval, depth, original_alpha, beta);
        return max_eval;

    } else {
        long long min_eval = POS_INF;
        for (auto& mv : ordered) {
            int row = mv.first, col = mv.second;
            MoveRecord record = apply_move(board, row, col);
            uint64_t child_hash = hasher.update_hash(position_hash, row, col, current_player);

            long long eval_score;
            try {
                eval_score = minimax(board, depth - 1, alpha, beta, true, ai_player,
                                      true, row, col, deadline, hasher, trans_table, child_hash);
            } catch (...) {
                undo_move(board, record);
                throw;
            }
            undo_move(board, record);

            if (eval_score < min_eval) min_eval = eval_score;
            if (eval_score < beta) beta = eval_score;
            if (beta <= alpha) break;
        }
        store_result(trans_table, position_hash, min_eval, depth, original_alpha, beta);
        return min_eval;
    }
}

static std::pair<std::pair<int,int>, long long> search_one_depth(
        GomokuBoard& board, int depth, int ai_player, Clock::time_point deadline,
        ZobristHasher& hasher, TranspositionTable& trans_table, uint64_t root_hash,
        bool has_preferred, int preferred_row, int preferred_col) {

    MoveList legal_moves;
    ThreatCells threats;
    get_legal_moves_and_threats(board, ai_player, legal_moves, threats);

    auto ordered = order_moves(board, legal_moves, ai_player, true);
    ordered = ensure_critical_moves(ordered, threats, 20);

    // PV move ordering : remonter le coup préféré en tête, s'il est présent.
    if (has_preferred) {
        std::pair<int,int> pref(preferred_row, preferred_col);
        auto it = std::find(ordered.begin(), ordered.end(), pref);
        if (it != ordered.end()) {
            ordered.erase(it);
            ordered.insert(ordered.begin(), pref);
        }
    }

    long long best_score = NEG_INF;
    std::pair<int,int> best_move = ordered[0];
    long long alpha = NEG_INF, beta = POS_INF;

    for (auto& mv : ordered) {
        int row = mv.first, col = mv.second;
        MoveRecord record = apply_move(board, row, col);
        uint64_t child_hash = hasher.update_hash(root_hash, row, col, ai_player);

        long long score;
        try {
            score = minimax(board, depth - 1, alpha, beta, false, ai_player,
                             true, row, col, deadline, hasher, trans_table, child_hash);
        } catch (...) {
            undo_move(board, record);
            throw;
        }
        undo_move(board, record);

        if (score > best_score) {
            best_score = score;
            best_move = {row, col};
        }
        if (score > alpha) alpha = score;
    }

    return {best_move, best_score};
}

AIResult ai_get_best_move(GomokuBoard& board, int max_depth, double time_limit_seconds) {
    auto start_time = Clock::now();
    auto deadline = start_time + std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(time_limit_seconds));

    int ai_player = board.current_player;
    MoveList legal_moves;
    get_legal_moves(board, ai_player, legal_moves);

    AIResult result{};
    if (legal_moves.count == 0) {
        result.valid = false;
        return result;
    }

    ZobristHasher hasher(42);
    TranspositionTable trans_table;
    uint64_t root_hash = hasher.compute_hash(board);

    std::pair<int,int> best_move(legal_moves.rows[0], legal_moves.cols[0]);
    long long best_score = NEG_INF;
    int depth_reached = 0;

    bool has_preferred = false;
    int preferred_row = 0, preferred_col = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        std::pair<std::pair<int,int>, long long> depth_result;
        try {
            depth_result = search_one_depth(board, depth, ai_player, deadline,
                                             hasher, trans_table, root_hash,
                                             has_preferred, preferred_row, preferred_col);
        } catch (const TimeoutException&) {
            break;
        }

        best_move = depth_result.first;
        best_score = depth_result.second;
        depth_reached = depth;
        has_preferred = true;
        preferred_row = best_move.first;
        preferred_col = best_move.second;

        if (best_score >= WIN_SCORE) break;
    }

    double elapsed_ms = std::chrono::duration<double, std::milli>(Clock::now() - start_time).count();

    result.row = best_move.first;
    result.col = best_move.second;
    result.score = best_score;
    result.time_ms = elapsed_ms;
    result.depth_reached = depth_reached;
    result.valid = true;
    return result;
}
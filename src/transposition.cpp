// transposition.cpp

#include "transposition.hpp"
#include <random>

ZobristHasher::ZobristHasher(unsigned seed) {
    // Graine FIXE (comme random.Random(seed) en Python) pour des hashes
    // reproductibles -- utile pour le débogage et les tests.
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    for (int player = 1; player <= 2; player++) {
        for (int r = 0; r < BOARD_SIZE; r++) {
            for (int c = 0; c < BOARD_SIZE; c++) {
                table_[player][r][c] = dist(rng);
            }
        }
    }
}

uint64_t ZobristHasher::compute_hash(const GomokuBoard& board) const {
    uint64_t h = 0;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            int v = board.grid[r][c];
            if (v != EMPTY) {
                h ^= table_[v][r][c];
            }
        }
    }
    return h;
}

uint64_t ZobristHasher::update_hash(uint64_t current_hash, int row, int col, int player) const {
    return current_hash ^ table_[player][row][col];
}

bool TranspositionTable::lookup(uint64_t hash, int depth, long long alpha, long long beta,
                                 long long& out_score) const {
    auto it = table_.find(hash);
    if (it == table_.end()) {
        return false;
    }

    const TTEntry& entry = it->second;

    if (entry.depth < depth) {
        return false;  // pas assez profond pour être réutilisé
    }

    if (entry.type == EXACT) {
        out_score = entry.score;
        return true;
    }

    if (entry.type == LOWER_BOUND && entry.score >= beta) {
        out_score = entry.score;
        return true;
    }

    if (entry.type == UPPER_BOUND && entry.score <= alpha) {
        out_score = entry.score;
        return true;
    }

    return false;
}

void TranspositionTable::store(uint64_t hash, long long score, int depth, int type) {
    auto it = table_.find(hash);
    if (it != table_.end() && it->second.depth > depth) {
        return;  // on garde l'entrée existante, plus profonde donc plus fiable
    }

    TTEntry entry;
    entry.score = score;
    entry.depth = depth;
    entry.type = type;
    entry.present = true;
    table_[hash] = entry;
}

void TranspositionTable::clear() {
    table_.clear();
}

size_t TranspositionTable::size() const {
    return table_.size();
}
// transposition.hpp
//
// Portage C++ de transposition.py : hachage de Zobrist et table de
// transposition, pour éviter de recalculer une position déjà vue.

#pragma once
#include "board.hpp"
#include <cstdint>
#include <unordered_map>

enum EntryType {
    EXACT = 0,
    LOWER_BOUND = 1,
    UPPER_BOUND = 2,
};

class ZobristHasher {
public:
    explicit ZobristHasher(unsigned seed = 42);

    // Calcul complet du hash d'une position (O(size²), à utiliser pour
    // initialiser un hash, pas pour les mises à jour pendant la recherche).
    uint64_t compute_hash(const GomokuBoard& board) const;

    // Mise à jour incrémentale en O(1) -- le XOR est sa propre inverse,
    // donc la même fonction sert pour apply_move ET undo_move.
    uint64_t update_hash(uint64_t current_hash, int row, int col, int player) const;

private:
    // table[player][row][col] -- index 0 (player) inutilisé
    uint64_t table_[3][BOARD_SIZE][BOARD_SIZE];
};

struct TTEntry {
    long long score;
    int depth;
    int type;
    bool present = false;
};

class TranspositionTable {
public:
    // Retourne true et remplit out_score si une entrée utilisable existe
    // pour cette position à cette profondeur (ou plus profonde).
    bool lookup(uint64_t hash, int depth, long long alpha, long long beta,
                long long& out_score) const;

    // Enregistre le résultat d'une recherche. Ne remplace pas une entrée
    // existante plus profonde (donc plus fiable) que la nouvelle.
    void store(uint64_t hash, long long score, int depth, int type);

    void clear();
    size_t size() const;

private:
    std::unordered_map<uint64_t, TTEntry> table_;
};
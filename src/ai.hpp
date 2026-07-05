// ai.hpp
//
// Portage C++ de ai.py : Minimax + Alpha-Beta + iterative deepening.

#pragma once
#include "board.hpp"

struct AIResult {
    int row, col;
    long long score;
    double time_ms;
    int depth_reached;
    bool valid;  // false si aucun coup n'était possible
};

AIResult ai_get_best_move(GomokuBoard& board, int max_depth = 10, double time_limit_seconds = 0.5);
// heuristic.hpp
//
// Portage C++ de heuristic.py : évaluation de position et évaluation
// rapide locale pour le tri des coups (move ordering).

#pragma once
#include "board.hpp"

// Évalue la position complète du point de vue de 'player'.
// Positif = favorable à player, négatif = favorable à l'adversaire.
long long evaluate_position(GomokuBoard& board, int player);

// Évaluation RAPIDE et locale d'un coup déjà joué en (row, col), pour le
// tri des coups avant Minimax (PAS pour l'évaluation finale d'une position).
// Suppose que board.grid[row][col] == player (le coup vient d'être joué).
long long quick_move_score(GomokuBoard& board, int row, int col, int player);
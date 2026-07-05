// rules.hpp
//
// Portage C++ de rules.py : capture, undo, double-three, get_legal_moves,
// endgame capture. Même logique que la version Python, déjà validée par
// nos tests -- ce portage doit produire EXACTEMENT les mêmes résultats.

#pragma once
#include "board.hpp"
#include <vector>
#include <array>

// ──────────────────────────────────────────────────────────────────────
// APPLY_MOVE / UNDO_MOVE
// ──────────────────────────────────────────────────────────────────────

// Équivalent du "move_record" Python : décrit tout ce qui a changé sur
// le plateau lors d'un coup, pour pouvoir l'annuler ensuite.
// On utilise un tableau fixe (pas de vector) pour les pierres capturées :
// au maximum 8 directions peuvent chacune capturer 2 pierres, donc 16 max.
struct MoveRecord {
    int row, col;
    int player;
    int num_captured;                  // nombre de pierres capturées (0 à 16)
    int captured_r[16];
    int captured_c[16];
    int captured_val[16];              // valeur d'origine avant capture
    bool valid;                        // false si le coup était illégal
};

// Joue un coup et retourne le move_record correspondant.
// Si le coup est illégal (hors plateau ou case occupée), record.valid = false.
MoveRecord apply_move(GomokuBoard& board, int row, int col);

// Annule un coup précédemment joué avec apply_move.
void undo_move(GomokuBoard& board, const MoveRecord& record);

// ──────────────────────────────────────────────────────────────────────
// DOUBLE-THREE
// ──────────────────────────────────────────────────────────────────────

bool is_double_three(GomokuBoard& board, int row, int col, int player);

// Compte le nombre de free-three créés sur les 4 axes (utilisé aussi
// pour du debug / tests). early_stop_at permet de s'arrêter dès qu'un
// seuil est atteint (optimisation, équivalent du paramètre Python).
int count_free_threes(GomokuBoard& board, int row, int col, int early_stop_at = -1);

// ──────────────────────────────────────────────────────────────────────
// GÉNÉRATION DES COUPS LÉGAUX
// ──────────────────────────────────────────────────────────────────────

// On utilise un tableau fixe de taille maximale (361 cases) plutôt qu'un
// std::vector, pour éviter les allocations dynamiques répétées à chaque
// appel (appelé des dizaines de milliers de fois pendant Minimax).
struct MoveList {
    int rows[BOARD_SIZE * BOARD_SIZE];
    int cols[BOARD_SIZE * BOARD_SIZE];
    int count;

    inline void clear() { count = 0; }
    inline void add(int r, int c) {
        rows[count] = r;
        cols[count] = c;
        count++;
    }
};

void get_legal_moves(GomokuBoard& board, int player, MoveList& out);

// Variante qui expose aussi les cases de victoire immédiate (pour le
// look-ahead de threat detection, équivalent de get_legal_moves_and_threats).
struct ThreatCells {
    // Petites listes fixes -- en pratique il y a rarement plus de
    // quelques cases de victoire immédiate simultanées.
    int my_wins_r[16], my_wins_c[16];
    int num_my_wins;
    int opp_wins_r[16], opp_wins_c[16];
    int num_opp_wins;
};

void get_legal_moves_and_threats(GomokuBoard& board, int player,
                                  MoveList& out_moves, ThreatCells& out_threats);

// ──────────────────────────────────────────────────────────────────────
// ENDGAME CAPTURE / VICTOIRE
// ──────────────────────────────────────────────────────────────────────

bool check_win(GomokuBoard& board, int row, int col);

// ──────────────────────────────────────────────────────────────────────
// POTENTIEL DE CAPTURE (pour l'heuristique statique)
// ──────────────────────────────────────────────────────────────────────
//
// Scanne le plateau et compte, du point de vue de 'player' :
//   - out_opportunities : paires ADVERSES que 'player' pourrait capturer
//                         à son prochain coup
//   - out_threats       : paires de 'player' que l'ADVERSAIRE pourrait
//                         capturer à son prochain coup
//
// Utilisé par heuristic.cpp pour que l'évaluation statique tienne compte
// des captures POTENTIELLES, pas seulement des captures déjà réalisées.
void count_potential_captures(GomokuBoard& board, int player,
                               int& out_opportunities, int& out_threats);
// rules.cpp

#include "rules.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cstring>
#include <array>

// Les 8 directions autour d'une pierre, identiques à rules.py
static const int DR8[8] = {0, 0, 1, -1, 1, -1, 1, -1};
static const int DC8[8] = {1, -1, 0, 0, 1, -1, -1, 1};

MoveRecord apply_move(GomokuBoard& board, int row, int col) {
    MoveRecord record{};
    record.valid = false;

    if (!board.in_bounds(row, col)) {
        return record;
    }
    if (board.grid[row][col] != EMPTY) {
        return record;
    }

    int player = board.current_player;
    int opponent = board.opponent_of(player);

    // 1. On pose la pierre
    board.grid[row][col] = static_cast<Cell>(player);

    // 2. On cherche les captures dans les 8 directions
    record.num_captured = 0;

    for (int d = 0; d < 8; d++) {
        int dr = DR8[d], dc = DC8[d];
        int r1 = row + dr, c1 = col + dc;
        int r2 = row + dr * 2, c2 = col + dc * 2;
        int r3 = row + dr * 3, c3 = col + dc * 3;

        if (!board.in_bounds(r3, c3)) continue;

        if (board.grid[r1][c1] == opponent &&
            board.grid[r2][c2] == opponent &&
            board.grid[r3][c3] == player) {

            // Capture confirmée : on mémorise puis on retire
            int idx = record.num_captured;
            record.captured_r[idx] = r1;
            record.captured_c[idx] = c1;
            record.captured_val[idx] = board.grid[r1][c1];
            record.num_captured++;

            idx = record.num_captured;
            record.captured_r[idx] = r2;
            record.captured_c[idx] = c2;
            record.captured_val[idx] = board.grid[r2][c2];
            record.num_captured++;

            board.grid[r1][c1] = EMPTY;
            board.grid[r2][c2] = EMPTY;
        }
    }

    // 3. Mise à jour du score de captures
    board.captures[player] += record.num_captured;

    // 4. Finalisation du record
    record.row = row;
    record.col = col;
    record.player = player;
    record.valid = true;

    // 4bis. Empile ce coup dans l'historique (composante dynamique de
    // l'heuristique -- voir board.hpp)
    // Indexation CIRCULAIRE (modulo) : jamais de débordement, même si
    // history_count dépasse HISTORY_CAPACITY sur une partie très longue
    // avec beaucoup de captures/replays.
    board.history_row[board.history_count % GomokuBoard::HISTORY_CAPACITY] = row;
    board.history_col[board.history_count % GomokuBoard::HISTORY_CAPACITY] = col;
    board.history_count++;

    // 5. Passage au joueur suivant
    board.switch_player();

    return record;
}

void undo_move(GomokuBoard& board, const MoveRecord& record) {
    // 1. Retirer la pierre posée
    board.grid[record.row][record.col] = EMPTY;

    // 2. Restaurer les pierres capturées
    for (int i = 0; i < record.num_captured; i++) {
        board.grid[record.captured_r[i]][record.captured_c[i]] =
            static_cast<Cell>(record.captured_val[i]);
    }

    // 3. Annuler le score de capture
    board.captures[record.player] -= record.num_captured;

    // 3bis. Dépile l'historique (LIFO -- toujours le dernier coup joué)
    board.history_count--;

    // 4. Restaurer le joueur courant
    board.current_player = record.player;
}


// ──────────────────────────────────────────────────────────────────────
// DOUBLE-THREE
// ──────────────────────────────────────────────────────────────────────

// Les 4 axes possibles passant par une case (identiques à rules.py)
static const int AXES_DR[4] = {0, 1, 1, 1};
static const int AXES_DC[4] = {1, 0, 1, -1};

constexpr int REACH = 3;              // équivalent de reach=3 en Python
constexpr int WINDOW_LEN = 2 * REACH + 1;  // 7 caractères

// Extrait une fenêtre de 7 caractères centrée sur (row,col) le long de
// l'axe (dr,dc). 'X' = player, 'O' = opponent, '.' = vide, '#' = hors plateau.
static std::string extract_line(const GomokuBoard& board, int row, int col,
                                 int dr, int dc, int player, int opponent) {
    std::string s;
    s.reserve(WINDOW_LEN);
    for (int step = -REACH; step <= REACH; step++) {
        int r = row + dr * step;
        int c = col + dc * step;
        if (!board.in_bounds(r, c)) {
            s.push_back('#');
        } else {
            Cell v = board.grid[r][c];
            if (v == player) s.push_back('X');
            else if (v == opponent) s.push_back('O');
            else s.push_back('.');
        }
    }
    return s;
}

// Motifs "free-three" : bloc plein ou avec un trou, cf rules.py
static const std::string_view FREE_THREE_PATTERNS[4] = {
    ".XXX..", "..XXX.", ".XX.X.", ".X.XX."
};

// string_view accepte aussi bien un std::string (extract_line) qu'un
// buffer local sur la pile (is_double_three_direct) -- aucune allocation
// n'est nécessaire dans les deux cas, contrairement à std::string.
static bool is_free_three(std::string_view line, int center_index) {
    const int pattern_len = 6;
    for (std::string_view pattern : FREE_THREE_PATTERNS) {
        size_t start = 0;
        while (true) {
            size_t idx = line.find(pattern, start);
            if (idx == std::string_view::npos) break;
            if ((int)idx <= center_index && center_index < (int)(idx + pattern_len)) {
                return true;
            }
            start = idx + 1;
        }
    }
    return false;
}

// ──────────────────────────────────────────────────────────────────────
// TABLE DE LOOKUP PRÉCALCULÉE pour is_free_three
// ──────────────────────────────────────────────────────────────────────
//
// Constat clé : dans TOUS les appels réels du projet, la fenêtre fait
// toujours exactement 7 caractères et center_index vaut toujours 3.
// Avec seulement 4 symboles possibles ('.', 'X', 'O', '#'), on peut
// encoder chaque caractère sur 2 bits -- une fenêtre de 7 caractères
// tient alors sur 14 bits, soit 16 384 combinaisons possibles.
//
// On précalcule UNE FOIS (au premier appel) une table de 16 384 booléens
// donnant directement le résultat de is_free_three(fenêtre, 3) -- un
// simple accès tableau remplace ensuite toute la recherche de motifs.

static inline int encode_symbol(char c) {
    switch (c) {
        case '.': return 0;
        case 'X': return 1;
        case 'O': return 2;
        default:  return 3;  // '#'
    }
}

static inline int encode_window7(const char* buf) {
    int key = 0;
    for (int i = 0; i < 7; i++) {
        key = (key << 2) | encode_symbol(buf[i]);
    }
    return key;
}

static const char SYMBOL_TABLE[4] = {'.', 'X', 'O', '#'};

static std::array<bool, 16384> build_free_three_lookup() {
    std::array<bool, 16384> table{};
    char buf[7];
    for (int key = 0; key < 16384; key++) {
        int k = key;
        // On décode du bit de poids faible vers le fort -> ordre inverse
        for (int i = 6; i >= 0; i--) {
            buf[i] = SYMBOL_TABLE[k & 0b11];
            k >>= 2;
        }
        table[key] = is_free_three(std::string_view(buf, 7), REACH);
    }
    return table;
}

static const std::array<bool, 16384> FREE_THREE_LOOKUP = build_free_three_lookup();

// Version O(1) : encode la fenêtre en 14 bits, puis simple accès tableau.
static inline bool is_free_three_fast(const char* window7) {
    return FREE_THREE_LOOKUP[encode_window7(window7)];
}

// ──────────────────────────────────────────────────────────────────────
// VERSION DIRECTE : is_double_three sans construire de cache de lignes
// complètes du plateau.
// ──────────────────────────────────────────────────────────────────────
//
// Constat clé (suite au profiling) : reconstruire ~76 lignes complètes
// du plateau (build_line_caches) à CHAQUE nœud de Minimax est un travail
// O(size²) alors qu'on n'a besoin, pour chaque candidat testé, que d'une
// fenêtre de 7 cases par axe. Lire ces 7 cases directement depuis
// board.grid (avec la table de lookup déjà ultra-rapide) est bien moins
// coûteux que reconstruire tout le plateau pour n'en extraire qu'une
// petite fenêtre à la fin.
static bool is_double_three_direct(GomokuBoard& board, int row, int col, int player) {
    int opponent = board.opponent_of(player);
    char buf[7];
    int count = 0;

    for (int a = 0; a < 4; a++) {
        int dr = AXES_DR[a], dc = AXES_DC[a];
        for (int i = 0; i < 7; i++) {
            int step = i - REACH;
            if (step == 0) { buf[i] = 'X'; continue; }  // le coup qu'on simule
            int r = row + dr * step, c = col + dc * step;
            if (!board.in_bounds(r, c)) { buf[i] = '#'; continue; }
            Cell v = board.grid[r][c];
            buf[i] = (v == player) ? 'X' : (v == opponent ? 'O' : '.');
        }
        if (is_free_three_fast(buf)) {
            count++;
            if (count >= 2) return true;
        }
    }
    return count >= 2;
}

int count_free_threes(GomokuBoard& board, int row, int col, int early_stop_at) {
    int player = board.grid[row][col];
    int opponent = board.opponent_of(player);
    int count = 0;

    for (int a = 0; a < 4; a++) {
        std::string line = extract_line(board, row, col, AXES_DR[a], AXES_DC[a],
                                         player, opponent);
        if (is_free_three_fast(line.data())) {
            count++;
            if (early_stop_at > 0 && count >= early_stop_at) {
                return count;
            }
        }
    }
    return count;
}

bool is_double_three(GomokuBoard& board, int row, int col, int player) {
    // Pose directe, sans simuler les captures (comme en Python)
    Cell backup = board.grid[row][col];
    board.grid[row][col] = static_cast<Cell>(player);

    int nb_free_threes = count_free_threes(board, row, col, 2);

    board.grid[row][col] = backup;

    return nb_free_threes >= 2;
}


// ──────────────────────────────────────────────────────────────────────
// GÉNÉRATION DES COUPS LÉGAUX
// ──────────────────────────────────────────────────────────────────────

constexpr int NEIGHBOR_RADIUS = 2;

void get_legal_moves(GomokuBoard& board, int player, MoveList& out) {
    out.clear();

    bool has_stone = false;
    for (int r = 0; r < BOARD_SIZE && !has_stone; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board.grid[r][c] != EMPTY) { has_stone = true; break; }
        }
    }

    if (!has_stone) {
        out.add(BOARD_SIZE / 2, BOARD_SIZE / 2);
        return;
    }

    bool is_candidate[BOARD_SIZE][BOARD_SIZE] = {};

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board.grid[r][c] == EMPTY) continue;
            for (int dr = -NEIGHBOR_RADIUS; dr <= NEIGHBOR_RADIUS; dr++) {
                for (int dc = -NEIGHBOR_RADIUS; dc <= NEIGHBOR_RADIUS; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr, nc = c + dc;
                    if (board.in_bounds(nr, nc) && board.grid[nr][nc] == EMPTY) {
                        is_candidate[nr][nc] = true;
                    }
                }
            }
        }
    }

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (!is_candidate[r][c]) continue;
            if (is_double_three_direct(board, r, c, player)) continue;
            out.add(r, c);
        }
    }
}


// ──────────────────────────────────────────────────────────────────────
// LOOK-AHEAD : détection des victoires immédiates (lecture directe)
// ──────────────────────────────────────────────────────────────────────
//
// Comme pour is_double_three_direct : on lit directement board.grid pour
// chaque candidat (4 axes, comptage simple), plutôt que de scanner des
// lignes complètes précalculées -- moins de travail quand le nombre de
// candidats est petit devant la taille du plateau.
static bool creates_five(GomokuBoard& board, int row, int col, int player) {
    Cell backup = board.grid[row][col];
    board.grid[row][col] = static_cast<Cell>(player);

    bool won = false;
    for (int a = 0; a < 4; a++) {
        int dr = AXES_DR[a], dc = AXES_DC[a];
        int count = 1;
        int step = 1;
        while (true) {
            int r = row + dr*step, c = col + dc*step;
            if (board.in_bounds(r,c) && board.grid[r][c]==player) { count++; step++; }
            else break;
        }
        step = 1;
        while (true) {
            int r = row - dr*step, c = col - dc*step;
            if (board.in_bounds(r,c) && board.grid[r][c]==player) { count++; step++; }
            else break;
        }
        if (count >= 5) { won = true; break; }
    }

    board.grid[row][col] = backup;
    return won;
}

void get_legal_moves_and_threats(GomokuBoard& board, int player,
                                  MoveList& out_moves, ThreatCells& out_threats) {
    out_moves.clear();
    out_threats.num_my_wins = 0;
    out_threats.num_opp_wins = 0;

    bool has_stone = false;
    for (int r = 0; r < BOARD_SIZE && !has_stone; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board.grid[r][c] != EMPTY) { has_stone = true; break; }
        }
    }

    if (!has_stone) {
        out_moves.add(BOARD_SIZE / 2, BOARD_SIZE / 2);
        return;
    }

    bool is_candidate[BOARD_SIZE][BOARD_SIZE] = {};
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board.grid[r][c] == EMPTY) continue;
            for (int dr = -NEIGHBOR_RADIUS; dr <= NEIGHBOR_RADIUS; dr++) {
                for (int dc = -NEIGHBOR_RADIUS; dc <= NEIGHBOR_RADIUS; dc++) {
                    if (dr == 0 && dc == 0) continue;
                    int nr = r + dr, nc = c + dc;
                    if (board.in_bounds(nr, nc) && board.grid[nr][nc] == EMPTY) {
                        is_candidate[nr][nc] = true;
                    }
                }
            }
        }
    }

    int opponent = board.opponent_of(player);

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (!is_candidate[r][c]) continue;
            if (is_double_three_direct(board, r, c, player)) continue;
            out_moves.add(r, c);

            if (creates_five(board, r, c, player) && out_threats.num_my_wins < 16) {
                out_threats.my_wins_r[out_threats.num_my_wins] = r;
                out_threats.my_wins_c[out_threats.num_my_wins] = c;
                out_threats.num_my_wins++;
            }
            if (creates_five(board, r, c, opponent) && out_threats.num_opp_wins < 16) {
                out_threats.opp_wins_r[out_threats.num_opp_wins] = r;
                out_threats.opp_wins_c[out_threats.num_opp_wins] = c;
                out_threats.num_opp_wins++;
            }
        }
    }
}


// ──────────────────────────────────────────────────────────────────────
// ENDGAME CAPTURE / VICTOIRE
// ──────────────────────────────────────────────────────────────────────

static const int WIN_DR[4] = {0, 1, 1, 1};
static const int WIN_DC[4] = {1, 0, 1, -1};

// Petite structure fixe pour stocker les pierres d'un alignement (max
// nécessaire : BOARD_SIZE, un alignement ne peut pas dépasser la taille
// du plateau sur un axe).
struct AlignmentStones {
    int r[BOARD_SIZE];
    int c[BOARD_SIZE];
    int count;
};

// Retourne true si un alignement de 5+ a été trouvé, en remplissant 'out'
// avec les coordonnées ORDONNÉES des pierres de cet alignement.
static bool get_alignment_stones(GomokuBoard& board, int row, int col,
                                  int player, AlignmentStones& out) {
    for (int a = 0; a < 4; a++) {
        int dr = WIN_DR[a], dc = WIN_DC[a];

        // Comptage rapide d'abord (sans bookkeeping de tableaux) -- dans
        // l'immense majorité des appels (pas de victoire), on s'arrête
        // ici sans jamais construire de liste de coordonnées.
        int count = 1;
        int step = 1;
        while (true) {
            int r = row + dr * step, c = col + dc * step;
            if (board.in_bounds(r, c) && board.grid[r][c] == player) {
                count++; step++;
            } else break;
        }
        int forward_steps = step - 1;

        step = 1;
        while (true) {
            int r = row - dr * step, c = col - dc * step;
            if (board.in_bounds(r, c) && board.grid[r][c] == player) {
                count++; step++;
            } else break;
        }
        int backward_steps = step - 1;

        if (count >= 5) {
            // Chemin lent : on reconstruit la liste de coordonnées
            // SEULEMENT maintenant qu'on sait qu'elle est nécessaire.
            int idx = 0;
            for (int s = backward_steps; s >= 1; s--) {
                out.r[idx] = row - dr * s; out.c[idx] = col - dc * s; idx++;
            }
            out.r[idx] = row; out.c[idx] = col; idx++;
            for (int s = 1; s <= forward_steps; s++) {
                out.r[idx] = row + dr * s; out.c[idx] = col + dc * s; idx++;
            }
            out.count = idx;
            return true;
        }
    }
    return false;
}

// Vérifie si la paire de pierres adjacentes (r1,c1)-(r2,c2) est capturable
// au prochain coup de l'adversaire.
static bool is_pair_capturable(GomokuBoard& board, int r1, int c1, int r2, int c2,
                                int /*player*/, int opponent) {
    int dr = r2 - r1, dc = c2 - c1;
    int before_r = r1 - dr, before_c = c1 - dc;
    int after_r = r2 + dr, after_c = c2 + dc;

    if (!board.in_bounds(before_r, before_c) || !board.in_bounds(after_r, after_c)) {
        return false;
    }

    if (board.grid[before_r][before_c] == opponent && board.grid[after_r][after_c] == EMPTY) {
        return true;
    }
    if (board.grid[after_r][after_c] == opponent && board.grid[before_r][before_c] == EMPTY) {
        return true;
    }
    return false;
}

static const int ALL_8_DR[8] = {0, 0, 1, -1, 1, -1, 1, -1};
static const int ALL_8_DC[8] = {1, -1, 0, 0, 1, -1, -1, 1};

// Vérifie que l'alignement de 5+ ne peut pas être cassé par une capture
// adverse -- point clé découvert pendant le développement Python : la
// paire vulnérable peut être sur un AXE DIFFÉRENT de celui de l'alignement.
static bool is_winning_alignment_safe(GomokuBoard& board, int row, int col, int player) {
    AlignmentStones stones;
    if (!get_alignment_stones(board, row, col, player, stones)) {
        return false;
    }

    int opponent = board.opponent_of(player);

    for (int i = 0; i < stones.count; i++) {
        int r1 = stones.r[i], c1 = stones.c[i];
        for (int d = 0; d < 8; d++) {
            int dr = ALL_8_DR[d], dc = ALL_8_DC[d];
            int r2 = r1 + dr, c2 = c1 + dc;
            if (!board.in_bounds(r2, c2)) continue;
            if (board.grid[r2][c2] != player) continue;

            if (is_pair_capturable(board, r1, c1, r2, c2, player, opponent)) {
                return false;
            }
        }
    }
    return true;
}

bool check_win(GomokuBoard& board, int row, int col) {
    int player = board.grid[row][col];
    if (player == EMPTY) return false;

    // Condition 1 : victoire par capture (10 pierres = 5 paires)
    if (board.captures[player] >= 10) {
        return true;
    }

    // Condition 2 : victoire par alignement, sécurisée contre la capture
    AlignmentStones stones;
    if (get_alignment_stones(board, row, col, player, stones)) {
        if (is_winning_alignment_safe(board, row, col, player)) {
            return true;
        }
    }

    return false;
}


// ──────────────────────────────────────────────────────────────────────
// POTENTIEL DE CAPTURE
// ──────────────────────────────────────────────────────────────────────

// Les 4 axes non-redondants pour parcourir chaque paire de pierres
// adjacentes une seule fois (pas besoin des 8 directions ici, une paire
// (r,c)-(r+dr,c+dc) et son inverse (r+dr,c+dc)-(r,c) sont la même paire).
static const int PAIR_DR[4] = {0, 1, 1, 1};
static const int PAIR_DC[4] = {1, 0, 1, -1};

void count_potential_captures(GomokuBoard& board, int player,
                               int& out_opportunities, int& out_threats) {
    out_opportunities = 0;
    out_threats = 0;
    int opponent = board.opponent_of(player);

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            Cell v = board.grid[r][c];
            if (v == EMPTY) continue;

            for (int a = 0; a < 4; a++) {
                int dr = PAIR_DR[a], dc = PAIR_DC[a];
                int r2 = r + dr, c2 = c + dc;
                if (!board.in_bounds(r2, c2)) continue;
                if (board.grid[r2][c2] != v) continue;  // pas une paire de même couleur

                if (v == player) {
                    // Paire de 'player' -- menace si l'adversaire peut la capturer
                    if (is_pair_capturable(board, r, c, r2, c2, player, opponent)) {
                        out_threats++;
                    }
                } else {
                    // Paire adverse -- opportunité si 'player' peut la capturer
                    if (is_pair_capturable(board, r, c, r2, c2, opponent, player)) {
                        out_opportunities++;
                    }
                }
            }
        }
    }
}
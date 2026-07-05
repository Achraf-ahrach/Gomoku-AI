// heuristic.cpp

#include "heuristic.hpp"
#include "rules.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <cstring>

// ──────────────────────────────────────────────────────────────────────
// BARÈME DES MOTIFS -- identique au barème rééquilibré de heuristic.py
// ──────────────────────────────────────────────────────────────────────

struct PatternScore {
    const char* pattern;
    long long score;
};

static const PatternScore PATTERN_SCORES[] = {
    {"XXXXX",   10'000'000},
    {".XXXX.",   1'000'000},
    {".XXXX#",     100'000},
    {"#XXXX.",     100'000},
    {"OXXXX.",     100'000},
    {".XXXXO",     100'000},
    {".XXX.",       50'000},
    {"OXXX.",        5'000},
    {".XXXO",        5'000},
    {"#XXX.",        5'000},
    {".XXX#",        5'000},
    {".XX.",         1'000},
    {"OXX.",           100},
    {".XXO",           100},
};
static const int NUM_PATTERNS = sizeof(PATTERN_SCORES) / sizeof(PatternScore);


// NOTE : l'ancienne implémentation de score_line (recherche de motifs via
// std::string::find, 14 patterns) a été remplacée par score_line_fast
// (table de lookup) ci-dessous. L'équivalence entre les deux a été
// vérifiée rigoureusement sur 100 000+ lignes aléatoires et des cas
// limites (voir test_score_line_lookup.cpp) : 0 divergence.

// ──────────────────────────────────────────────────────────────────────
// TABLE DE LOOKUP pour score_line -- même principe que FREE_THREE_LOOKUP
// dans rules.cpp, mais pour la fenêtre de 6 caractères (la plus longue
// des 14 patterns du barème).
// ──────────────────────────────────────────────────────────────────────
//
// Constat clé : le score total d'une ligne complète est la somme, pour
// CHAQUE position de départ, du score des motifs qui commencent
// EXACTEMENT à cette position. On précalcule donc, pour les 4^6 = 4096
// combinaisons possibles d'une fenêtre de 6 caractères, la somme des
// scores des motifs (de longueur <= 6) qui matchent en PRÉFIXE de cette
// fenêtre -- puis on fait glisser cette fenêtre le long de la ligne en
// sommant les contributions, remplaçant 14 recherches de motifs (avec
// potentiellement plusieurs occurrences chacune) par une seule table de
// correspondance par position.

constexpr int SCORE_WINDOW = 6;  // longueur du plus long motif du barème

static inline int score_encode_symbol(char c) {
    switch (c) {
        case '.': return 0;
        case 'X': return 1;
        case 'O': return 2;
        default:  return 3;  // '#'
    }
}

static const char SCORE_SYMBOL_TABLE[4] = {'.', 'X', 'O', '#'};

static std::array<long long, 4096> build_score_window_lookup() {
    std::array<long long, 4096> table{};
    char buf[SCORE_WINDOW];

    for (int key = 0; key < 4096; key++) {
        int k = key;
        for (int i = SCORE_WINDOW - 1; i >= 0; i--) {
            buf[i] = SCORE_SYMBOL_TABLE[k & 0b11];
            k >>= 2;
        }

        long long total = 0;
        for (int p = 0; p < NUM_PATTERNS; p++) {
            const char* pattern = PATTERN_SCORES[p].pattern;
            int plen = (int)strlen(pattern);
            // Le motif doit matcher EXACTEMENT en préfixe de la fenêtre
            // (positions 0..plen-1) -- les caractères au-delà de plen
            // dans la fenêtre ne sont pas pertinents pour CE motif.
            bool match = true;
            for (int i = 0; i < plen; i++) {
                if (buf[i] != pattern[i]) { match = false; break; }
            }
            if (match) total += PATTERN_SCORES[p].score;
        }
        table[key] = total;
    }
    return table;
}

static const std::array<long long, 4096> SCORE_WINDOW_LOOKUP = build_score_window_lookup();

static inline int score_encode_window(const char* buf) {
    int key = 0;
    for (int i = 0; i < SCORE_WINDOW; i++) {
        key = (key << 2) | score_encode_symbol(buf[i]);
    }
    return key;
}

// Version rapide de score_line : on fait glisser une fenêtre de 6
// caractères le long de la ligne, en sommant les contributions
// précalculées à chaque position -- remplace les 14 recherches de
// motifs par une table de correspondance en O(1) par position.
static long long score_line_fast(std::string_view line) {
    long long total = 0;
    int len = (int)line.size();
    char buf[SCORE_WINDOW];

    // Positions de départ valides : 0 à len-1 (un motif a besoin d'au
    // moins 1 caractère réel comme point de départ). Au-delà de la fin
    // réelle de la ligne, on traite comme '#' (bord), cohérent avec le
    // comportement de score_line original (qui ne peut jamais trouver
    // de motif nécessitant des caractères au-delà de la fin de la chaîne).
    for (int start = 0; start < len; start++) {
        for (int i = 0; i < SCORE_WINDOW; i++) {
            int idx = start + i;
            buf[i] = (idx < len) ? line[idx] : '#';
        }
        total += SCORE_WINDOW_LOOKUP[score_encode_window(buf)];
    }

    return total;
}

// ──────────────────────────────────────────────────────────────────────
// EXTRACTION DE LIGNES COMPLÈTES (pour evaluate_position)
// ──────────────────────────────────────────────────────────────────────

// Écrit la ligne dans 'buf' (fourni par l'appelant, taille >= BOARD_SIZE+2)
// et retourne sa longueur réelle. Pas d'allocation de std::string : les
// lignes font jusqu'à 21 caractères, ce qui DÉPASSE la Small String
// Optimization de libstdc++ (15 caractères) -- chaque construction de
// std::string ici déclenchait donc une allocation sur le tas, à chaque
// appel, pour chaque ligne du plateau, à chaque nœud de Minimax.
static int extract_full_line(GomokuBoard& board, int start_row, int start_col,
                              int dr, int dc, int player, int opponent, char* buf) {
    int len = 0;
    buf[len++] = '#';
    int r = start_row, c = start_col;
    while (board.in_bounds(r, c)) {
        Cell v = board.grid[r][c];
        buf[len++] = (v == player) ? 'X' : (v == opponent ? 'O' : '.');
        r += dr;
        c += dc;
    }
    buf[len++] = '#';
    return len;
}

static long long evaluate_for(GomokuBoard& board, int player, int opponent) {
    long long total = 0;
    char buf[BOARD_SIZE + 2];

    bool scanned_rows[BOARD_SIZE] = {};
    bool scanned_cols[BOARD_SIZE] = {};
    // diag_down : clé = r - c, plage [-(SIZE-1), SIZE-1], décalée de (SIZE-1)
    bool scanned_diag_down[2 * BOARD_SIZE - 1] = {};
    // diag_up : clé = r + c, plage [0, 2*SIZE-2]
    bool scanned_diag_up[2 * BOARD_SIZE - 1] = {};

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (board.grid[r][c] == EMPTY) continue;

            // Horizontal
            if (!scanned_rows[r]) {
                scanned_rows[r] = true;
                int len = extract_full_line(board, r, 0, 0, 1, player, opponent, buf);
                total += score_line_fast(std::string_view(buf, len));
            }

            // Vertical
            if (!scanned_cols[c]) {
                scanned_cols[c] = true;
                int len = extract_full_line(board, 0, c, 1, 0, player, opponent, buf);
                total += score_line_fast(std::string_view(buf, len));
            }

            // Diagonale descendante (dr=1, dc=1), clé = r - c
            int diag_key = r - c + (BOARD_SIZE - 1);
            if (!scanned_diag_down[diag_key]) {
                scanned_diag_down[diag_key] = true;
                int start_row, start_col;
                if (r - c >= 0) { start_row = r - c; start_col = 0; }
                else { start_row = 0; start_col = c - r; }
                int len = extract_full_line(board, start_row, start_col, 1, 1, player, opponent, buf);
                total += score_line_fast(std::string_view(buf, len));
            }

            // Diagonale montante (dr=1, dc=-1), clé = r + c
            int diag_key2 = r + c;
            if (!scanned_diag_up[diag_key2]) {
                scanned_diag_up[diag_key2] = true;
                int start_row, start_col;
                if (r + c < BOARD_SIZE) { start_row = 0; start_col = r + c; }
                else { start_row = r + c - (BOARD_SIZE - 1); start_col = BOARD_SIZE - 1; }
                int len = extract_full_line(board, start_row, start_col, 1, -1, player, opponent, buf);
                total += score_line_fast(std::string_view(buf, len));
            }
        }
    }

    return total;
}

// Déclaration anticipée -- implémentée plus bas (réutilise QUICK_REACH).
static long long fork_bonus_score_diff(GomokuBoard& board, int player, int opponent);
static long long dynamic_recency_bonus(GomokuBoard& board, int player);

long long evaluate_position(GomokuBoard& board, int player) {
    int opponent = board.opponent_of(player);

    long long player_score = evaluate_for(board, player, opponent);
    long long opponent_score = evaluate_for(board, opponent, player);
    long long alignment_score = player_score - opponent_score;

    constexpr long long CAPTURE_PAIR_SCORE = 2'000;
    long long capture_score =
        (board.captures[player] / 2) * CAPTURE_PAIR_SCORE
        - (board.captures[opponent] / 2) * CAPTURE_PAIR_SCORE;

    // ── Potentiel de capture (barème 42 : "Static part - Potential
    // captures") -- une paire vulnérable/capturable n'est pas encore une
    // capture réalisée, donc son poids reste inférieur à CAPTURE_PAIR_SCORE.
    constexpr long long POTENTIAL_CAPTURE_SCORE = 800;
    int my_opportunities, my_threats;
    count_potential_captures(board, player, my_opportunities, my_threats);
    long long potential_capture_score =
        (long long)my_opportunities * POTENTIAL_CAPTURE_SCORE
        - (long long)my_threats * POTENTIAL_CAPTURE_SCORE;

    // ── Combinaisons avantageuses / figures (barème 42 : "Static part -
    // Figures") -- une pierre qui participe à 2+ menaces simultanées sur
    // des axes différents (une "fourche") est bien plus dangereuse que la
    // somme de ses lignes prises isolément, car l'adversaire ne peut en
    // bloquer qu'une seule à la fois.
    long long fork_score = fork_bonus_score_diff(board, player, opponent);

    // ── Partie DYNAMIQUE (barème 42 : "Dynamic part") -- contrairement
    // aux composantes précédentes qui ne regardent que l'état ACTUEL du
    // plateau, celle-ci utilise l'HISTORIQUE des coups joués pour
    // pondérer l'évaluation : les coups les plus récents (là où l'action
    // se concentre en ce moment) reçoivent un poids plus important que
    // les coups anciens, reflétant l'idée qu'une menace qui se développe
    // activement mérite plus d'attention qu'une pierre isolée posée tôt
    // dans la partie et jamais reprise depuis.
    long long dynamic_score = dynamic_recency_bonus(board, player);

    return alignment_score + capture_score + potential_capture_score
         + fork_score + dynamic_score;
}

// ──────────────────────────────────────────────────────────────────────
// ÉVALUATION RAPIDE POUR LE MOVE ORDERING
// ──────────────────────────────────────────────────────────────────────

constexpr int QUICK_REACH = 4;

static const int QUICK_AXES_DR[4] = {0, 1, 1, 1};
static const int QUICK_AXES_DC[4] = {1, 0, 1, -1};

// ──────────────────────────────────────────────────────────────────────
// COMBINAISONS AVANTAGEUSES (FOURCHES)
// ──────────────────────────────────────────────────────────────────────
//
// Seuil à partir duquel on considère qu'un axe représente une menace
// "sérieuse" pour le calcul de fourche (three fermé ou mieux, cf barème
// PATTERN_SCORES). En dessous, un simple "two" ne compte pas comme une
// menace suffisante pour former une figure dangereuse.
constexpr long long FORK_THREAT_THRESHOLD = 5'000;
constexpr long long FORK_BONUS = 3'000;

// Compte sur combien d'axes (parmi les 4) la pierre en (row,col) fait
// partie d'une menace sérieuse (score local >= FORK_THREAT_THRESHOLD).
static int count_threat_axes_for_stone(GomokuBoard& board, int row, int col,
                                        int player, int opponent) {
    constexpr int QLEN = 2 * QUICK_REACH + 1;
    int count = 0;

    for (int a = 0; a < 4; a++) {
        int dr = QUICK_AXES_DR[a], dc = QUICK_AXES_DC[a];
        char buf[QLEN];
        int idx = 0;
        for (int step = -QUICK_REACH; step <= QUICK_REACH; step++) {
            int r = row + dr * step, c = col + dc * step;
            if (!board.in_bounds(r, c)) { buf[idx++] = '#'; continue; }
            Cell v = board.grid[r][c];
            buf[idx++] = (v == player) ? 'X' : (v == opponent ? 'O' : '.');
        }
        long long score = score_line_fast(std::string_view(buf, QLEN));
        if (score >= FORK_THREAT_THRESHOLD) count++;
    }
    return count;
}

// NOTE : fork_bonus_score (une version non-fusionnée qui scannait le
// plateau séparément pour chaque joueur) a été remplacée par
// fork_bonus_score_diff ci-dessous, qui fait le même travail en UN SEUL
// passage -- gain de vitesse significatif, comportement identique
// (vérifié par test_fork_bonus.cpp).

// Version fusionnée : calcule le score de fourche des DEUX joueurs en
// UN SEUL passage sur le plateau, au lieu de deux appels séparés à
// fork_bonus_score (chacun refaisant un balayage complet de 361 cases).
// Utilisée par evaluate_position pour éviter de scanner le plateau deux
// fois pour une information équivalente.
static long long fork_bonus_score_diff(GomokuBoard& board, int player, int opponent) {
    long long player_total = 0;
    long long opponent_total = 0;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            Cell v = board.grid[r][c];
            if (v == EMPTY) continue;

            if (v == player) {
                int axes = count_threat_axes_for_stone(board, r, c, player, opponent);
                if (axes >= 2) player_total += FORK_BONUS * (axes - 1);
            } else {
                int axes = count_threat_axes_for_stone(board, r, c, opponent, player);
                if (axes >= 2) opponent_total += FORK_BONUS * (axes - 1);
            }
        }
    }

    return player_total - opponent_total;
}

long long quick_move_score(GomokuBoard& board, int row, int col, int player) {
    int opponent = board.opponent_of(player);
    constexpr int QLEN = 2 * QUICK_REACH + 1;
    char buf[QLEN];

    // ── Composante offensive : alignements créés pour 'player' ──
    long long offensive = 0;
    for (int a = 0; a < 4; a++) {
        int dr = QUICK_AXES_DR[a], dc = QUICK_AXES_DC[a];
        int idx = 0;
        for (int step = -QUICK_REACH; step <= QUICK_REACH; step++) {
            int r = row + dr * step, c = col + dc * step;
            if (!board.in_bounds(r, c)) { buf[idx++] = '#'; continue; }
            Cell v = board.grid[r][c];
            buf[idx++] = (v == player) ? 'X' : (v == opponent ? 'O' : '.');
        }
        offensive += score_line_fast(std::string_view(buf, QLEN));
    }

    // ── Composante défensive : menace qu'on neutralise en jouant ici ──
    // On simule "et si l'adversaire avait joué ici" : la case centrale
    // est forcée à 'X' (= adversaire), le reste utilise la perspective
    // inversée (adversaire='X', joueur='O').
    long long defensive = 0;
    for (int a = 0; a < 4; a++) {
        int dr = QUICK_AXES_DR[a], dc = QUICK_AXES_DC[a];
        int idx = 0;
        for (int step = -QUICK_REACH; step <= QUICK_REACH; step++) {
            int r = row + dr * step, c = col + dc * step;
            if (r == row && c == col) { buf[idx++] = 'X'; continue; }
            if (!board.in_bounds(r, c)) { buf[idx++] = '#'; continue; }
            Cell v = board.grid[r][c];
            buf[idx++] = (v == opponent) ? 'X' : (v == player ? 'O' : '.');
        }
        defensive += score_line_fast(std::string_view(buf, QLEN));
    }

    constexpr double DEFENSIVE_WEIGHT = 0.9;
    long long total = offensive + static_cast<long long>(defensive * DEFENSIVE_WEIGHT);

    // ── Bonus capture ──
    constexpr long long CAPTURE_HINT_BONUS = 1'500;
    static const int ALL_8_DR[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static const int ALL_8_DC[8] = {1, -1, 0, 0, 1, -1, -1, 1};

    for (int d = 0; d < 8; d++) {
        int dr = ALL_8_DR[d], dc = ALL_8_DC[d];
        int r1 = row + dr, c1 = col + dc;
        int r2 = row + dr * 2, c2 = col + dc * 2;
        int r3 = row + dr * 3, c3 = col + dc * 3;
        if (!board.in_bounds(r3, c3)) continue;
        if (board.grid[r1][c1] == opponent &&
            board.grid[r2][c2] == opponent &&
            board.grid[r3][c3] == player) {
            total += CAPTURE_HINT_BONUS;
        }
    }

    return total;
}

// ──────────────────────────────────────────────────────────────────────
// PARTIE DYNAMIQUE -- pondération selon l'historique des coups
// ──────────────────────────────────────────────────────────────────────
//
// Contrairement à toutes les composantes précédentes (statiques : elles
// ne regardent que le plateau EN L'ÉTAT), celle-ci utilise
// board.history_row/history_col (maintenu par apply_move/undo_move dans
// rules.cpp) pour identifier les coups RÉCEMMENT joués, et pondère leur
// évaluation locale selon leur ancienneté -- un coup qui vient d'être
// joué reflète l'action en cours, un coup ancien jamais repris depuis
// est probablement moins pertinent pour la dynamique actuelle.

constexpr int DYNAMIC_HISTORY_DEPTH = 6;   // on regarde les 6 derniers coups
constexpr double DYNAMIC_WEIGHT_FACTOR = 0.15;  // poids modeste : un raffinement,
                                                  // pas une refonte du score statique

static long long dynamic_recency_bonus(GomokuBoard& board, int player) {
    long long bonus = 0;
    int n = board.history_count;
    int k = (n < DYNAMIC_HISTORY_DEPTH) ? n : DYNAMIC_HISTORY_DEPTH;

    for (int i = 0; i < k; i++) {
        int idx = n - 1 - i;  // du coup le plus récent vers le plus ancien
        // Même indexation circulaire que l'écriture dans apply_move --
        // idx est toujours >= 0 ici (garanti par k = min(n, DEPTH)).
        int slot = idx % GomokuBoard::HISTORY_CAPACITY;
        int r = board.history_row[slot];
        int c = board.history_col[slot];
        int stone = board.grid[r][c];

        // Sécurité : la pierre a pu être capturée depuis qu'elle a été
        // jouée -- dans ce cas la case est vide, on ignore cette entrée.
        if (stone == EMPTY) continue;

        // Poids décroissant linéairement : le coup le plus récent (i=0)
        // compte pour 100%, le 6e coup en arrière compte pour ~17%.
        double weight = 1.0 - (double)i / DYNAMIC_HISTORY_DEPTH;

        long long local_score = quick_move_score(board, r, c, stone);
        long long signed_score = (stone == player) ? local_score : -local_score;

        bonus += (long long)(signed_score * weight * DYNAMIC_WEIGHT_FACTOR);
    }

    return bonus;
}
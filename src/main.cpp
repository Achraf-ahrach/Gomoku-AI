// main.cpp
//
// Boucle de jeu principale, équivalent C++ de main.py.

#include "board.hpp"
#include "rules.hpp"
#include "ai.hpp"
#include "ui.hpp"
#include "config.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Gomoku AI - C++");
    window.setFramerateLimit(60);

    GameUI ui;
    SettingsState settings;

    enum GameState { MENU, PLAYING };
    GameState game_state = MENU;

    GomokuBoard board;
    bool game_over = false;
    std::string winner_msg;
    int human_player = PLAYER_ONE;
    int computer_player = PLAYER_TWO;

    // Dernier résultat de l'IA, pour l'affichage du timer (obligatoire)
    bool has_ai_result = false;
    double last_ai_time_ms = 0.0;
    int last_ai_depth = 0;

    // Suggestion de coup pour le mode Humain vs Humain -- exigence
    // OBLIGATOIRE du sujet (Chapitre III) : "Against another human player
    // ... but with a move-suggestion feature." On réutilise ai_get_best_move,
    // mais on affiche juste le coup suggéré SANS le jouer automatiquement --
    // le joueur reste libre de suivre la suggestion ou de jouer ailleurs.
    bool has_suggestion = false;
    int suggested_row = -1, suggested_col = -1;

    auto reset_game_from_menu = [&]() {
        board = GomokuBoard();
        game_over = false;
        winner_msg.clear();
        has_ai_result = false;
        has_suggestion = false;
        if (settings.play_against == "Computer") {
            human_player = (settings.play_as == "White") ? PLAYER_ONE : PLAYER_TWO;
            computer_player = (human_player == PLAYER_ONE) ? PLAYER_TWO : PLAYER_ONE;
        } else {
            human_player = PLAYER_ONE;
            computer_player = PLAYER_TWO;
        }
    };

    auto resolve_move = [&](int row, int col) -> bool {
        MoveRecord record = apply_move(board, row, col);
        if (!record.valid) return false;

        // Un coup vient d'être joué -- la suggestion précédente n'est
        // plus pertinente pour la nouvelle position.
        has_suggestion = false;

        if (check_win(board, row, col)) {
            game_over = true;
            std::string winner_color = GameUI::player_color_name(record.player);
            if (board.captures[record.player] >= 10) {
                winner_msg = winner_color + " wins by capture (10 stones)!";
            } else {
                winner_msg = winner_color + " wins by alignment!";
            }
        } else {
            // Détection de match nul : si le joueur suivant n'a plus AUCUN
            // coup légal (plateau plein, ou tous les coups restants seraient
            // des double-three), on arrête la partie proprement plutôt que
            // de rester "bloqué" à essayer indéfiniment de faire jouer un
            // joueur qui n'a plus d'option -- un vrai risque de blocage
            // perçu (pas un crash, mais un mauvais comportement à éviter).
            MoveList remaining;
            get_legal_moves(board, board.current_player, remaining);
            if (remaining.count == 0) {
                game_over = true;
                winner_msg = "Draw! No legal moves remain.";
            }
        }
        // apply_move a déjà fait switch_player() en interne (comme
        // place_stone le fait en Python) -- rien d'autre à faire ici.
        return true;
    };

    auto request_suggestion = [&]() {
        // Suggère un coup pour le joueur dont c'est ACTUELLEMENT le tour,
        // sans jamais le jouer -- c'est au joueur humain de décider.
        if (game_over) return;
        AIResult result = ai_get_best_move(board, 10, 0.5);
        if (!result.valid) return;
        has_suggestion = true;
        suggested_row = result.row;
        suggested_col = result.col;
    };

    auto run_computer_turn = [&]() {
        if (game_over || settings.play_against != "Computer") return;
        if (board.current_player != computer_player) return;

        double time_limit = 0.5;
        int max_depth = 10;
        if (settings.difficulty == "Easy")   { time_limit = 0.1; max_depth = 2; }
        if (settings.difficulty == "Medium") { time_limit = 0.3; max_depth = 6; }
        if (settings.difficulty == "Hard")   { time_limit = 0.5; max_depth = 10; }

        AIResult result = ai_get_best_move(board, max_depth, time_limit);
        if (!result.valid) return;

        last_ai_time_ms = result.time_ms;
        last_ai_depth = result.depth_reached;
        has_ai_result = true;

        resolve_move(result.row, result.col);
    };

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mouse_pos((float)event.mouseButton.x, (float)event.mouseButton.y);

                if (game_state == MENU) {
                    for (auto& hb : ui.hitbox_play_against) {
                        if (hb.rect.contains(mouse_pos)) {
                            settings.play_against = hb.value;
                            if (settings.play_against == "Human") {
                                settings.play_as = "White";
                                settings.difficulty = "Medium";
                            }
                        }
                    }
                    if (settings.play_against == "Computer") {
                        for (auto& hb : ui.hitbox_play_as) {
                            if (hb.rect.contains(mouse_pos)) settings.play_as = hb.value;
                        }
                        for (auto& hb : ui.hitbox_difficulty) {
                            if (hb.rect.contains(mouse_pos)) settings.difficulty = hb.value;
                        }
                    }
                    if (ui.hitbox_confirm.contains(mouse_pos)) {
                        reset_game_from_menu();
                        game_state = PLAYING;
                    } else if (ui.hitbox_cancel.contains(mouse_pos)) {
                        window.close();
                    }
                } else if (game_state == PLAYING && !game_over) {
                    bool is_computer_turn = (settings.play_against == "Computer" &&
                                              board.current_player == computer_player);

                    // Clic sur le bouton "Suggest Move" -- uniquement en
                    // mode Humain vs Humain (obligatoire selon le sujet).
                    if (settings.play_against == "Human" &&
                        ui.hitbox_suggest.contains(mouse_pos)) {
                        request_suggestion();
                    } else if (!is_computer_turn) {
                        int mx = event.mouseButton.x, my = event.mouseButton.y;
                        int adjusted_y = my - TOP_PANEL;
                        int c = (int)std::round((mx - MARGIN) / (double)CELL_SIZE);
                        int r = (int)std::round((adjusted_y - MARGIN) / (double)CELL_SIZE);

                        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                            if (settings.play_against == "Human" || board.current_player == human_player) {
                                resolve_move(r, c);
                            }
                        }
                    }
                }
            }
        }

        if (game_state == PLAYING && !game_over && settings.play_against == "Computer" &&
            board.current_player == computer_player) {
            run_computer_turn();
        }

        if (game_state == MENU) {
            sf::Vector2i mp = sf::Mouse::getPosition(window);
            ui.draw_settings_menu(window, settings, sf::Vector2f((float)mp.x, (float)mp.y));
        } else {
            ui.render_board_background(window, true, &board);
            ui.draw_hud(window, board, game_over, winner_msg, settings, human_player,
                        has_ai_result, last_ai_time_ms, last_ai_depth);

            // Bouton de suggestion -- visible uniquement en mode Humain
            // vs Humain (exigence obligatoire du sujet).
            if (settings.play_against == "Human") {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                ui.hitbox_suggest = ui.draw_suggest_button(
                    window, sf::Vector2f((float)mp.x, (float)mp.y), !game_over);
            }

            if (has_suggestion) {
                ui.draw_suggestion_marker(window, suggested_row, suggested_col);
            }
        }

        window.display();
    }

    return 0;
}
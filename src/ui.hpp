// ui.hpp
//
// Portage C++ de ui_components.py : rendu du plateau, menu de réglages,
// HUD (timer, captures, tour actuel).

#pragma once
#include "board.hpp"
#include "config.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct SettingsState {
    std::string play_against = "Computer";  // "Computer" ou "Human"
    std::string play_as = "White";          // "White" ou "Black"
    std::string difficulty = "Medium";      // "Easy", "Medium", "Hard"
};

// Une zone cliquable associée à une valeur (ex: "Easy" -> rectangle).
struct Hitbox {
    std::string value;
    sf::FloatRect rect;
};

class GameUI {
public:
    GameUI();

    void render_board_background(sf::RenderWindow& window, bool draw_stones, const GomokuBoard* board);
    void draw_settings_menu(sf::RenderWindow& window, const SettingsState& settings, sf::Vector2f mouse_pos);
    void draw_hud(sf::RenderWindow& window, const GomokuBoard& board, bool game_over,
                  const std::string& winner_msg, const SettingsState& settings, int human_player,
                  bool show_ai_timer, double ai_time_ms, int ai_depth);

    // Popup affiché en fin de partie avec un bouton pour quitter.
    sf::FloatRect draw_game_over_popup(sf::RenderWindow& window, const std::string& winner_msg,
                                       sf::Vector2f mouse_pos);

    // Dessine un marqueur visuel (anneau coloré) sur la case suggérée par
    // l'IA, sans jouer le coup -- le joueur reste libre de le suivre ou non.
    void draw_suggestion_marker(sf::RenderWindow& window, int row, int col);

    // Zones cliquables du menu, remplies par draw_settings_menu, lues par main.cpp
    std::vector<Hitbox> hitbox_play_against;
    std::vector<Hitbox> hitbox_play_as;
    std::vector<Hitbox> hitbox_difficulty;
    sf::FloatRect hitbox_confirm;
    sf::FloatRect hitbox_cancel;
    sf::FloatRect hitbox_exit;

    static std::string player_color_name(int player_id);
    static sf::Color player_stone_color(int player_id);

private:
    sf::Font font_;
    bool font_loaded_;

    void draw_text(sf::RenderWindow& window, const std::string& text, float x, float y,
                   unsigned int size, sf::Color color, bool bold = true);
    sf::FloatRect draw_button(sf::RenderWindow& window, sf::FloatRect rect, const std::string& text,
                               bool selected, bool enabled, bool hovered);
    std::vector<sf::FloatRect> draw_segmented_row(sf::RenderWindow& window, const std::string& label,
                                                   const std::vector<std::string>& options,
                                                   const std::string& selected_value,
                                                   sf::FloatRect row_rect, bool enabled,
                                                   sf::Vector2f mouse_pos);
};
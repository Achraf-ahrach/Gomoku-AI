// ui.cpp

#include "ui.hpp"
#include <iostream>

GameUI::GameUI() : font_loaded_(false) {
    // On essaie plusieurs chemins de police courants, pour rester portable
    // entre Linux (dev), Windows et macOS (machine de l'équipière).
    const char* font_paths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Verdana.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNS.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "assets/font.ttf",  // police fournie avec le projet, en secours
    };

    for (const char* path : font_paths) {
        if (font_.openFromFile(path)) {
            font_loaded_ = true;
            break;
        }
    }

    if (!font_loaded_) {
        std::cerr << "[UI] Attention : aucune police trouvee, le texte ne s'affichera pas.\n";
    }
}

void GameUI::draw_text(sf::RenderWindow& window, const std::string& text, float x, float y,
                        unsigned int size, sf::Color color, bool bold) {
    if (!font_loaded_) return;
    sf::Text t(font_, text, size);
    t.setCharacterSize(size);
    t.setFillColor(color);
    t.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
    t.setPosition({x, y});
    window.draw(t);
}

std::string GameUI::player_color_name(int player_id) {
    return (player_id == PLAYER_ONE) ? "White" : "Black";
}

sf::Color GameUI::player_stone_color(int player_id) {
    return (player_id == PLAYER_ONE) ? COLOR_WHITE_STONE : COLOR_BLACK_STONE;
}

void GameUI::render_board_background(sf::RenderWindow& window, bool draw_stones,
                                      const GomokuBoard* board) {
    window.clear(COLOR_WHITE);

    sf::RectangleShape board_rect(sf::Vector2f((float)WINDOW_WIDTH, (float)(WINDOW_HEIGHT - TOP_PANEL)));
    board_rect.setPosition({0.f, (float)TOP_PANEL});
    board_rect.setFillColor(COLOR_WOOD);
    window.draw(board_rect);

    // Lignes du plateau
    for (int i = 0; i < BOARD_SIZE; i++) {
        sf::Vertex hline[] = {
            {{(float)MARGIN, (float)(TOP_PANEL + MARGIN + i * CELL_SIZE)}, COLOR_LINE},
            {{(float)(WINDOW_WIDTH - MARGIN), (float)(TOP_PANEL + MARGIN + i * CELL_SIZE)}, COLOR_LINE},
        };
        window.draw(hline, 2, sf::PrimitiveType::Lines);

        sf::Vertex vline[] = {
            {{(float)(MARGIN + i * CELL_SIZE), (float)(TOP_PANEL + MARGIN)}, COLOR_LINE},
            {{(float)(MARGIN + i * CELL_SIZE), (float)(WINDOW_HEIGHT - MARGIN)}, COLOR_LINE},
        };
        window.draw(vline, 2, sf::PrimitiveType::Lines);
    }

    // Points d'etoile (hoshi)
    int star_points[] = {3, 9, 15};
    for (int r : star_points) {
        for (int c : star_points) {
            sf::CircleShape dot(3.f);
            dot.setFillColor(COLOR_LINE);
            dot.setOrigin({3.f, 3.f});
            dot.setPosition({(float)(MARGIN + c * CELL_SIZE), (float)(TOP_PANEL + MARGIN + r * CELL_SIZE)});
            window.draw(dot);
        }
    }

    if (!draw_stones || board == nullptr) return;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            int cell_value = board->grid[r][c];
            if (cell_value == EMPTY) continue;

            float cx = (float)(MARGIN + c * CELL_SIZE);
            float cy = (float)(TOP_PANEL + MARGIN + r * CELL_SIZE);

            sf::CircleShape stone(16.f);
            stone.setOrigin({16.f, 16.f});
            stone.setPosition({cx, cy});
            stone.setFillColor(player_stone_color(cell_value));
            sf::Color outline = (cell_value == PLAYER_ONE) ? sf::Color(35, 35, 35) : sf::Color(240, 240, 240);
            stone.setOutlineColor(outline);
            stone.setOutlineThickness(1.f);
            window.draw(stone);
        }
    }
}

sf::FloatRect GameUI::draw_button(sf::RenderWindow& window, sf::FloatRect rect, const std::string& text,
                                   bool selected, bool enabled, bool hovered) {
    sf::Color button_color = selected ? COLOR_BUTTON_SELECTED : COLOR_BUTTON;
    if (hovered || !enabled) {
        button_color = sf::Color(
            std::min(255, button_color.r + 8),
            std::min(255, button_color.g + 8),
            std::min(255, button_color.b + 8)
        );
    }

    sf::RectangleShape box(sf::Vector2f(rect.size.x, rect.size.y));
    box.setPosition(rect.position);
    box.setFillColor(button_color);
    box.setOutlineColor(sf::Color(182, 155, 120));
    box.setOutlineThickness(2.f);
    window.draw(box);

    if (font_loaded_) {
        sf::Text label(font_, text, 16);
        label.setFillColor(enabled ? COLOR_BUTTON_TEXT : COLOR_DISABLED_TEXT);
        label.setStyle(sf::Text::Bold);
        sf::FloatRect bounds = label.getLocalBounds();
        label.setPosition({rect.position.x + (rect.size.x - bounds.size.x) / 2.f - bounds.position.x,
                           rect.position.y + (rect.size.y - bounds.size.y) / 2.f - bounds.position.y});
        window.draw(label);
    }

    return rect;
}

std::vector<sf::FloatRect> GameUI::draw_segmented_row(
        sf::RenderWindow& window, const std::string& label,
        const std::vector<std::string>& options, const std::string& selected_value,
        sf::FloatRect row_rect, bool enabled, sf::Vector2f mouse_pos) {

    draw_text(window, label, row_rect.position.x, row_rect.position.y + row_rect.size.y / 2.f - 10.f,
              18, enabled ? COLOR_TEXT : COLOR_DISABLED_TEXT);

    float label_width = 180.f;
    float button_area_x = row_rect.position.x + label_width + 12.f;
    float button_area_width = row_rect.position.x + row_rect.size.x - button_area_x;
    float gap = 10.f;
    int count = (int)options.size();
    float button_width = (button_area_width - gap * (count - 1)) / count;
    float button_height = 46.f;
    float button_y = row_rect.position.y + row_rect.size.y / 2.f - button_height / 2.f;

    std::vector<sf::FloatRect> rects;
    for (int i = 0; i < count; i++) {
        sf::FloatRect r({button_area_x + i * (button_width + gap), button_y}, {button_width, button_height});
        bool is_selected = (options[i] == selected_value);
        bool is_hovered = enabled && r.contains(mouse_pos);
        draw_button(window, r, options[i], is_selected, enabled, is_hovered);
        rects.push_back(r);
    }
    return rects;
}

void GameUI::draw_settings_menu(sf::RenderWindow& window, const SettingsState& settings,
                                 sf::Vector2f mouse_pos) {
    render_board_background(window, false, nullptr);

    sf::RectangleShape overlay(sf::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
    overlay.setFillColor(sf::Color(255, 255, 255, 18));
    window.draw(overlay);

    sf::FloatRect card_rect({90.f, 42.f}, {(float)WINDOW_WIDTH - 180.f, 420.f});
    sf::RectangleShape card(sf::Vector2f(card_rect.size.x, card_rect.size.y));
    card.setPosition(card_rect.position);
    card.setFillColor(COLOR_CARD);
    card.setOutlineColor(COLOR_CARD_EDGE);
    card.setOutlineThickness(4.f);
    window.draw(card);

    draw_text(window, "Game Settings", card_rect.position.x + card_rect.size.x / 2.f - 90.f,
              card_rect.position.y + 20.f, 24, COLOR_TEXT);
    draw_text(window, "Choose how you want to play before starting.",
              card_rect.position.x + card_rect.size.x / 2.f - 160.f, card_rect.position.y + 58.f,
              14, sf::Color(92, 68, 42));

    float row_top = card_rect.position.y + 78.f;
    float row_left = card_rect.position.x + 28.f;
    float row_width = card_rect.size.x - 56.f;
    float row_height = 68.f;
    float row_gap = 10.f;

    sf::FloatRect row_play_against({row_left, row_top}, {row_width, row_height});
    sf::FloatRect row_play_as({row_left, row_top + (row_height + row_gap)}, {row_width, row_height});
    sf::FloatRect row_difficulty({row_left, row_top + 2 * (row_height + row_gap)}, {row_width, row_height});
    hitbox_play_against.clear();
    hitbox_play_as.clear();
    hitbox_difficulty.clear();

    std::vector<std::string> play_against_opts = {"Computer", "Human"};
    auto rects1 = draw_segmented_row(window, "Play Against", play_against_opts,
                                      settings.play_against, row_play_against, true, mouse_pos);
    for (size_t i = 0; i < play_against_opts.size(); i++) {
        hitbox_play_against.push_back({play_against_opts[i], rects1[i]});
    }

    bool play_as_enabled = (settings.play_against == "Computer");
    std::vector<std::string> play_as_opts = {"White", "Black"};
    auto rects2 = draw_segmented_row(window, "Play As", play_as_opts,
                                      settings.play_as, row_play_as, play_as_enabled, mouse_pos);
    for (size_t i = 0; i < play_as_opts.size(); i++) {
        hitbox_play_as.push_back({play_as_opts[i], rects2[i]});
    }

    std::vector<std::string> difficulty_opts = {"Easy", "Medium", "Hard"};
    auto rects3 = draw_segmented_row(window, "Difficulty", difficulty_opts,
                                      settings.difficulty, row_difficulty, play_as_enabled, mouse_pos);
    for (size_t i = 0; i < difficulty_opts.size(); i++) {
        hitbox_difficulty.push_back({difficulty_opts[i], rects3[i]});
    }

    float button_row_top = row_top + 3 * (row_height + row_gap) + 10.f;
    sf::FloatRect confirm_rect({row_left, button_row_top}, {row_width / 2.f - 10.f, 58.f});
    sf::FloatRect cancel_rect({row_left + row_width / 2.f + 10.f, button_row_top}, {row_width / 2.f - 10.f, 58.f});

    hitbox_confirm = confirm_rect;
    hitbox_cancel = cancel_rect;

    draw_button(window, confirm_rect, "Confirm", true, true, confirm_rect.contains(mouse_pos));

    sf::RectangleShape cancel_box(sf::Vector2f(cancel_rect.size.x, cancel_rect.size.y));
    cancel_box.setPosition(cancel_rect.position);
    cancel_box.setFillColor(COLOR_CANCEL);
    cancel_box.setOutlineColor(sf::Color(112, 56, 50));
    cancel_box.setOutlineThickness(2.f);
    window.draw(cancel_box);
    draw_text(window, "Cancel", cancel_rect.position.x + cancel_rect.size.x / 2.f - 30.f,
              cancel_rect.position.y + cancel_rect.size.y / 2.f - 10.f, 18, COLOR_WHITE);
}

void GameUI::draw_hud(sf::RenderWindow& window, const GomokuBoard& board, bool game_over,
                       const std::string& winner_msg, const SettingsState& settings, int human_player,
                       bool show_ai_timer, double ai_time_ms, int ai_depth) {
    std::string mode_label;
    if (settings.play_against == "Computer") {
        mode_label = "Mode: Computer | You: " + settings.play_as + " | Difficulty: " + settings.difficulty;
    } else {
        mode_label = "Mode: Human vs Human";
    }

    if (!game_over) {
        std::string turn_side = (board.current_player == PLAYER_ONE) ? "White" : "Black";
        std::string turn_text;
        if (settings.play_against == "Computer") {
            std::string owner = (board.current_player == human_player) ? "You" : "Computer";
            turn_text = "Turn: " + turn_side + " (" + owner + ")";
        } else {
            turn_text = "Turn: " + turn_side;
        }
        draw_text(window, turn_text, (float)MARGIN, 10.f, 20, COLOR_TEXT);
        draw_text(window, mode_label, (float)MARGIN, 34.f, 14, COLOR_TEXT);
    } else {
        draw_text(window, winner_msg, (float)MARGIN, 10.f, 20, COLOR_WIN_TEXT);
        draw_text(window, mode_label, (float)MARGIN, 34.f, 14, COLOR_TEXT);
    }

    draw_text(window, "White Captures: " + std::to_string(board.captures[1]) + "/10",
              (float)MARGIN, 54.f, 18, COLOR_TEXT);
    draw_text(window, "Black Captures: " + std::to_string(board.captures[2]) + "/10",
              (float)(WINDOW_WIDTH - MARGIN - 190), 54.f, 18, COLOR_TEXT);

    // Timer IA (obligatoire selon le sujet)
    if (show_ai_timer && settings.play_against == "Computer") {
        char buf[128];
        snprintf(buf, sizeof(buf), "AI time: %.1f ms | depth: %d", ai_time_ms, ai_depth);
        draw_text(window, buf, (float)(WINDOW_WIDTH - MARGIN - 260), 78.f, 14, COLOR_TEXT);
    }

    sf::RectangleShape footer(sf::Vector2f((float)WINDOW_WIDTH, 34.f));
    footer.setPosition({0.f, (float)WINDOW_HEIGHT - 34.f});
    footer.setFillColor(sf::Color(255, 255, 255, 90));
    window.draw(footer);

    draw_text(window, "T: Suggest move    X: Undo    E: Exit", (float)MARGIN, (float)WINDOW_HEIGHT - 27.f,
              14, COLOR_TEXT, true);
}

sf::FloatRect GameUI::draw_game_over_popup(sf::RenderWindow& window, const std::string& winner_msg,
                                           sf::Vector2f mouse_pos) {
    sf::RectangleShape overlay(sf::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
    overlay.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(overlay);

    sf::FloatRect popup_rect({(float)WINDOW_WIDTH / 2.f - 210.f, (float)WINDOW_HEIGHT / 2.f - 95.f}, {420.f, 190.f});
    sf::RectangleShape popup(sf::Vector2f(popup_rect.size.x, popup_rect.size.y));
    popup.setPosition(popup_rect.position);
    popup.setFillColor(COLOR_CARD);
    popup.setOutlineColor(COLOR_CARD_EDGE);
    popup.setOutlineThickness(4.f);
    window.draw(popup);

    draw_text(window, winner_msg, popup_rect.position.x + 28.f, popup_rect.position.y + 22.f,
              22, COLOR_TEXT);
    draw_text(window, "The game is over.", popup_rect.position.x + 28.f, popup_rect.position.y + 58.f,
              16, COLOR_TEXT, false);
    draw_text(window, "Click Exit to close the program.", popup_rect.position.x + 28.f,
              popup_rect.position.y + 82.f, 16, COLOR_TEXT, false);

    sf::FloatRect exit_rect({popup_rect.position.x + 95.f, popup_rect.position.y + 124.f}, {230.f, 44.f});
    hitbox_exit = exit_rect;
    draw_button(window, exit_rect, "Exit", true, true, exit_rect.contains(mouse_pos));

    return exit_rect;
}

void GameUI::draw_suggestion_marker(sf::RenderWindow& window, int row, int col) {
    // Anneau colore (pas un disque plein, pour ne pas etre confondu avec
    // une vraie pierre) indiquant la case suggeree par l'IA.
    float cx = (float)(MARGIN + col * CELL_SIZE);
    float cy = (float)(TOP_PANEL + MARGIN + row * CELL_SIZE);

    sf::CircleShape ring(14.f);
    ring.setOrigin({14.f, 14.f});
    ring.setPosition({cx, cy});
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(220, 50, 50));  // rouge vif, bien visible
    ring.setOutlineThickness(3.f);
    window.draw(ring);
}
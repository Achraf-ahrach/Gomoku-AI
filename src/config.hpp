// config.hpp
//
// Constantes partagées pour l'interface graphique, équivalent de config.py.

#pragma once
#include "board.hpp"
#include <SFML/Graphics.hpp>

constexpr int CELL_SIZE = 40;
constexpr int MARGIN = 40;
constexpr int TOP_PANEL = 80;

constexpr int WINDOW_WIDTH = (BOARD_SIZE - 1) * CELL_SIZE + (MARGIN * 2);
constexpr int WINDOW_HEIGHT = (BOARD_SIZE - 1) * CELL_SIZE + (MARGIN * 2) + TOP_PANEL;

// "const" à portée namespace => linkage interne, sûr contre les violations
// ODR même si ce header est inclus dans plusieurs fichiers .cpp.
const sf::Color COLOR_WOOD(220, 179, 119);
const sf::Color COLOR_LINE(40, 40, 40);
const sf::Color COLOR_WHITE_STONE(245, 245, 240);
const sf::Color COLOR_BLACK_STONE(36, 36, 36);
const sf::Color COLOR_WHITE(245, 245, 245);
const sf::Color COLOR_TEXT(44, 62, 80);
const sf::Color COLOR_CARD(249, 242, 231);
const sf::Color COLOR_CARD_EDGE(146, 103, 58);
const sf::Color COLOR_BUTTON(236, 223, 200);
const sf::Color COLOR_BUTTON_SELECTED(255, 248, 236);
const sf::Color COLOR_BUTTON_TEXT(75, 49, 29);
const sf::Color COLOR_DISABLED_TEXT(119, 95, 65);
const sf::Color COLOR_CONFIRM(67, 121, 77);
const sf::Color COLOR_CANCEL(146, 78, 68);
const sf::Color COLOR_WIN_TEXT(39, 174, 96);
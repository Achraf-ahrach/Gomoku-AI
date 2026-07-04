"""Pygame UI components for Gomoku."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Sequence, Tuple

import pygame

from config import (
    BOARD_SIZE,
    CELL_SIZE,
    COLOR_BLACK_STONE,
    COLOR_BUTTON,
    COLOR_BUTTON_SELECTED,
    COLOR_BUTTON_TEXT,
    COLOR_CANCEL,
    COLOR_CARD,
    COLOR_CARD_EDGE,
    COLOR_CONFIRM,
    COLOR_DISABLED_TEXT,
    COLOR_LINE,
    COLOR_TEXT,
    COLOR_WHITE,
    COLOR_WHITE_STONE,
    COLOR_WOOD,
    MARGIN,
    TOP_PANEL,
    WINDOW_HEIGHT,
    WINDOW_WIDTH,
)


@dataclass
class SettingsState:
    play_against: str = "Computer"
    play_as: str = "White"
    difficulty: str = "Medium"


@dataclass
class MenuAction:
    play_against: Optional[str] = None
    play_as: Optional[str] = None
    difficulty: Optional[str] = None
    confirm: bool = False
    cancel: bool = False


@dataclass
class MenuLayout:
    card_rect: pygame.Rect = field(default_factory=lambda: pygame.Rect(90, 55, WINDOW_WIDTH - 180, 420))
    row_height: int = 68
    row_gap: int = 10

    def __post_init__(self) -> None:
        row_top = self.card_rect.y + 78
        row_left = self.card_rect.x + 28
        row_width = self.card_rect.width - 56
        self.menu_row_rects = {
            "play_against": pygame.Rect(row_left, row_top, row_width, self.row_height),
            "play_as": pygame.Rect(row_left, row_top + (self.row_height + self.row_gap), row_width, self.row_height),
            "difficulty": pygame.Rect(row_left, row_top + 2 * (self.row_height + self.row_gap), row_width, self.row_height),
        }
        self.button_row_rect = pygame.Rect(row_left, row_top + 3 * (self.row_height + self.row_gap) + 10, row_width, 58)


class GameUI:
    def __init__(self) -> None:
        self.title_font = pygame.font.SysFont("Arial", 28, bold=True)
        self.label_font = pygame.font.SysFont("Arial", 21, bold=True)
        self.option_font = pygame.font.SysFont("Arial", 18, bold=True)
        self.hud_font = pygame.font.SysFont("Arial", 20, bold=True)
        self.small_font = pygame.font.SysFont("Arial", 16, bold=True)
        self.layout = MenuLayout()
        self.menu_hitboxes: Dict[str, List[Tuple[str, pygame.Rect]]] = {}

    @staticmethod
    def rounded_rect(surface: pygame.Surface, color, rect: pygame.Rect, radius: int = 18, border_color=None, border_width: int = 0) -> None:
        pygame.draw.rect(surface, color, rect, border_radius=radius)
        if border_color and border_width > 0:
            pygame.draw.rect(surface, border_color, rect, width=border_width, border_radius=radius)

    @staticmethod
    def player_color_name(player_id: int) -> str:
        return "White" if player_id == 1 else "Black"

    @staticmethod
    def player_stone_color(player_id: int):
        return COLOR_WHITE_STONE if player_id == 1 else COLOR_BLACK_STONE

    def _draw_button(self, surface: pygame.Surface, rect: pygame.Rect, text: str, selected: bool = False, enabled: bool = True, hovered: bool = False) -> None:
        button_color = COLOR_BUTTON_SELECTED if selected else COLOR_BUTTON
        if hovered:
            button_color = tuple(min(255, channel + 8) for channel in button_color)
        if not enabled:
            button_color = tuple(min(255, channel + 8) for channel in button_color)

        self.rounded_rect(surface, button_color, rect, radius=16, border_color=(182, 155, 120), border_width=2)
        label = self.option_font.render(text, True, COLOR_BUTTON_TEXT if enabled else COLOR_DISABLED_TEXT)
        surface.blit(label, label.get_rect(center=rect.center))

    def _draw_segmented_row(
        self,
        surface: pygame.Surface,
        row_label: str,
        options: Sequence[str],
        selected_value: str,
        row_rect: pygame.Rect,
        enabled: bool = True,
        faded_alpha: int = 255,
        mouse_pos: Optional[Tuple[int, int]] = None,
    ) -> List[pygame.Rect]:
        row_surface = pygame.Surface(row_rect.size, pygame.SRCALPHA)
        label_width = 180
        label_text = self.label_font.render(row_label, True, COLOR_TEXT if enabled else COLOR_DISABLED_TEXT)
        row_surface.blit(label_text, (0, row_rect.height // 2 - label_text.get_height() // 2))

        button_area_x = label_width + 12
        button_area_width = row_rect.width - button_area_x
        gap = 10
        button_count = len(options)
        button_width = (button_area_width - gap * (button_count - 1)) // button_count
        button_height = 46
        button_y = row_rect.height // 2 - button_height // 2

        button_rects: List[pygame.Rect] = []
        for index, option in enumerate(options):
            button_rect = pygame.Rect(button_area_x + index * (button_width + gap), button_y, button_width, button_height)
            is_selected = option == selected_value
            is_hovered = enabled and mouse_pos is not None and button_rect.move(row_rect.topleft).collidepoint(mouse_pos)
            button_color = COLOR_BUTTON_SELECTED if is_selected else COLOR_BUTTON
            if is_hovered:
                button_color = tuple(min(255, channel + 8) for channel in button_color)
            if not enabled:
                button_color = tuple(min(255, channel + 8) for channel in button_color)
            self.rounded_rect(row_surface, button_color, button_rect, radius=15, border_color=(176, 146, 109), border_width=2)
            option_label = self.option_font.render(option, True, COLOR_BUTTON_TEXT if enabled else COLOR_DISABLED_TEXT)
            row_surface.blit(option_label, option_label.get_rect(center=button_rect.center))
            button_rects.append(button_rect.move(row_rect.topleft))

        row_surface.set_alpha(faded_alpha)
        surface.blit(row_surface, row_rect.topleft)
        return button_rects

    def render_board_background(self, screen: pygame.Surface, draw_stones: bool, game=None) -> None:
        screen.fill(COLOR_WHITE)
        board_rect = pygame.Rect(0, TOP_PANEL, WINDOW_WIDTH, WINDOW_HEIGHT - TOP_PANEL)
        pygame.draw.rect(screen, COLOR_WOOD, board_rect)

        for i in range(BOARD_SIZE):
            pygame.draw.line(screen, COLOR_LINE, (MARGIN, TOP_PANEL + MARGIN + i * CELL_SIZE), (WINDOW_WIDTH - MARGIN, TOP_PANEL + MARGIN + i * CELL_SIZE), 1)
            pygame.draw.line(screen, COLOR_LINE, (MARGIN + i * CELL_SIZE, TOP_PANEL + MARGIN), (MARGIN + i * CELL_SIZE, WINDOW_HEIGHT - MARGIN), 1)

        star_points = [3, 9, 15]
        for r in star_points:
            for c in star_points:
                pygame.draw.circle(screen, COLOR_LINE, (MARGIN + c * CELL_SIZE, TOP_PANEL + MARGIN + r * CELL_SIZE), 5)

        if not draw_stones or game is None:
            return

        for r in range(BOARD_SIZE):
            for c in range(BOARD_SIZE):
                cell_value = game.grid[r][c]
                if cell_value == 0:
                    continue
                center_pos = (MARGIN + c * CELL_SIZE, TOP_PANEL + MARGIN + r * CELL_SIZE)
                stone_color = self.player_stone_color(cell_value)
                outline_color = (35, 35, 35) if cell_value == 1 else (240, 240, 240)
                pygame.draw.circle(screen, stone_color, center_pos, 16)
                pygame.draw.circle(screen, outline_color, center_pos, 16, 1)

    def draw_settings_menu(self, screen: pygame.Surface, settings_state: SettingsState, mouse_pos: Optional[Tuple[int, int]] = None) -> Dict[str, List[Tuple[str, pygame.Rect]]]:
        self.render_board_background(screen, draw_stones=False)
        overlay = pygame.Surface((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.SRCALPHA)
        overlay.fill((255, 255, 255, 18))
        screen.blit(overlay, (0, 0))

        rounded_rect = self.rounded_rect
        rounded_rect(screen, COLOR_CARD, self.layout.card_rect, radius=28, border_color=COLOR_CARD_EDGE, border_width=4)

        title = self.title_font.render("Game Settings", True, COLOR_TEXT)
        subtitle = self.small_font.render("Choose how you want to play before starting.", True, (92, 68, 42))
        screen.blit(title, title.get_rect(center=(self.layout.card_rect.centerx, self.layout.card_rect.y + 40)))
        screen.blit(subtitle, subtitle.get_rect(center=(self.layout.card_rect.centerx, self.layout.card_rect.y + 74)))

        self.menu_hitboxes.clear()

        play_against_labels = ["🤖 Computer", "🤝 Human"]
        play_against_values = ["Computer", "Human"]
        play_against_selected = "🤖 Computer" if settings_state.play_against == "Computer" else "🤝 Human"
        self.menu_hitboxes["play_against"] = [(value, rect) for value, rect in zip(
            play_against_values,
            self._draw_segmented_row(screen, "Play Against", play_against_labels, play_against_selected, self.layout.menu_row_rects["play_against"], enabled=True, faded_alpha=255, mouse_pos=mouse_pos),
        )]

        play_as_enabled = settings_state.play_against == "Computer"
        play_as_labels = ["⚪ White", "⚫ Black"]
        play_as_values = ["White", "Black"]
        play_as_selected = "⚪ White" if settings_state.play_as == "White" else "⚫ Black"
        self.menu_hitboxes["play_as"] = [(value, rect) for value, rect in zip(
            play_as_values,
            self._draw_segmented_row(screen, "Play As", play_as_labels, play_as_selected, self.layout.menu_row_rects["play_as"], enabled=play_as_enabled, faded_alpha=255 if play_as_enabled else 120, mouse_pos=mouse_pos),
        )]

        difficulty_labels = ["😳 Easy", "😨 Medium", "😰 Hard"]
        difficulty_values = ["Easy", "Medium", "Hard"]
        difficulty_selected = "😳 Easy" if settings_state.difficulty == "Easy" else ("😨 Medium" if settings_state.difficulty == "Medium" else "😰 Hard")
        self.menu_hitboxes["difficulty"] = [(value, rect) for value, rect in zip(
            difficulty_values,
            self._draw_segmented_row(screen, "Difficulty", difficulty_labels, difficulty_selected, self.layout.menu_row_rects["difficulty"], enabled=play_as_enabled, faded_alpha=255 if play_as_enabled else 120, mouse_pos=mouse_pos),
        )]

        confirm_rect = pygame.Rect(self.layout.button_row_rect.x, self.layout.button_row_rect.y, self.layout.button_row_rect.width // 2 - 10, self.layout.button_row_rect.height)
        cancel_rect = pygame.Rect(self.layout.button_row_rect.x + self.layout.button_row_rect.width // 2 + 10, self.layout.button_row_rect.y, self.layout.button_row_rect.width // 2 - 10, self.layout.button_row_rect.height)
        self.menu_hitboxes["confirm"] = confirm_rect
        self.menu_hitboxes["cancel"] = cancel_rect

        self._draw_button(screen, confirm_rect, "Confirm", selected=True, enabled=True, hovered=mouse_pos is not None and confirm_rect.collidepoint(mouse_pos))
        self.rounded_rect(screen, COLOR_CANCEL, cancel_rect, radius=18, border_color=(112, 56, 50), border_width=2)
        cancel_label = self.label_font.render("Cancel", True, COLOR_WHITE)
        screen.blit(cancel_label, cancel_label.get_rect(center=cancel_rect.center))

        return self.menu_hitboxes

    def draw_hud(self, screen: pygame.Surface, game, game_over: bool, winner_msg: str, play_against: str, play_as: str, difficulty: str, human_player: int) -> None:
        if play_against == "Computer":
            mode_label = f"Mode: Computer | You: {play_as} | Difficulty: {difficulty}"
        else:
            mode_label = "Mode: Human vs Human"

        if not game_over:
            turn_side = "White" if game.current_player == 1 else "Black"
            if play_against == "Computer":
                turn_owner = "You" if game.current_player == human_player else "Computer"
                turn_text = self.hud_font.render(f"Turn: {turn_side} ({turn_owner})", True, COLOR_TEXT)
            else:
                turn_text = self.hud_font.render(f"Turn: {turn_side}", True, COLOR_TEXT)
            mode_text = self.small_font.render(mode_label, True, COLOR_TEXT)
            screen.blit(turn_text, (MARGIN, 10))
            screen.blit(mode_text, (MARGIN, 34))
        else:
            winner_text = self.hud_font.render(winner_msg, True, (39, 174, 96))
            mode_text = self.small_font.render(mode_label, True, COLOR_TEXT)
            screen.blit(winner_text, (MARGIN, 10))
            screen.blit(mode_text, (MARGIN, 34))

        white_caps = self.hud_font.render(f"White Captures: {game.captures[1]}/10", True, COLOR_TEXT)
        black_caps = self.hud_font.render(f"Black Captures: {game.captures[2]}/10", True, COLOR_TEXT)
        screen.blit(white_caps, (MARGIN, 54))
        screen.blit(black_caps, (WINDOW_WIDTH - MARGIN - 190, 54))

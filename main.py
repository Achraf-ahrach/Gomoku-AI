# main.py
import os
import sys
from pathlib import Path


def _bootstrap_local_venv():
    project_root = Path(__file__).resolve().parent
    venv_python = project_root / "venv" / "bin" / "python"

    if Path(sys.argv[0]).resolve() != Path(__file__).resolve():
        return

    if venv_python.exists() and os.environ.get("GOMOKU_AI_BOOTSTRAPPED") != "1":
        os.environ["GOMOKU_AI_BOOTSTRAPPED"] = "1"
        os.execv(str(venv_python), [str(venv_python), str(Path(__file__).resolve()), *sys.argv[1:]])


try:
    import pygame
except ModuleNotFoundError:
    _bootstrap_local_venv()
    import pygame

from ai_engine import get_computer_move
from board import GomokuBoard
from config import BOARD_SIZE, CELL_SIZE, MARGIN, TOP_PANEL, WINDOW_HEIGHT, WINDOW_WIDTH
from ui_components import GameUI, SettingsState

def main():
    pygame.init()
    pygame.font.init()

    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Gomoku AI - Settings Menu")

    clock = pygame.time.Clock()
    ui = GameUI()
    settings = SettingsState()

    game_state = "MENU"

    game = None
    game_over = False
    winner_msg = ""
    human_player = 1
    computer_player = 2

    def reset_game_from_menu():
        nonlocal game, game_over, winner_msg, human_player, computer_player
        game = GomokuBoard()
        game_over = False
        winner_msg = ""
        if settings.play_against == "Computer":
            human_player = 1 if settings.play_as == "White" else 2
            computer_player = 2 if human_player == 1 else 1
        else:
            human_player = 1
            computer_player = 2

    def resolve_move(row, col):
        nonlocal game_over, winner_msg

        if not game.place_stone(row, col):
            return False

        if game.check_win(row, col):
            game_over = True
            winner_color = ui.player_color_name(game.current_player)
            if game.captures[game.current_player] >= 10:
                winner_msg = f"🏆 {winner_color} wins by capture (10 stones)!"
            else:
                winner_msg = f"🏆 {winner_color} wins by alignment!"
        else:
            game.switch_player()

        return True

    def run_computer_turn():
        if game_over or settings.play_against != "Computer" or game is None:
            return

        if game.current_player != computer_player:
            return

        print(f"🤖 Computer thinking using {settings.difficulty} mode...")
        computer_move = get_computer_move(game, settings.difficulty)
        if computer_move is None:
            print("⚠️ Computer AI did not return a move.")
            return

        row, col = computer_move
        if not (0 <= row < BOARD_SIZE and 0 <= col < BOARD_SIZE):
            print(f"⚠️ Computer AI returned an out-of-bounds move: {(row, col)}")
            return

        resolve_move(row, col)

    def handle_menu_click(mx, my):
        nonlocal game_state

        play_against_hitboxes = ui.menu_hitboxes.get("play_against", [])
        for option, rect in play_against_hitboxes:
            if rect.collidepoint(mx, my):
                settings.play_against = option
                if settings.play_against == "Human":
                    settings.play_as = "White"
                    settings.difficulty = "Medium"
                return

        if settings.play_against == "Computer":
            for option, rect in ui.menu_hitboxes.get("play_as", []):
                if rect.collidepoint(mx, my):
                    settings.play_as = option
                    return

            for option, rect in ui.menu_hitboxes.get("difficulty", []):
                if rect.collidepoint(mx, my):
                    settings.difficulty = option
                    return

        confirm_rect = ui.menu_hitboxes.get("confirm")
        cancel_rect = ui.menu_hitboxes.get("cancel")
        if confirm_rect and confirm_rect.collidepoint(mx, my):
            reset_game_from_menu()
            game_state = "PLAYING"
        elif cancel_rect and cancel_rect.collidepoint(mx, my):
            pygame.quit()
            sys.exit()
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()

            if event.type == pygame.MOUSEBUTTONDOWN:
                mx, my = pygame.mouse.get_pos()
                if game_state == "MENU":
                    handle_menu_click(mx, my)
                elif game_state == "PLAYING" and not game_over:
                    if settings.play_against == "Computer" and game.current_player == computer_player:
                        continue

                    adjusted_y = my - TOP_PANEL

                    c = round((mx - MARGIN) / CELL_SIZE)
                    r = round((adjusted_y - MARGIN) / CELL_SIZE)

                    if 0 <= r < BOARD_SIZE and 0 <= c < BOARD_SIZE:
                        if settings.play_against == "Human" or game.current_player == human_player:
                            resolve_move(r, c)

        if game_state == "PLAYING" and not game_over and settings.play_against == "Computer" and game.current_player == computer_player:
            run_computer_turn()

        if game_state == "MENU":
            ui.draw_settings_menu(screen, settings, mouse_pos=pygame.mouse.get_pos())
        else:
            ui.render_board_background(screen, draw_stones=True, game=game)
            ui.draw_hud(screen, game, game_over, winner_msg, settings.play_against, settings.play_as, settings.difficulty, human_player)

        pygame.display.flip()
        clock.tick(60)

if __name__ == "__main__":
    main()
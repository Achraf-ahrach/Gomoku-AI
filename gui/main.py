# main.py
import sys
import pygame
from board import GomokuBoard

BOARD_SIZE = 19
CELL_SIZE = 40
MARGIN = 40
TOP_PANEL = 80

WINDOW_WIDTH = (BOARD_SIZE - 1) * CELL_SIZE + (MARGIN * 2)
WINDOW_HEIGHT = (BOARD_SIZE - 1) * CELL_SIZE + (MARGIN * 2) + TOP_PANEL

COLOR_WOOD = (220, 179, 119)
COLOR_LINE = (40, 40, 40)
COLOR_PLAYER1 = (41, 128, 185)    # Blue
COLOR_PLAYER2 = (192, 57, 43)     # Red
COLOR_WHITE = (245, 245, 245)
COLOR_TEXT = (44, 62, 80)

def main():
    pygame.init()
    pygame.font.init()
    
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    pygame.display.set_caption("Gomoku Engine - Captures Enabled")
    
    font = pygame.font.SysFont("Arial", 20, bold=True)
    game = GomokuBoard()
    game_over = False
    winner_msg = ""

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
                
            if event.type == pygame.MOUSEBUTTONDOWN and not game_over:
                mx, my = pygame.mouse.get_pos()
                adjusted_y = my - TOP_PANEL
                
                c = round((mx - MARGIN) / CELL_SIZE)
                r = round((adjusted_y - MARGIN) / CELL_SIZE)
                
                if 0 <= r < BOARD_SIZE and 0 <= c < BOARD_SIZE:
                    if game.place_stone(r, c):
                        if game.check_win(r, c):
                            game_over = True
                            if game.captures[game.current_player] >= 10:
                                winner_msg = f"🏆 Player {game.current_player} Wins by Capture (10 Stones)!"
                            else:
                                winner_msg = f"🏆 Player {game.current_player} Wins by Alignment!"
                        else:
                            game.switch_player()

        # Render Graphics Layout
        screen.fill(COLOR_WHITE)
        board_rect = pygame.Rect(0, TOP_PANEL, WINDOW_WIDTH, WINDOW_HEIGHT - TOP_PANEL)
        pygame.draw.rect(screen, COLOR_WOOD, board_rect)
        
        # Grid Drawing
        for i in range(BOARD_SIZE):
            pygame.draw.line(screen, COLOR_LINE, (MARGIN, TOP_PANEL + MARGIN + i * CELL_SIZE), (WINDOW_WIDTH - MARGIN, TOP_PANEL + MARGIN + i * CELL_SIZE), 1)
            pygame.draw.line(screen, COLOR_LINE, (MARGIN + i * CELL_SIZE, TOP_PANEL + MARGIN), (MARGIN + i * CELL_SIZE, WINDOW_HEIGHT - MARGIN), 1)

        # Star Points
        star_points = [3, 9, 15]
        for r in star_points:
            for c in star_points:
                pygame.draw.circle(screen, COLOR_LINE, (MARGIN + c * CELL_SIZE, TOP_PANEL + MARGIN + r * CELL_SIZE), 5)

        # Draw Active Stones Matrix
        for r in range(BOARD_SIZE):
            for c in range(BOARD_SIZE):
                cell_value = game.grid[r][c]
                if cell_value != 0:
                    center_pos = (MARGIN + c * CELL_SIZE, TOP_PANEL + MARGIN + r * CELL_SIZE)
                    stone_color = COLOR_PLAYER1 if cell_value == 1 else COLOR_PLAYER2
                    pygame.draw.circle(screen, stone_color, center_pos, 16)
                    pygame.draw.circle(screen, (30, 30, 30), center_pos, 16, 1)

        # --- Top Info Panel Status Graphics ---
        if not game_over:
            current_char = "Blue" if game.current_player == 1 else "Red"
            turn_text = font.render(f"Turn: Player {game.current_player} ({current_char})", True, COLOR_TEXT)
            screen.blit(turn_text, (MARGIN, 15))
        else:
            winner_text = font.render(winner_msg, True, (39, 174, 96))
            screen.blit(winner_text, (MARGIN, 15))

        # Render Capture Score Trackers Live
        p1_caps = font.render(f"Blue Captures: {game.captures[1]}/10", True, COLOR_PLAYER1)
        p2_caps = font.render(f"Red Captures: {game.captures[2]}/10", True, COLOR_PLAYER2)
        screen.blit(p1_caps, (MARGIN, 45))
        screen.blit(p2_caps, (WINDOW_WIDTH - MARGIN - 180, 45))

        pygame.display.flip()

if __name__ == "__main__":
    main()
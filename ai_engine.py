"""AI engine placeholder for computer move selection.

Your classmate should replace the stub logic in this module with the real
search/evaluation implementation.
"""

from __future__ import annotations

import random


def get_computer_move(board_instance, difficulty_level):
    """Return a temporary legal move for the current board state.

    Args:
        board_instance: The active `GomokuBoard` instance.
        difficulty_level: Menu difficulty string such as "Easy", "Medium", or "Hard".

    Returns:
        A `(row, col)` tuple for a legal move, or `None` if the board is full.
    """
    # TODO: My classmate will inject their AI search logic here.
    print(f"🤖 AI stub active at {difficulty_level} difficulty.")

    valid_moves = [
        (row, col)
        for row in range(board_instance.size)
        for col in range(board_instance.size)
        if board_instance.grid[row][col] == 0
    ]

    if not valid_moves:
        print("⚠️ No legal moves remain for the computer.")
        return None

    move = random.choice(valid_moves)
    print(f"🤖 Temporary stub selected move: {move}")
    return move

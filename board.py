
class GomokuBoard:
    def __init__(self):
        # 0 = Empty, 1 = Player 1 (X), 2 = Player 2 (O)
        self.size = 19
        self.grid = [[0 for _ in range(self.size)] for _ in range(self.size)]
        self.current_player = 1

    def print_board(self):
        """Prints the current state of the board to the console."""
        print("   " + " ".join([f"{i:2d}" for i in range(self.size)]))
        for r_idx, row in enumerate(self.grid):
            row_str = []
            for cell in row:
                if cell == 0: row_str.append(" .")
                elif cell == 1: row_str.append(" X")
                elif cell == 2: row_str.append(" O")
            print(f"{r_idx:2d} {' '.join(row_str)}")

    def place_stone(self, row, col):
        """Places a stone on the board if the move is valid."""
        if row < 0 or row >= self.size or col < 0 or col >= self.size:
            print("❌ Move is out of bounds!")
            return False
        
        if self.grid[row][col] != 0:
            print("❌ This cell is already occupied!")
            return False
        
        self.grid[row][col] = self.current_player
        return True

    def switch_player(self):
        """Switches the turn to the other player."""
        self.current_player = 2 if self.current_player == 1 else 1

    def check_win(self, r, c):
        """Checks if the last placed stone at (r, c) creates a line of 5 or more."""
        player = self.grid[r][c]
        if player == 0:
            return False

        # The 4 directions to check: (delta_row, delta_col)
        # 1. Horizontal, 2. Vertical, 3. Diagonal Down-Right, 4. Diagonal Up-Right
        directions = [(0, 1), (1, 0), (1, 1), (1, -1)]

        for dr, dc in directions:
            count = 1  # Include the stone just placed
            
            # Check forward direction
            step = 1
            while True:
                nr, nc = r + dr * step, c + dc * step
                if 0 <= nr < self.size and 0 <= nc < self.size and self.grid[nr][nc] == player:
                    count += 1
                    step += 1
                else:
                    break
            
            # Check backward direction
            step = 1
            while True:
                nr, nc = r - dr * step, c - dc * step
                if 0 <= nr < self.size and 0 <= nc < self.size and self.grid[nr][nc] == player:
                    count += 1
                    step += 1
                else:
                    break
            
            # According to the rules, 5 or more in a row wins
            if count >= 5:
                return True
                
        return False
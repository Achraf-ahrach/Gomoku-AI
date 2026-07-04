# board.py

class GomokuBoard:
    def __init__(self):
        # 0 = Empty, 1 = Player 1 (Blue), 2 = Player 2 (Red)
        self.size = 19
        self.grid = [[0 for _ in range(self.size)] for _ in range(self.size)]
        self.current_player = 1
        
        # Track individual stone captures (10 stones caught = win)
        self.captures = {1: 0, 2: 0}

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
        """Places a stone and resolves any active capture sequences."""
        if row < 0 or row >= self.size or col < 0 or col >= self.size:
            print("❌ Move is out of bounds!")
            return False
        
        if self.grid[row][col] != 0:
            print("❌ This cell is already occupied!")
            return False
        
        # Place the stone
        self.grid[row][col] = self.current_player
        
        # Check and apply captures immediately after placement
        self.check_and_apply_captures(row, col)
        return True

    def switch_player(self):
        """Switches the turn to the other player."""
        self.current_player = 2 if self.current_player == 1 else 1

    def check_and_apply_captures(self, r, c):
        """
        Scans all 8 directions radiating from the placed stone (r, c).
        If a pattern of [Current, Opponent, Opponent, Current] is detected,
        the middle two stones are removed and added to the current player's score.
        """
        player = self.grid[r][c]
        opponent = 2 if player == 1 else 1
        
        # Look in all 8 directions radiating from the placed stone
        directions = [
            (0, 1),   # Right
            (0, -1),  # Left
            (1, 0),   # Down
            (-1, 0),  # Up
            (1, 1),   # Down-Right
            (-1, -1), # Up-Left
            (1, -1),  # Down-Left
            (-1, 1)   # Up-Right
        ]

        for dr, dc in directions:
            # Look up to 3 spaces forward on the directional vector
            r1, c1 = r + dr, c + dc
            r2, c2 = r + dr * 2, c + dc * 2
            r3, c3 = r + dr * 3, c + dc * 3

            # Check inside bounds
            if (0 <= r3 < self.size) and (0 <= c3 < self.size):
                # Look for the exact flanking pattern: [Player, Opponent, Opponent, Player]
                if (self.grid[r1][c1] == opponent and 
                    self.grid[r2][c2] == opponent and 
                    self.grid[r3][c3] == player):
                    
                    # Capture confirmed: Clear the points off the board grid matrix
                    self.grid[r1][c1] = 0
                    self.grid[r2][c2] = 0
                    
                    # Log 2 stone captures to the active scoring profile
                    self.captures[player] += 2
                    print(f"🎯 Player {player} captured a pair! Total captures: {self.captures[player]}/10")

    def check_win(self, r, c):
        """Checks both alignment conditions (5+ stones) and capture conditions (10 stones)."""
        player = self.grid[r][c]
        if player == 0:
            return False

        # Condition 1: Check if current player reached 10 total individual captured stones
        if self.captures[player] >= 10:
            return True

        # Condition 2: Traditional alignment match check (5+ continuous stones)
        directions = [(0, 1), (1, 0), (1, 1), (1, -1)]
        for dr, dc in directions:
            count = 1
            
            # Forward scan
            step = 1
            while True:
                nr, nc = r + dr * step, c + dc * step
                if 0 <= nr < self.size and 0 <= nc < self.size and self.grid[nr][nc] == player:
                    count += 1
                    step += 1
                else:
                    break
            
            # Reverse scan
            step = 1
            while True:
                nr, nc = r - dr * step, c - dc * step
                if 0 <= nr < self.size and 0 <= nc < self.size and self.grid[nr][nc] == player:
                    count += 1
                    step += 1
                else:
                    break
            
            if count >= 5:
                return True
                
        return False
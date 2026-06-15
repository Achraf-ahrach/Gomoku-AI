
from board import GomokuBoard

def main():
    game = GomokuBoard()
    print("🎯 Welcome to Gomoku Backend Verification!")
    
    while True:
        game.print_board()
        player_char = 'X' if game.current_player == 1 else 'O'
        print(f"\n👉 Player {game.current_player}'s turn ({player_char})")
        
        try:
            move = input("Enter coordinates (row col) or 'q' to quit: ").strip()
            if move.lower() == 'q':
                print("Exiting game. Goodbye!")
                break
                
            r, c = map(int, move.split())
            
            if game.place_stone(r, c):
                if game.check_win(r, c):
                    game.print_board()
                    print(f"🎉🏆 Congratulations! Player {game.current_player} wins!")
                    break
                game.switch_player()
                
        except ValueError:
            print("❌ Invalid input! Please enter two integers separated by a space (e.g., 10 5).")
        print("-" * 40)

if __name__ == "__main__":
    main()
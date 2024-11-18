# chessengine

## Overview
This project is a chess engine implemented in C. It includes functionality for parsing chess positions in FEN notation, generating legal moves, evaluating board states, and searching for optimal moves using the negamax algorithm. The engine supports various debug positions, including the starting position and several complex scenarios.

## Features
- **FEN Parsing**: Parse Forsyth-Edwards Notation strings to set up board positions.
- **Move Generation**: Generate legal moves for all pieces, including special moves like castling and en passant.
- **Board Evaluation**: Evaluate board states to determine the best possible moves.
- **Search Algorithms**: Implement search algorithms like negamax to explore move trees and optimize decisions.
- **Magic Bitboards**: Use magic bitboards for efficient move generation of sliding pieces (bishops, rooks, queens).

## Functions in main.c
- **get_randU32, get_randU64**: Generate random 32-bit and 64-bit numbers.
- **generate_magic_num**: Generate magic numbers for bitboards.
- **count_bits, get_lsb_index**: Utility functions for bitboard manipulation.
- **print_bitboard, print_board**: Print bitboards and board states for debugging.
- **parse_fen**: Parse FEN strings to set up the board.
- **mask_pawn_attacks, mask_knight_attacks, mask_king_attacks, mask_bishop_attacks, mask_rook_attacks**: Generate attack masks for each piece type.
- **bishop_attacks_otf, rook_attacks_otf**: Generate on-the-fly attacks for sliding pieces.
- **init_leaper_attacks, init_slider_attacks**: Initialize attack tables for pieces.
- **set_occupancy, find_magic_num, init_magic_num**: Functions related to magic bitboards.
- **get_bishop_attacks, get_rook_attacks, get_queen_attacks**: Get attacks for sliding pieces using magic bitboards.
- **is_attacked, print_attacked**: Check if a square is attacked by a side.
- **moves, add_move, print_move, print_move_list**: Structures and functions for handling move lists.
- **make_move, generate_moves**: Make moves and generate all possible legal moves.
- **get_time_ms**: Get the current time in milliseconds.
- **perft_driver, perft_test**: Performance test functions for move generation.
- **evaluate**: Evaluate the current board position.
- **negamax**: Implement the negamax search algorithm.
- **search_position**: Search for the best move from the current position.
- **parse_move, parse_position, parse_go**: Parse UCI commands.
- **uci_loop**: Main loop to handle UCI commands.
- **init_all**: Initialize all necessary data structures and tables.
- **main**: Entry point of the program.

## How to Run
1. Clone the repository:
   ```
   git clone https://github.com/StrydrS/chessengine.git
   ```
2. Navigate to the project directory:
   ```
   cd chessengine
   ```
3. Compile the program:
   ```
   gcc -o chessengine main.c
   ```
4. Run the engine:
   ```
   ./chessengine
   ```

This README provides an overview of the chess engine project, detailing its features, main functions, and instructions on how to run the engine. For further details, you can refer to the source code in the `main.c` file.

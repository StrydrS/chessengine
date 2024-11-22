
# Kirin Chess Engine

Kirin Chess Engine is a lightweight chess program implemented in C. It uses the Universal Chess Interface (UCI) protocol to interact with chess GUIs like Arena, Stockfish-compatible engines, or online platforms such as Lichess. Currently, the engine has an approximate rating of **1800** on Lichess.

## Features

- Implements the UCI protocol for compatibility with GUI chess interfaces.
- Supports chessboard representation via bitboards for efficient computation.
- Allows position loading and initialization using FEN (Forsyth-Edwards Notation) strings.
- Capable of move generation, position evaluation, and basic gameplay logic.
- Designed for educational purposes with clean, modular code.

## Getting Started

### Prerequisites

To compile and run the engine, you need:
- A C compiler (e.g., GCC or Clang)
- A chess GUI that supports the UCI protocol (e.g., Arena, ChessBase, or Lichess bots)

### Compilation

Use the following command to compile the program:

```bash
cd kirin-ce
make
```

### Running the Engine

Once compiled, you can run the engine from the command line:

```bash
./kirin
```

The engine will initialize and await commands from a UCI-compatible GUI.

### Using with a Chess GUI

1. Open your preferred UCI-compatible chess GUI.
2. Add the compiled `kirin` binary as a new UCI engine.
3. Start playing games, analyzing positions, or testing the engine's strength.

## Code Structure

The source code is structured for clarity and modularity:

- **Bitboards**: Efficient representation of the chessboard for computations.
- **FEN Strings**: Predefined starting positions and utilities for custom positions.
- **UCI Protocol**: Core implementation to interact with chess GUIs and platforms.

## Acknowledgments

Special thanks to the following resources and contributors for their guidance and inspiration:

- [Maxim Korzh](https://github.com/maksimKorzh)
- [ChessProgramming.org](https://www.chessprogramming.org)
- [TalkChess (Computer Chess Club)](https://talkchess.com/)

## License

This project is licensed under the MIT License. See the LICENSE file for details.

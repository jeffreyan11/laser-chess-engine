# Laser
Laser is a UCI-compliant chess engine written in C++11, by Jeffrey An and Michael An.

For the latest release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).

A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com.

A few ideas and inspiration came from

Stockfish (https://stockfishchess.org/):
- "bench" command
- Using SWAR for midgame/endgame evaluation
- Razoring, singular extension implementations

Crafty, EXChess:
- Lazy SMP implementation

Rebel:
- King safety

This project would not have been possible without Cute Chess, the primary tool used for testing: (http://cutechess.com/).


### Engine Strength
CCRL 40/40

Laser 1.1: 68th, 2775 elo as of May 21, 2016

CCRL 40/4

Laser 1.1: 65-66th, 2770 elo as of May 21, 2016

CEGT Blitz - Best Single Versions

Laser 1.1: 70th, 2596 elo as of May 24, 2016


### Implementation Details
- Lazy SMP up to 128 threads
- Fancy magic bitboards for a 4.5 sec PERFT 6.
- Evaluation with piece square tables, basic king safety, isolated/doubled/passed/backwards pawns, and mobility
- A transposition table with Zobrist hashing, a two bucket system, and 16 MB default size
- An evaluation cache
- Fail-hard principal variation search
  - Adaptive null move pruning
  - Late move reduction
  - Futility pruning
  - Razoring
  - Move count based pruning (late move pruning)
  - Check and singular extensions
- Quiescence search with captures, queen promotions, and checks on the first two plies
- Move ordering
  - Internal iterative deepening when a hash move is not available
  - Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) to order captures
  - Killer and history heuristics to order quiet moves


### Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.
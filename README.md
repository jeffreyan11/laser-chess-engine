# Laser
Laser is a UCI-compliant chess engine written in C++11, by Jeffrey An and Michael An.

For the latest release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).


### Thanks To:
- The Chess Programming Wiki, which is a great resource for beginning chess programmers, and was consulted frequently for this project: https://chessprogramming.wikispaces.com
- The authors of Stockfish, Crafty, EXChess, Rebel, Texel, and all other open-source engines for providing inspiration for many parts of this project
- The engine testers, for uncovering bugs, providing high quality games and ratings, and giving us motivation to improve
- Cute Chess, the primary tool used for testing: (http://cutechess.com/)


### To Dos
 - Chess960 support
 - Improved pruning rules
 - More efficient PERFT and eval


### Engine Strength
CCRL 40/40

Laser 1.2: 57th, 2868 elo as of Oct 22, 2016

CCRL 40/4

Laser 1.2: 54th, 2873 elo as of Oct 22, 2016

CEGT Blitz - Best Single Versions

Laser 1.2: 59th, 2702 elo as of Oct 16, 2016


### Implementation Details
- Lazy SMP up to 128 threads
- Fancy magic bitboards for a 4.5 sec PERFT 6.
- Evaluation with piece square tables, basic king safety, isolated/doubled/passed/backwards pawns, and mobility
  - Tuned with reinforcement learning and a variation of Texel's Tuning Method
- A transposition table with Zobrist hashing, a two bucket system, and 16 MB default size
- An evaluation cache
- Syzygy tablebase support
- Fail-soft principal variation search
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
# Laser
Laser is a UCI-compliant chess engine written in C++11 by Jeffrey An and Michael An.

For the latest release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).


### Thanks To:
- The Chess Programming Wiki, which is a great resource for beginning chess programmers, and was consulted frequently for this project: https://chessprogramming.wikispaces.com
- The authors of Stockfish, Crafty, EXChess, Rebel, Texel, and all other open-source engines for providing inspiration and great ideas to move Laser forward
- The engine testers, for uncovering bugs, providing high quality games and ratings, and giving us motivation to improve
- Cute Chess, the primary tool used for testing: (http://cutechess.com/)


### To Dos
 - Enumerate or typedef basic values such as color, piece type, and scores
 - Chess960 support
 - Improved pruning rules
 - More efficient PERFT and eval


### Engine Strength
- **CCRL 40/40:** 41st, 2949 ELO as of January 28, 2017
- **CCRL 40/4:** 48th, 2914 ELO as of January 28, 2017
- **CEGT 40/4 (Best Single Versions):** 33rd, 2817 ELO as of January 20, 2017
- **CEGT 40/20 (Best Single Versions):** 33rd, 2826 ELO as of January 22, 2017


### Implementation Details
- Lazy SMP up to 128 threads
- Fancy magic bitboards for a 4.5 sec PERFT 6 @2.2 GHz (no bulk counting).
- Evaluation
  - Tuned with reinforcement learning, coordinate descent, and a variation of Texel's Tuning Method
  - Piece square tables
  - King safety, pawns shields and storms
  - Isolated/doubled/passed/backwards pawns
  - Mobility
  - Outposts
  - Basic threat detection and pressure on weak pieces
- A two-bucket transposition table with Zobrist hashing, 16 MB default size
- An evaluation cache
- Syzygy tablebase support
- Fail-soft principal variation search
  - Adaptive null move pruning, late move reduction
  - Futility pruning, razoring, move count based pruning (late move pruning)
  - Check and singular extensions
- Quiescence search with captures, queen promotions, and checks on the first two plies
- Move ordering
  - Internal iterative deepening when a hash move is not available
  - Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) to order captures
  - Killer and history heuristics to order quiet moves


### Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.
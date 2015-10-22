# Laser
Laser is a UCI-compliant chess engine written in C++11, by Jeffrey An and Michael An.

For the latest (semi-)stable release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).

A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com.
A few ideas and inspiration came from Stockfish (https://stockfishchess.org/):
- Using SWAR for midgame/endgame evaluation
- Razoring implementation


### Engine Strength
We are now on the CCRL 40/4!

Laser 0.1: 179-180th, 2377 elo as of Oct 11, 2015.

Most recent estimated ELO for Laser 0.2 beta is 2580 +/- 40 on the CCRL 40/4 list (tested 17/10/15) and 2597 +/- 80 (tested 18/10/15) on the CCRL 40/40 list (95% CI), tested using cutechess-cli (http://cutechess.com/).

The code was compiled with MinGW, GCC version 4.9.2. All engines used their default hash sizes and one thread on an i5-4690 using the appropriate equivalent time controls.

Engines used for testing: Fridolin 2.00, Maverick 1.0, and Jellyfish 1.1. Thanks to the respective authors for making their engines available for public use.


### Engine Personality
- Strong in complex tactical lines (even more so than most other engines at this level)
- Strong in ultra-bullet, scales poorly with time
- Poor endgame play (there is little to no endgame code)


### Implementation Details
- Fancy magic bitboards for a 5 to 6 sec PERFT 6.
- Evaluation with piece square tables, basic king safety, isolated/doubled/passed pawns, and mobility
- A transposition table with Zobrist hashing, a two bucket system, and 16 MB default size
- An evaluation cache
- Fail-hard principal variation search
  - Adaptive null move pruning
  - Late move reduction
  - Futility pruning
  - Razoring
  - Move count based pruning (late move pruning)
- Quiescence search with captures, queen promotions, and checks on the first three plies
- Move ordering
  - Internal iterative deepening when a hash move is not available
  - Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) to order captures
  - Killer and history heuristics to order quiet moves


### Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.


### Known issues:
- Hash errors on PV nodes cause strange moves and give illegal PVs, also causing feedPVToTT() to fail
- tunemagic command leaks a large amount of memory
- SEE, MVV/LVA scoring functions handle en passant as if no pawn was captured
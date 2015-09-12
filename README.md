# Laser
Laser is a UCI-compliant chess engine written in C++11, by Jeffrey An and Michael An.

For the latest (semi-)stable release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).


### Engine Strength
Estimated ELO is 2547 +/- 50 on the CCRL 40/4 list (tested 12/9/15) and 2358 +/- 90 (tested 26/8/15) on the CCRL 40/40 list (95% CI), tested using cutechess-cli (http://cutechess.com/).

The code was compiled with MinGW, GCC version 4.9.2. All engines used their default hash sizes and one thread on an i5-4690 using the appropriate equivalent time controls.

Link to the CCRL website: http://www.computerchess.org.uk/ccrl/

Engines used for testing: Delphil 3.2, Fridolin 2.00, Maverick 1.0, and Jellyfish 1.1. Thanks to the respective authors for making their engines available for public use.


### Engine Personality
- Strong in complex tactical lines (even more so than most other engines at this level)
- Strong in ultra-bullet, scales poorly with time
- Extremely poor endgame play (there is little to no endgame code)


### Implementation Details
- Fancy magic bitboards for a 5 to 6 sec PERFT 6.
- Evaluation with piece square tables, basic king safety, isolated/doubled/passed pawns, and mobility
- A transposition table with Zobrist hashing, a two bucket system, and 16 MB default size
- Fail-hard principal variation search
  - Adaptive null move pruning
  - Late move reduction
  - Futility pruning
  - Razoring
  - Move count based pruning (late move pruning)
- Internal iterative deepening when a hash move is not available
- Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) to order captures
- Killer and history heuristics to order quiet moves
- Quiescence search with captures, queen promotions, and checks on the first three plies


### Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.

A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com


### Known issues:
- feedPVToTT() crashes, possibly because the PV collected is not actually the PV
- Hash errors can cause strange moves and give illegal PVs?
- tunemagic command leaks a large amount of memory
- SEE, MVV/LVA scoring functions handle en passant as if no pawn was captured
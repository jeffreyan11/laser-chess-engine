# Laser
Laser is a UCI-compliant chess engine written in C++11, by Jeffrey An and Michael An.

For the latest release and previous versions, check https://github.com/jeffreyan11/uci-chess-engine/releases. Compiled binaries for 64-bit Windows are included.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).

A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com.

A few ideas and inspiration came from

Stockfish (https://stockfishchess.org/):
- "bench" command
- Using SWAR for midgame/endgame evaluation
- Razoring implementation
- Singular extension implementation

Crafty:
- Lockless hashing by XORing Zobrist keys and hash data

EXChess:
- Lazy SMP implementation

A thanks also to Cute Chess, the primary tool used for testing: (http://cutechess.com/).


### Engine Strength
CCRL 40/40

Laser 0.2.1: 92nd, 2641 elo as of Dec 18, 2015

CCRL 40/4

Laser 0.2.1: 103-104th, 2606 elo as of Nov 15, 2015

Laser 0.1: 179-180th, 2377 elo as of Oct 11, 2015

CEGT Blitz - Best Single Versions

Laser 0.2.1: 107th, 2414 elo as of Nov 15, 2015


### Engine Personality
- Materialistic
- Overextends pawns, especially in quiet positions
- Poor endgame play (there is little endgame code)
- Poor evaluation of material imbalances


### Implementation Details
- Lazy SMP up to 32 threads
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
  - Check and singular extensions
- Quiescence search with captures, queen promotions, and checks on the first three plies
- Move ordering
  - Internal iterative deepening when a hash move is not available
  - Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) to order captures
  - Killer and history heuristics to order quiet moves


### Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.


### Known issues:
- Gives incorrect mate scores for long mates
- SMP bugs:
  - hashfull goes above 1000
- Hash errors on PV nodes cause strange moves and give illegal PVs, also causing feedPVToTT() to fail
- SEE, MVV/LVA scoring functions handle en passant as if no pawn was captured
# Laser
Laser is a UCI-compliant chess engine written in C++11.

After being compiled, the executable can be run with any UCI chess GUI, such as Arena (http://www.playwitharena.com/) or Tarrasch (http://www.triplehappy.com/).

Estimated ELO is 2400 +/- 100 on the CCRL 40/4 list, tested using cutechess-cli (http://cutechess.com/). The tests were performed on one core of a i5-5200U processor with 40/120 sec time controls. The code was compiled with MinGW, GCC version 4.9.2.


## Implementation Details
The implementation uses fancy magic bitboards for a fairly fast getLegalMoves and doMove (~5.2 sec PERFT 6).

The search is a fail-hard principal variation search. A transposition table, null move pruning, futility pruning, late move reduction, and move count based pruning are used in the main search. Internal iterative deepening is used when a hash move is not available. Static exchange evaluation (SEE) and Most Valuable Victim / Least Valuable Attacker (MVV/LVA) are used to order captures, and the killer and history heuristics are used to order quiet moves.

A basic quiescence search with captures, queen promotions, and checks on the first three plies is implemented. Pruning in quiescence is done with delta pruning and SEE. Move ordering is done purely with MVV/LVA.

The hash table uses Zobrist hashing and a two bucket system and has a default 16 MB size.


## Notes
The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support). For older or 32-bit systems, set the preprocessor flag USE_INLINE_ASM in common.h to false.


A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com


## Known issues:

isCheckMove() ignores en passant and castling

tunemagic command leaks a large amount of memory

Futility pruning can fail low even if the position is a stalemate

SEE, MVV/LVA scoring functions handle en passant as if no pawn was captured
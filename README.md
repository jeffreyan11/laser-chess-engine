# uci-chess-engine
A UCI-compliant chess engine.

The code and Makefile support gcc on Linux and MinGW on Windows for Intel Nehalem (2008-2010) and later processors only (due to popcnt instruction support).

The implementation uses fancy magic bitboards for a fairly fast getLegalMoves and doMove (~6.2 sec PERFT 6).

The search is a fail-hard principal variation, null window search (PVS, NWS). A transposition table, null move pruning, internal iterative deepening, static exchange evaluation (SEE), Most Valuable Victim, Least Valuable Attacker (MVV/LVA), and the killer heuristic are used to prune and move order in the main search.

A basic quiescence search with captures, queen promotions, and checks on the first ply is implemented. Pruning in quiescence is done with delta pruning and SEE. Move ordering is done purely with MVV/LVA.

The hash table uses Zobrist hashing and a two bucket system and has a default 16 MB size.


A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com

Estimated ELO is currently 2100-2200, tested using Stockfish 280615 64 BMI2 (https://stockfishchess.org/) (Commit 112607b on https://github.com/official-stockfish/Stockfish) and cutechess-cli (http://cutechess.com/). The tests were performed on one core of a i5-5200u processor with 15 sec + 0.05 sec/move time controls. The code was compiled with MinGW, GCC version 4.9.2.

Known issues:
isCheckMove() ignores en passant and castling and does not handle pawn/king moves to give discovered check correctly
tunemagic command leaks a large amount of memory
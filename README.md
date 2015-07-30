# uci-chess-engine
A UCI-compliant chess engine.

The implementation uses kindergarten bitboards for a fairly fast getLegalMoves and doMove (~0.3 sec PERFT 5, ~7.5 sec PERFT 6).

The search is a fail-hard principal variation, null window search (PVS, NWS). A basic quiescence search with captures and queen promotions is implemented. A transposition table, null move pruning, internal iterative deepening, and static exchange evaluation (SEE) are used to prune and move order in the main search.

The hash table uses Zobrist hashing and a single bucket system and has a default 16 MB size.


A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com

Estimated ELO is currently 1900-2000, tested using Stockfish 280615 64 BMI2 (https://stockfishchess.org/) (Commit 112607b on https://github.com/official-stockfish/Stockfish) and cutechess-cli (http://cutechess.com/). The tests were performed on one core of a i5-5200u processor with 15 sec + 0.05 sec/move time controls.
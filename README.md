# uci-chess-engine
A UCI-compliant chess engine.

The implementation uses kindergarten bitboards for a fairly fast getLegalMoves and doMove (~0.3 sec PERFT 5).
The search is a fail-hard principal variation, null window search (PVS, NWS). A basic quiescence search with only captures is implemented. A transposition table, null move pruning, internal iterative deepening, and static exchange evaluation (SEE) are used.
The hash table makes use of Zobrist hashing and has a default 16 MB size.

A special thanks to the Chess Programming Wiki, which was consulted frequently for this project: https://chessprogramming.wikispaces.com.

Estimated ELO is currently about 1800-1900, tested using Stockfish 6 (https://stockfishchess.org/) and cutechess-cli (http://cutechess.com/).
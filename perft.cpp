#include <chrono>
#include <sstream>
#include <vector>
#include "board.h"
#include "common.h"

uint64_t perft(Board &b, int color, int depth);

int captures = 0;

int main(int argc, char **argv) {
	initZobristTable();

    int depth = 5;
    if (argc == 2)
        depth = atoi(argv[1]);

    Board b; //= fenToBoard("8/2p5/3p3r/KP5r/1R3p1k/1p6/4P1P1/8 w - - 0 1");
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();

    cout << "Nodes: " << perft(b, 1, depth) << endl;
    cout << "Captures: " << captures << endl;

    auto end_time = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(
        end_time-start_time);

    cerr << time_span.count() << endl;

    return 0;
}

/*
 * Performs a PERFT.
 * 7/8/15: PERFT 5, 1.46 s (i5-2450m)
 * 7/11/15: PERFT 5, 1.22 s (i5-2450m)
 * 7/13/15: PERFT 5, 1.27/1.08 s (i5-2450m) before/after pass Board by reference
 * 7/14/15: PERFT 5, 0.86 s (i5-2450m)
 * 7/17/15: PERFT 5, 0.32 s (i5-2450m)
 */
uint64_t perft(Board &b, int color, int depth) {
	if (depth == 0)
		return 1;
	
	MoveList pl = b.getAllPseudoLegalMoves(color);
	uint64_t nodes = 0;
	
	for (unsigned int i = 0; i < pl.size(); i++) {
		Board copy = b.staticCopy();
		if (!copy.doPseudoLegalMove(pl.get(i), color))
			continue;

		/*if(!b->doPseudoLegalMove(pl.get(i), color)) {
			b->undoMove();
			continue;
		}*/

        if (isCapture(pl.get(i)))
            captures++;
		
		nodes += perft(copy, color^1, depth-1);
		
		//b.undoMove();
	}
	
	return nodes;
}

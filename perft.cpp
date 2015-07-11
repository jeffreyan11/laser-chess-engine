#include <chrono>
#include "board.h"
#include "common.h"

uint64_t perft(Board b, int color, int depth);

int captures = 0;

int main(int argc, char **argv) {
    Board b;
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();

    cout << "Nodes: " << perft(b, 1, 5) << endl;
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
 */
uint64_t perft(Board b, int color, int depth) {
	if(depth == 0)
		return 1;
	
	MoveList pl = b.getPseudoLegalMoves(color);
	MoveList pc = b.getPLCaptures(color);
	uint64_t nodes = 0;
	
	for(unsigned int i = 0; i < pl.size(); i++) {
		Board copy = b.staticCopy();
		if(!copy.doPLMove(pl.get(i), color))
			continue;
		/*if(!b->doPLMove(pl.get(i), color)) {
			b->undoMove();
			continue;
		}*/
		
		nodes += perft(copy, -color, depth-1);
		
		//b.undoMove();
	}

    pl.free();
	
	for(unsigned int i = 0; i < pc.size(); i++) {
		Board copy = b.staticCopy();
		if(!copy.doPLMove(pc.get(i), color))
			continue;
		
		captures++;
		
		nodes += perft(copy, -color, depth-1);
	}

    pc.free();
	
	return nodes;
}

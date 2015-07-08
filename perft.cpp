#include <chrono>
#include "board.h"
#include "standards.h"

uint64_t perft(Board *b, int color, int depth);

int captures = 0;

int main(int argc, char **argv) {
    Board *b = new Board();
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();

    cout << "Nodes: " << perft(b, 1, 4) << endl;
    cout << "Captures: " << captures << endl;

    auto end_time = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(
        end_time-start_time);

    cerr << time_span.count() << endl;

    return 0;
}

uint64_t perft(Board *b, int color, int depth) {
	if(depth == 0)
		return 1;
	
	vector<Move *> pl = b->getPseudoLegalMoves(color);
	vector<Move *> pc = b->getPLCaptures(color);
	uint64_t nodes = 0;
	
	for(unsigned int i = 0; i < pl.size(); i++) {
		Board *copy = b->copy();
		if(!copy->doPLMove(pl.at(i), color))
			continue;
		/*if(!b->doPLMove(pl.get(i), color)) {
			b->undoMove();
			continue;
		}*/
		
		nodes += perft(copy, -color, depth-1);
		
		//b.undoMove();
	}
	
	for(unsigned int i = 0; i < pc.size(); i++) {
		Board *copy = b->copy();
		if(!copy->doPLMove(pc.at(i), color))
			continue;
		
		captures++;
		
		nodes += perft(copy, -color, depth-1);
	}
	
	return nodes;
}

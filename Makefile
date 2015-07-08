CC          = g++
CFLAGS      = -Wall -ansi -pedantic -ggdb -std=c++0x -g -O3
OBJS        = board.o common.o

all: uci perft

uci: $(OBJS) uci.o
	$(CC) -o $@ $^

perft: $(OBJS) perft.o
	$(CC) -o $@ $^

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o uci perft

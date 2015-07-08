CC          = g++
CFLAGS      = -Wall -ansi -pedantic -ggdb -std=c++0x -g -O3
OBJS        = board.o

all: perft

perft: $(OBJS) perft.o
	$(CC) -o $@ $^

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o perft

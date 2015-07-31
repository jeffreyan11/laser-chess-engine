CC          = g++
CFLAGS      = -Wall -ansi -pedantic -ggdb -std=c++0x -g -O3
OBJS        = board.o common.o hash.o search.o

all: uci

uci: $(OBJS) uci.o
	$(CC) -o $@ $^

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o *.exe uci

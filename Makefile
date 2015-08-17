CC          = g++
CFLAGS      = -Wall -ansi -pedantic -ggdb -std=c++0x -g -O3
LDFLAGS     = -lpthread
OBJS        = board.o common.o hash.o search.o searchspace.o
ENGINENAME  = uci

all: uci

uci: $(OBJS) uci.o
	$(CC) -o $(ENGINENAME)$(EXT) $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o $(ENGINENAME)$(EXT).exe $(ENGINENAME)$(EXT)

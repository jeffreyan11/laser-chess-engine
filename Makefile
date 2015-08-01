CC          = g++
CFLAGS      = -Wall -ansi -pedantic -ggdb -std=c++0x -g -O3
LDFLAGS     =
OBJS        = board.o common.o hash.o search.o

ifneq ($(comp),mingw)
	LDFLAGS += -lpthread
endif

all: uci

uci: $(OBJS) uci.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o *.exe uci

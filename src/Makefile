# Laser, a UCI chess engine written in C++11.
# Copyright 2015-2018 Jeffrey An and Michael An
#
# Laser is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Laser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Laser.  If not, see <http://www.gnu.org/licenses/>.

CC      = g++
CFLAGS  = -Wall -Wextra -Wcast-qual -Wshadow -DNDEBUG -ansi -pedantic -std=c++11 -O3 -flto
LDFLAGS = -lpthread
OBJS    = bbinit.o board.o common.o eval.o evalhash.o hash.o search.o moveorder.o syzygy/tbprobe.o
EXE     = laser

ifeq ($(USE_STATIC), true)
	LDFLAGS += -static -static-libgcc -static-libstdc++
endif

ifneq ($(NOPOPCNT), true)
	CFLAGS += -msse3 -mpopcnt
endif

ifeq ($(BMI2), true)
	CFLAGS += -march=haswell
endif

all: uci

uci: $(OBJS) uci.o
	$(CC) -O3 -flto -o $(EXE)$(EXT) $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) -x c++ $< -o $@

clean:
	rm -f *.o syzygy/*.o $(EXE)$(EXT).exe $(EXE)$(EXT)

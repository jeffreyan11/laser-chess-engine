/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An
    Copyright (c) 2011-2015 Ronald de Man

    Laser is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Laser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Laser.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TBPROBE_H
#define TBPROBE_H

#include "../common.h"
#include "../board.h"

extern int TBlargest; // 5 if 5-piece tables, 6 if 6-piece tables were found.

void init_tablebases(char *path);
int probe_wdl(const Board &b, int *success);
int probe_dtz(const Board &b, int *success);
int root_probe(const Board *b, MoveList &rootMoves, ScoreList &scores, int &TBScore);
int root_probe_wdl(const Board *b, MoveList &rootMoves, ScoreList &scores, int &TBScore);

#endif

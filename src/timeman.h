/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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

#ifndef __TIME_H__
#define __TIME_H__

// Search modes
constexpr int TIME = 1;
constexpr int DEPTH = 2;
// constexpr int NODES = 3;
constexpr int MOVETIME = 4;

// Time management constants
constexpr int MOVE_HORIZON = 40; // expect this many moves left in the game
constexpr int ENDGAME_HORIZON_LIMIT = 80;
constexpr int MOVE_HORIZON_DEC = 8; // at the endgame horizon limit, move horizon decreases by this much
constexpr double TIME_FACTOR = 0.85; // timeFactor = log b / (b - 1) where b is branch factor
constexpr double MAX_TIME_FACTOR = 4.0; // do not spend more than this multiple of time over the limit
constexpr double ALLOTMENT_FACTORS[8] = {1.0, 0.99, 0.40, 0.30, 0.25, 0.22, 0.20, 0.18};
constexpr double MAX_USAGE_FACTORS[8] = {1.0, 0.99, 0.72, 0.63, 0.59, 0.56, 0.54, 0.52};

struct TimeManagement {
    int searchMode;
    int allotment;
    // Hard limit on time usage for this move, only for time-based searches
    int maxAllotment;
};

#endif
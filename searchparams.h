/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015 Jeffrey An and Michael An

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

#ifndef __SEARCHPARAMS_H__
#define __SEARCHPARAMS_H__

#include "common.h"

struct SearchParameters {
    int rootDepth;
    int ply;
    int nullMoveCount;
    int extensions;
    int selectiveDepth;
    int singularExtensions;
    ChessTime startTime;
    uint64_t timeLimit;
    Move killers[MAX_DEPTH][2];
    int historyTable[2][6][64];
    uint8_t rootMoveNumber;

    SearchParameters() {
        reset();
        resetHistoryTable();
    }

    void reset() {
        ply = 0;
        nullMoveCount = 0;
        extensions = 0;
        singularExtensions = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
        //resetHistoryTable();
    }
    
    void resetHistoryTable() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] = 0;
            }
        }
    }

    void ageHistoryTable(int depth, bool isEndOfSearch) {
        int posHistoryScale, negHistoryScale;
        if (isEndOfSearch) {
            posHistoryScale = depth * depth;
            negHistoryScale = depth;
        }
        else {
            posHistoryScale = depth;
            negHistoryScale = std::max(1, ((int) std::sqrt(depth)) / 2);
        }

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++) {
                    if (historyTable[i][j][k] > 0)
                        historyTable[i][j][k] /= posHistoryScale;
                    else
                        historyTable[i][j][k] /= negHistoryScale;
                }
            }
        }
    }
};

#endif
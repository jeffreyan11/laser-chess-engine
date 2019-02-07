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

#ifndef __SEARCHPARAMS_H__
#define __SEARCHPARAMS_H__

#include "common.h"

struct SearchParameters {
    int ply;
    int selectiveDepth;
    Move killers[MAX_DEPTH+1][2];
    int historyTable[2][6][64];
    int captureHistory[2][6][6][64];
    int **counterMoveHistory[6][64];
    int **followupMoveHistory[6][64];

    SearchParameters() {
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                counterMoveHistory[i][j] = new int *[6];
                for (int k = 0; k < 6; k++) {
                    counterMoveHistory[i][j][k] = new int[64];
                }
            }
        }
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                followupMoveHistory[i][j] = new int *[6];
                for (int k = 0; k < 6; k++) {
                    followupMoveHistory[i][j][k] = new int[64];
                }
            }
        }
        reset();
        resetHistoryTable();
    }

    ~SearchParameters() {
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 6; k++) {
                    delete[] counterMoveHistory[i][j][k];
                }
                delete[] counterMoveHistory[i][j];
            }
        }
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 6; k++) {
                    delete[] followupMoveHistory[i][j][k];
                }
                delete[] followupMoveHistory[i][j];
            }
        }
    }

    void reset() {
        ply = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
    }

    void resetHistoryTable() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] = 0;
            }
        }

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 6; k++) {
                    for (int l = 0; l < 64; l++)
                        captureHistory[i][j][k][l] = 0;
                }
            }
        }

        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 6; k++) {
                    for (int l = 0; l < 64; l++)
                        counterMoveHistory[i][j][k][l] = 0;
                }
            }
        }

        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 6; k++) {
                    for (int l = 0; l < 64; l++)
                        followupMoveHistory[i][j][k][l] = 0;
                }
            }
        }
    }
};

#endif
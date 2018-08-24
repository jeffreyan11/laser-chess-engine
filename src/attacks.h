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

#pragma once

#include <cstdint>

void initAttacks();

uint64_t knightAttacks(int sq);
uint64_t bishopAttacks(int sq, uint64_t occupied);
uint64_t rookAttacks(int sq, uint64_t occupied);
uint64_t queenAttacks(int sq, uint64_t occupied);
uint64_t kingAttacks(int sq);

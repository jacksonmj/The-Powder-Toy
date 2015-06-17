/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef Simulation_Coords_h
#define Simulation_Coords_h

#include "simulation/Config.hpp"

// Some classes to ease converting between integer and float coords and cell coords

class SimCoordFT;
class SimCoordF;
class SimCoordI;
class SimCellCoord;

class SimCoordFT
{
public:
	float x, y;
	// hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	SimCoordFT(float x_, float y_) {
		volatile float tmpx = x_, tmpy = y_;
		x = tmpx, y = tmpy;
	}
};

class SimCoordF
{
public:
	float x, y;
	constexpr SimCoordF(float x_, float y_) : x(x_), y(y_) {}
	SimCoordF(SimCoordFT c) : x(c.x), y(c.y) {}
	constexpr SimCoordF(SimCoordI c);
	constexpr SimCoordF(SimCellCoord c);
};

class SimCoordI
{
public:
	int x, y;
	constexpr SimCoordI(int x_, int y_) : x(x_), y(y_) {}
	SimCoordI(SimCoordFT c) : x(int(c.x+0.5f)), y(int(c.y+0.5f)) {}
	constexpr SimCoordI(SimCoordF c) : x(int(c.x+0.5f)), y(int(c.y+0.5f)) {}
	constexpr SimCoordI(SimCellCoord c);
};

class SimCellCoord
{
public:
	int x, y;
	constexpr SimCellCoord(int x_, int y_) : x(x_), y(y_) {}
	SimCellCoord(SimCoordFT c) : SimCellCoord(SimCoordI(c)) {}
	constexpr SimCellCoord(SimCoordF c) : SimCellCoord(SimCoordI(c)) {}
	constexpr SimCellCoord(SimCoordI c) : x(c.x/CELL), y(c.y/CELL) {}

	constexpr SimCoordI topLeft() const { return SimCoordI(x*CELL, y*CELL); }
	constexpr SimCoordI bottomRight() const { return SimCoordI(x*CELL+CELL-1, y*CELL+CELL-1); }
};

constexpr SimCoordF::SimCoordF(SimCoordI c) : x(c.x), y(c.y) {}
constexpr SimCoordF::SimCoordF(SimCellCoord c) : SimCoordF(SimCoordI(c)) {}

constexpr SimCoordI::SimCoordI(SimCellCoord c) : x(c.x*CELL), y(c.y*CELL) {}

#endif

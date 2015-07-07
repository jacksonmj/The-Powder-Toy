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

class SimPosFT;
class SimPosF;
class SimPosI;
class SimPosCell;

class SimPosFT
{
public:
	float x, y;
	SimPosFT() {}
	// hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	SimPosFT(float x_, float y_) {
		volatile float tmpx = x_, tmpy = y_;
		x = tmpx, y = tmpy;
	}
};

class SimPosF
{
public:
	float x, y;
	SimPosF () {}
	constexpr SimPosF(float x_, float y_) : x(x_), y(y_) {}
	SimPosF(SimPosFT c) : x(c.x), y(c.y) {}
	constexpr SimPosF(SimPosI c);
};

class SimPosI
{
public:
	int x, y;
	SimPosI() {}
	constexpr SimPosI(int x_, int y_) : x(x_), y(y_) {}
	SimPosI(SimPosFT c) : x(int(c.x+0.5f)), y(int(c.y+0.5f)) {}
	constexpr SimPosI(SimPosF c) : x(int(c.x+0.5f)), y(int(c.y+0.5f)) {}

};

class SimPosCell
{
public:
	int x, y;
	SimPosCell() {}
	constexpr SimPosCell(int x_, int y_) : x(x_), y(y_) {}
	SimPosCell(SimPosFT c) : SimPosCell(SimPosI(c)) {}
	constexpr SimPosCell(SimPosF c) : SimPosCell(SimPosI(c)) {}
	constexpr SimPosCell(SimPosI c) : x(c.x/CELL), y(c.y/CELL) {}

	constexpr SimPosI topLeft() const { return SimPosI(x*CELL, y*CELL); }
	constexpr SimPosI middle() const { return SimPosI(x*CELL+CELL/2, y*CELL+CELL/2); }
	constexpr SimPosI bottomRight() const { return SimPosI(x*CELL+CELL-1, y*CELL+CELL-1); }
};

constexpr SimPosF::SimPosF(SimPosI c) : x(c.x), y(c.y) {}

#endif

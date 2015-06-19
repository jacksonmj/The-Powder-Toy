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

#ifndef simulation_walls_WallsData_h
#define simulation_walls_WallsData_h

#include "simulation/CellsData.hpp"

// pointers to wall data
class WallsDataP
{
public:
	CellsUCharP wallType;
	CellsUCharP electricity;
	CellsFloatP fanVX, fanVY;
	WallsDataP() : wallType(nullptr), electricity(nullptr), fanVX(nullptr), fanVY(nullptr) {}
};

// pointers to constant walls data
class const_WallsDataP
{
public:
	const_CellsUCharP wallType;
	const_CellsUCharP electricity;
	const_CellsFloatP fanVX, fanVY;
	const_WallsDataP() : wallType(nullptr), electricity(nullptr), fanVX(nullptr), fanVY(nullptr) {}
	const_WallsDataP(const WallsDataP &p) : wallType(p.wallType), electricity(p.electricity), fanVX(p.fanVX), fanVY(p.fanVY) {}
};

// walls data (memory to store the data is allocated by this class)
class WallsData
{
public:
	CellsUChar wallType;
	CellsUChar electricity;
	CellsFloat fanVX, fanVY;
	WallsData();
	WallsData(const_WallsDataP src);
	WallsData& operator=(const_WallsDataP src);
	operator WallsDataP();
	operator const_WallsDataP() const;
};
void swap(WallsData& a, WallsData& b);

void WallsData_copy(const_WallsDataP src, WallsDataP dest);

#endif

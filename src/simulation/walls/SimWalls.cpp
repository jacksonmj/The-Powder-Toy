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


#include "simulation/CellsData.hpp"
#include "simulation/walls/SimWalls.hpp"
#include "simulation/Simulation.h"
#include <cmath>
#include <algorithm>
#include <utility>

SimWalls::SimWalls(Simulation *sim_) : sim(sim_)
{
	setManipData(data);
	clear();
}

SimWalls::~SimWalls() {}

void SimWalls::getData(WallsDataP destination)
{
	WallsData_copy(data, destination);
}
void SimWalls::setData(const_WallsDataP newData)
{
	WallsData_copy(newData, data);
}

void SimWalls::clear()
{
	clear_electricity();
	CellsData_fill<uint8_t>(data.wallType, 0);
}

void SimWalls::clear_electricity()
{
	CellsData_fill<uint8_t>(data.electricity, 0);
}

void SimWalls::clear_fanVelocity()
{
	CellsData_fill<float>(data.fanVX, 0.0f);
	CellsData_fill<float>(data.fanVY, 0.0f);	
}

void SimWalls::simBeforeUpdate()
{
	CellsData_subtract_sat(data.electricity, data.electricity, 1);
	if (gravwl_timeout>0)
		gravwl_timeout--;
}


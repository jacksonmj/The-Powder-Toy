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

#ifndef simulation_walls_SimWalls_h
#define simulation_walls_SimWalls_h

#include "simulation/CellsData.hpp"
#include "simulation/walls/WallsManipulator.hpp"
#include "simulation/walls/WallsData.hpp"

class Simulation;

class SimWalls : public WallsManipulator
{
protected:
	WallsData data;
	Simulation *sim;

public:
	SimWalls(Simulation *sim_);
	virtual ~SimWalls();

	void getData(WallsDataP destination);
	void setData(const_WallsDataP newData);
	void clear_electricity();
	void clear_fanVelocity();
	void clear();
	void simBeforeUpdate();

};

#endif

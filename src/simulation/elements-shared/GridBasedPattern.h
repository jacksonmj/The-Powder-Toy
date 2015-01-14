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

#ifndef Simulation_elemshared_GridBasedPattern_h
#define Simulation_elemshared_GridBasedPattern_h

#include "simulation/ElemDataShared.h"
#include "simulation/ElemDataSim.h"
#include "simulation/Simulation.h"
#include "common/tpt-stdint.h"
#include <algorithm>
#include <iostream>

template <int GridCellSizeX, int GridCellSizeY>
class GridBasedPattern_Pattern : public ElemDataShared
{
protected:
	bool pattern[GridCellSizeY][GridCellSizeX];
	int storedSizeX, storedSizeY;// for checking the pattern size matches when this class is acccessed through a static_cast of an ElemDataShared*
public:
	typedef bool (*PatternPtr)[GridCellSizeX];
	void setPattern(bool pattern_[GridCellSizeY][GridCellSizeX])
	{
		int x, y;
		for (y=0; y<GridCellSizeY; y++)
			for (x=0; x<GridCellSizeX; x++)
				pattern[y][x] = pattern_[y][x];
	}
	const PatternPtr getPattern(int sizeX, int sizeY)
	{
		if (sizeX==storedSizeX && sizeY==storedSizeY)
			return pattern;
		else
			return NULL;
	}
	GridBasedPattern_Pattern(SimulationSharedData *sd, int t) :
		ElemDataShared(sd,t)
	{
		storedSizeX = GridCellSizeX;
		storedSizeY = GridCellSizeY;
	}
	virtual ~GridBasedPattern_Pattern()
	{}
};

template <int GridCellSizeX, int GridCellSizeY, // size in pixels of one grid cell
		  int MarginLeft=CELL, int MarginRight=CELL, int MarginTop=CELL, int MarginBottom=CELL>// size in pixels of margin between simulation edge and grid edge
class GridBasedPattern_ElemDataSim : public ElemDataSim
{
protected:
	enum {
		CellCountX = (XRES-MarginLeft-MarginRight)/GridCellSizeX,
		CellCountY = (YRES-MarginTop-MarginBottom)/GridCellSizeY,
		MinX = MarginLeft, ActiveSizeX = CellCountX*GridCellSizeX,
		MinY = MarginTop,  ActiveSizeY = CellCountY*GridCellSizeY
	};
	int patternElement;
	bool cellOccupied[CellCountY][CellCountX];
	Observer_ClassMember<GridBasedPattern_ElemDataSim> obs_simBeforeUpdate;
public:
	typedef GridBasedPattern_Pattern<GridCellSizeX,GridCellSizeY> PatternContainer;
	void Simulation_BeforeUpdate()
	{
		if (!sim->elementCount[patternElement])
			return;

		// Get pattern data
		const bool (*pattern)[GridCellSizeX] = sim->elemDataShared<PatternContainer>(elementId)->getPattern(GridCellSizeX,GridCellSizeY);
		if (!pattern) // this will happen if grid cell sizes don't match
			return;

		// Work out which grid cells have at least one particle of this type inside them
		for (int i=0; i<=sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].type==patternElement)
			{
				int x = int(parts[i].x+0.5f)-MinX, y = int(parts[i].y+0.5f)-MinY;
				if (x<0 || y<0 || x>=ActiveSizeX || y>=ActiveSizeY)
					sim->part_kill(i);
				else
					cellOccupied[y/GridCellSizeY][x/GridCellSizeX] = true;
			}
		}

		// In each occupied grid cell, match the pattern provided
		int cellX, cellY, pattX, pattY;
		for (cellY=0; cellY<CellCountY; cellY++)
			for (cellX=0; cellX<CellCountX; cellX++)
			{
				if (cellOccupied[cellY][cellX])
				{
					int baseX = MinX+cellX*GridCellSizeX;
					int baseY = MinY+cellY*GridCellSizeY;
					for (pattY=0; pattY<GridCellSizeY; pattY++)
						for (pattX=0; pattX<GridCellSizeX; pattX++)
						{
							if (pattern[pattY][pattX])
							{
								// pattern says there should be a particle at this coord
								if (sim->pmap_find_one(baseX+pattX, baseY+pattY, patternElement)<0)
									sim->part_create(-1, baseX+pattX, baseY+pattY, patternElement);
							}
							else
							{
								// pattern says there should not be a particle at this coord
								sim->delete_position(baseX+pattX, baseY+pattY, patternElement);
							}
						}
				}
				cellOccupied[cellY][cellX] = false;
			}
	}
	GridBasedPattern_ElemDataSim(Simulation *s, int t) :
		ElemDataSim(s, t),
		patternElement(t),
		obs_simBeforeUpdate(sim->hook_beforeUpdate, this, &GridBasedPattern_ElemDataSim::Simulation_BeforeUpdate)
	{
		int cellX, cellY;
		for (cellY=0; cellY<CellCountY; cellY++)
			for (cellX=0; cellX<CellCountX; cellX++)
				cellOccupied[cellY][cellX] = false;

		if (!sim->elemDataShared<PatternContainer>(elementId)->getPattern(GridCellSizeX,GridCellSizeY))
			std::cerr << "GridBasedPattern cell sizes do not match for " << sim->elements[elementId].Identifier << std::endl;
	}
	virtual ~GridBasedPattern_ElemDataSim() {}
};

#endif

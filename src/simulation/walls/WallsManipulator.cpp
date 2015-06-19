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

#include "simulation/walls/WallsManipulator.hpp"
#include "simulation/CoordStack.h"

WallsManipulator::WallsManipulator() :
	gravwl_timeout(0)
{}
WallsManipulator::~WallsManipulator()
{}

void WallsManipulator::setManipData(WallsDataP data_)
{
	data = data_;
}

void WallsManipulator::makeSpark(SimCellCoord c)
{
	using CC = SimCellCoord;
	CoordStack<CC> cs;
	cs.push(c);

	do
	{
		CC c = cs.pop();
		int x1, x2;

		if (!canSpark(c))
			continue;

		// go left as far as possible
		x1 = x2 = c.x;
		while (x1>0)
		{
			if (!canSpark(CC(x1-1, c.y)))
				break;
			x1--;
		}
		while (x2<XRES/CELL-1)
		{
			if (!canSpark(CC(x2+1, c.y)))
				break;
			x2++;
		}

		// fill span
		for (int x=x1; x<=x2; x++)
			data.electricity[c.y][x] = 16;

		// fill children
		if (c.y>1 && x1==x2 &&
				isConductive(CC(x1-1, c.y-1)) && isConductive(CC(x1, c.y-1)) && isConductive(CC(x1+1, c.y-1)) &&
				!isConductive(CC(x1-1, c.y-2)) && isConductive(CC(x1, c.y-2)) && !isConductive(CC(x1+1, c.y-2)))
			makeSpark(CC(x1, c.y-2));// perpendicular wire crossing
		else if (c.y>0)
		{
			for (int x=x1; x<=x2; x++)
				if (canSpark(CC(x, c.y-1)))
				{
					if (x==x1 || x==x2 || c.y>=YRES/CELL-1 ||
							isConductive(CC(x-1, c.y-1)) || isConductive(CC(x+1, c.y-1)) ||
							isConductive(CC(x-1, c.y+1)) || !isConductive(CC(x, c.y+1)) || isConductive(CC(x+1, c.y+1)))
						cs.push(CC(x, c.y-1));
				}
		}
		if (c.y<YRES/CELL-2 && x1==x2 &&
				isConductive(CC(x1-1, c.y+1)) && isConductive(CC(x1, c.y+1)) && isConductive(CC(x1+1, c.y+1)) &&
				!isConductive(CC(x1-1, c.y+2)) && isConductive(CC(x1, c.y+2)) && !isConductive(CC(x1+1, c.y+2)))
			cs.push(CC(x1, c.y+2));// perpendicular wire crossing
		else if (c.y<YRES/CELL-1)
		{
			for (int x=x1; x<=x2; x++)
				if (canSpark(CC(x, c.y+1)))
				{
					if (x==x1 || x==x2 || c.y<0 ||
							isConductive(CC(x-1, c.y+1)) || isConductive(CC(x+1, c.y+1)) ||
							isConductive(CC(x-1, c.y-1)) || !isConductive(CC(x, c.y-1)) || isConductive(CC(x+1, c.y-1)))
						cs.push(CC(x, c.y+1));
				}
		}
	} while (cs.size()>0);
}

void WallsManipulator::fanVel_flood(SimCellCoord c, float newVX, float newVY)
{
	using CC = SimCellCoord;
	try
	{
		CoordStack<CC> cs;
		cs.push(c);

		do
		{
			CC c = cs.pop();
			int x1, x2;

			if (type(c)!=WL_FAN)
				continue;

			// go left as far as possible
			x1 = x2 = c.x;
			while (x1>0)
			{
				if (type(CC(x1-1, c.y))!=WL_FAN)
					break;
				x1--;
			}
			while (x2<XRES/CELL-1)
			{
				if (type(CC(x2+1, c.y))!=WL_FAN)
					break;
				x2++;
			}

			// fill span
			for (int x=x1; x<=x2; x++)
				data.wallType[c.y][x] = WL_FLOODHELPER;

			// fill children
			if (c.y>0)
			{
				for (int x=x1; x<=x2; x++)
				{
					if (type(CC(x, c.y-1))==WL_FAN)
						cs.push(CC(x, c.y-1));
				}
			}
			if (c.y<YRES/CELL-1)
			{
				for (int x=x1; x<=x2; x++)
				{
					if (type(CC(x, c.y+1))==WL_FAN)
						cs.push(CC(x, c.y+1));
				}
			}
		} while (cs.size()>0);

		for (size_t y=0; y<YRES/CELL; y++)
		{
			for (size_t x=0; x<XRES/CELL; x++)
			{
				if (data.wallType[y][x]==WL_FLOODHELPER)
				{
					data.wallType[y][x] = WL_FAN;
					fanVel(CC(x,y), newVX, newVY);
				}
			}
		}
	}
	catch (std::exception& e)
	{
		for (size_t y=0; y<YRES/CELL; y++)
		{
			for (size_t x=0; x<XRES/CELL; x++)
			{
				if (data.wallType[y][x]==WL_FLOODHELPER)
					data.wallType[y][x] = WL_FAN;
			}
		}
		throw;
	}
}

void WallsManipulator::setBorder(uint8_t wallType)
{
	for(int i = 0; i<(XRES/CELL); i++)
	{
		type(SimCellCoord(i,0), wallType);
		type(SimCellCoord(i,YRES/CELL-1), wallType);
	}
	for(int i = 1; i<((YRES/CELL)-1); i++)
	{
		type(SimCellCoord(0,i), wallType);
		type(SimCellCoord(XRES/CELL-1,i), wallType);
	}
}


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

#ifndef SIMULATION_H
#define SIMULATION_H

#include "simulation/Element.h"


// Defines for element transitions
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM


class Simulation
{
public:
	particle parts[NPART];
	Element elements[PT_NUM];
	
	int pfree;
	


	Simulation();
	void InitElements();
	
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		return i;
	}
	void part_free(int i)
	{
		parts[i].life = pfree;
		pfree = i;
	}
	
};

void Simulation_Compat_CopyData(Simulation *sim);

extern Simulation *globalSim; // TODO: remove this

#endif

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

#include "simulation/BasicData.hpp"
#include <algorithm>
#include <cstring>

Sim_BasicData::Sim_BasicData(std::shared_ptr<SimulationSharedData> sd) :
	simSD(sd),
	pfree(-1)
{
	elements = simSD->elements;
	elemDataShared_ = simSD->elemDataShared_;
	std::fill(elemDataSim_, elemDataSim_+PT_NUM, nullptr);

	// TODO: this class does not currently allocate elemDataSim (done by elements[t].Func_SimInit functions, but these take a Simulation*)

	clear();
}

Sim_BasicData::~Sim_BasicData()
{
	for (int t=0; t<PT_NUM; t++)
	{
		delete elemDataSim_[t];
	}
}

void Sim_BasicData::clear()
{
	pmap.clear();
	elementCount.fill(0);
	std::memset(parts, 0, sizeof(particle)*NPART);
	parts_count = 0;
	parts_lastActiveIndex = 0;
#ifdef DEBUG_PARTSALLOC
	for (int i=0; i<NPART; i++)
		partsFree[i] = true;
#endif
	for (int i=0; i<NPART-1; i++)
		parts[i].life = i+1;
	parts[NPART-1].life = -1;
	pfree = 0;
}

/* Recalculates elementCount[] values */
void Sim_BasicData::recalc_elementCount()
{
	elementCount.fill(0);
	parts_count = 0;
	for (int i=0; i<NPART; i++)
	{
		if (parts[i].type)
		{
			elementCount[parts[i].type]++;
			parts_count++;
		}
	}
}

/* Completely recalculates the entire pmap.
 * Avoid this as much as possible. */
void Sim_BasicData::recalc_pmap()
{
	int i;
	pmap.clear();
#ifdef DEBUG_PARTSALLOC
	for (i=0; i<NPART; i++)
		partsFree[i] = true;
#endif
	for (i=0; i<NPART; i++)
	{
		if (parts[i].type)
		{
			pmap_add(i, SimPosF(parts[i].x, parts[i].y), parts[i].type);
#ifdef DEBUG_PARTSALLOC
			partsFree[i] = false;
#endif
		}
	}
}

/* Recalculates the pfree/parts[].life linked list for particles with ID <= parts_lastActiveIndex.
 * This ensures that future particle allocations are done near the start of the parts array, to keep parts_lastActiveIndex low.
 * parts_lastActiveIndex is also decreased if appropriate.
 * Does not modify or even read any particles beyond parts_lastActiveIndex */
void Sim_BasicData::recalc_freeParticles()
{
	int lastPartUsed = 0;
	int lastPartUnused = -1;

	for (int i=0; i<=parts_lastActiveIndex; i++)
	{
		if (parts[i].type)
		{
			// TODO: do stuff with commented out code below
			// int x, y, t;
			/*t = parts[i].type;
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			if (x>=0 && y>=0 && x<XRES && y<YRES)
			{
				if (elements[t].Properties & TYPE_ENERGY)
					photons[y][x] = t|(i<<8);
				else
				{
					// Particles are sometimes allowed to go inside INVS and FILT
					// To make particles collide correctly when inside these elements, these elements must not overwrite an existing pmap entry from particles inside them
					if (!pmap[y][x] || (t!=PT_INVIS && t!= PT_FILT))
						pmap[y][x] = t|(i<<8);
					// Count number of particles at each location, for excess stacking check
					// (there are a few exceptions, including energy particles - currently no limit on stacking those)
					if (t!=PT_THDR && t!=PT_EMBR && t!=PT_FIGH && t!=PT_PLSM)
						pmap_count[y][x]++;
				}
			}*/
			lastPartUsed = i;
		}
		else
		{
			if (lastPartUnused<0) pfree = i;
			else parts[lastPartUnused].life = i;
			lastPartUnused = i;
		}
	}
	if (lastPartUnused==-1)
	{
		if (parts_lastActiveIndex>=NPART-1) pfree = -1;
		else pfree = parts_lastActiveIndex+1;
	}
	else
	{
		if (parts_lastActiveIndex>=NPART-1) parts[lastPartUnused].life = -1;
		else parts[lastPartUnused].life = parts_lastActiveIndex+1;
	}
	parts_lastActiveIndex = lastPartUsed;
}



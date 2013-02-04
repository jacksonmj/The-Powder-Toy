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

#ifndef Simulation_h
#define Simulation_h

#include "simulation/Element.h"

#include "powder.h"

// Defines for element transitions
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM

// to loop through all the particles in a certain position
// sim is the Simulation object
// x and y are the coordinates of the position to loop through
// r_count, r_i, and r_next are int variables (which need declaring before using this macro. r_count is the number of particles in this position, r_i the current particle index, and r_next is temporary storage for the next particle index
// If any particle in the current position is killed or moved except particle r_i and particles that have already been iterated over, then you must break out of the pmap position loop. 
#define FOR_PMAP_POSITION(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count, r_i=(sim)->pmap[(y)][(x)].first, r_next = (r_count ? (sim)->parts[r_i].pmap_next : -1); r_count; r_count--, r_i=r_next, r_next=(sim)->parts[r_i].pmap_next)
// exactly the same, except for use in Simulation class member functions
#define FOR_PMAP_POSITION_SIM(x, y, r_count, r_i, r_next) for (r_count=pmap[(y)][(x)].count, r_i=pmap[(y)][(x)].first, r_next = (r_count ? parts[r_i].pmap_next : -1); r_count; r_count--, r_i=r_next, r_next=parts[r_i].pmap_next)

class ElementDataContainer;
struct pmap_entry
{
	int count;
	int first;
};
typedef struct pmap_entry pmap_entry;

class Simulation
{
public:
	particle parts[NPART];
	//bool partsFree[NPART];
	int elementCount[PT_NUM];
	Element elements[PT_NUM];
	ElementDataContainer *elementData[PT_NUM];
	pmap_entry pmap[YRES][XRES];
	int pfree;
	


	Simulation();
	~Simulation();
	void InitElements();
	void Clear();
	bool Check();
	int part_create(int p, int x, int y, int t);
	void part_change_type(int i, int x, int y, int t);
	void part_kill(int i);
	
	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling

	bool is_element(int t)
	{
		return (t>=0 && t<PT_NUM && elements[t].Enabled);
	}
	// Most of the time, part_alloc and part_free should not be used directly unless you really know what you're doing. 
	// Use part_create and part_kill instead.
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		if (i>parts_lastActiveIndex)
			parts_lastActiveIndex = i;
		/*if (!partsFree[i])
			printf("Particle allocated that isn't free: %d\n", i);
		partsFree[i] = false;*/
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
		/*if (partsFree[i])
			printf("Particle freed twice: %d\n", i);
		partsFree[i] = true;*/
	}
	void pmap_add(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (pmap[y][x].count)
		{
			parts[pmap[y][x].first].pmap_prev = i;
			parts[i].pmap_next = pmap[y][x].first;
		}
		else
		{
			parts[i].pmap_next = -1;
		}
		parts[i].pmap_prev = -1;
		pmap[y][x].first = i;
		pmap[y][x].count++;
	}
	void pmap_remove(int i, int x, int y)
	{
		//if (pmap[y][x].count<=0)
		//	printf("count<0\n");
		// NB: all arguments are assumed to be within bounds
		if (parts[i].pmap_prev>=0)
			parts[parts[i].pmap_prev].pmap_next = parts[i].pmap_next;
		if (parts[i].pmap_next>=0)
			parts[parts[i].pmap_next].pmap_prev = parts[i].pmap_prev;
		if (pmap[y][x].first==i)
			pmap[y][x].first = parts[i].pmap_next;
		pmap[y][x].count--;
	}
	void part_move(int i, float nxf, float nyf)
	{
		pmap_remove(i, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f));
		parts[i].x = nxf;
		parts[i].y = nyf;
		pmap_add(i, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f), parts[i].type);
	}
	int pmap_find_one(int x, int y, int t)
	{
		int count = pmap[y][x].count;
		int i = pmap[y][x].first;
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (parts[i].type==t)
				return i;
		}
		return -1;
	}
	void pmap_reset();
};

void Simulation_Compat_CopyData(Simulation *sim);

extern Simulation *globalSim; // TODO: remove this

#endif

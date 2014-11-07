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

// Macros to loop through all the particles in a certain position
// sim is the Simulation object ("FOR_PMAP_POSITION(this, ...)" in Simulation methods)
// x and y are the coordinates of the position to loop through
// r_count, r_i, and r_next are int variables (which need declaring before using this macro). r_count is the number of particles left to iterate in this position, r_i the current particle index, and r_next is temporary storage for the next particle index
// If any particle in the current position is killed or moved except particle r_i and particles that have already been iterated over, then you must break out of the pmap position loop. 
#define FOR_PMAP_POSITION(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count, r_next = (sim)->pmap[(y)][(x)].first; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_NOENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count_notEnergy, r_next = (sim)->pmap[(y)][(x)].first; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_ONLYENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count-(sim)->pmap[(y)][(x)].count_notEnergy, r_next = (sim)->pmap[(y)][(x)].first_energy; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)


class ElementDataContainer;
struct pmap_entry
{
	int count;// number of particles, including energy particles
	int first;// ID of first particle
	int count_notEnergy;// number of particles, not including energy particles
	int first_energy;// ID of first energy particle, or if there are no energy particles, ID of last non-energy particle
};
typedef struct pmap_entry pmap_entry;

class Simulation
{
public:
	particle parts[NPART];
#ifdef DEBUG_PARTSALLOC
	bool partsFree[NPART];
#endif
	int elementCount[PT_NUM];
	Element elements[PT_NUM];
	ElementDataContainer *elementData[PT_NUM];
	pmap_entry pmap[YRES][XRES];
	int pfree;

	short heat_mode;//Will be a replacement for legacy_enable at some point. 0=heat sim off, 1=heat sim on, 2=realistic heat on (todo)

	Simulation();
	~Simulation();
	void InitElements();
	void Clear();
	bool Check();
	void UpdateParticles();
	void RecalcFreeParticles();
	void RecalcElementCounts();
	void pmap_reset();

	int part_create(int p, int x, int y, int t);
	void part_kill(int i);
	void part_kill(int i, int x, int y);
	bool part_change_type(int i, int x, int y, int t);

	int delete_position(int x, int y, int only_type=0, int except_id=-1);
	int delete_position_notEnergy(int x, int y, int only_type=0, int except_id=-1);

	// Functions for changing particle temperature, respecting temperature caps.
	// The _noLatent functions also set the stored transition energy of the particle to make it appear as though latent heat does not apply - the particles will change into the type that they should be at that temperature, instead of the temperature increase just contributing towards the stored transition energy
	// set_temp sets the temperature to a specific value, add_temp changes the temperature by the given amount (can be positive or negative)
	void part_set_temp(int i, float newTemp);
	void part_set_temp_noLatent(int i, float newTemp);
	void part_add_temp(int i, float change);
	void part_add_temp_noLatent(int i, float change);
	void part_add_energy(int i, float amount);

	void spark_particle_nocheck(int i, int x, int y);
	void spark_particle_nocheck_forceSPRK(int i, int x, int y);
	bool spark_particle(int i, int x, int y);
	bool spark_particle_conductiveOnly(int i, int x, int y);
	int spark_position(int x, int y);
	int spark_position_conductiveOnly(int x, int y);

	bool IsWallBlocking(int x, int y, int type);

	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling

	bool IsElement(int t) const
	{
		return (t>=0 && t<PT_NUM && elements[t].Enabled);
	}
	bool InBounds(int x, int y) const
	{
		return (x>=0 && y>=0 && x<XRES && y<YRES);
	}
	// Is this particle an element of type t, ignoring the current SPRKed status of this particle?
	bool part_cmp_conductive(const particle& p, int t) const
	{
		return (p.type==t || (p.type==PT_SPRK && p.ctype==t));
	}
	// Is this particle a sparkable element (though not necessarily able to be sparked immediately)?
	bool part_is_sparkable(const particle& p) const
	{
		if (p.type==PT_WIRE || p.type==PT_INST || (elements[p.type].Properties&PROP_CONDUCTS))
			return true;
		if (p.type==PT_SWCH && p.life >= 10)
			return true;
		if (p.type==PT_SPRK && p.ctype >= 0 && p.ctype < PT_NUM && ((elements[p.ctype].Properties&PROP_CONDUCTS) || p.ctype==PT_INST || p.ctype==PT_SWCH))
			return true;
		return false;
	}
	// Returns true if spark trying to travel between i1 and i2 is blocked by INSL (or other future spark blocking elements)
	// Plus a version for if integer coordinates are already known
	bool is_spark_blocked(int i1, int i2) const
	{
		if (pmap_find_one(((int)(parts[i1].x+0.5f) + (int)(parts[i2].x+0.5f))/2, ((int)(parts[i1].y+0.5f) + (int)(parts[i2].y+0.5f))/2, PT_INSL)>=0)
			return true;
		return false;
	}
	bool is_spark_blocked(int x1, int y1, int x2, int y2) const
	{
		if (pmap_find_one((x1+x2)/2, (y1+y2)/2, PT_INSL)>=0)
			return true;
		return false;
	}
	
	// Returns true if a particle of type t exists halfway between i1 and i2
	bool check_middle_particle_type(int i1, int i2, int t) const
	{
		if (pmap_find_one((int)((parts[i1].x + parts[i2].x)/2+0.5f), (int)((parts[i1].y + parts[i2].y)/2+0.5f), t)>=0)
			return true;
		else
			return false;
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
#ifdef DEBUG_PARTSALLOC
		if (!partsFree[i])
			printf("Particle allocated that isn't free: %d\n", i);
		partsFree[i] = false;
#endif
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle freed twice: %d\n", i);
		partsFree[i] = true;
#endif
	}
	void part_move(int i, int x, int y, float nxf, float nyf)
	{
		volatile float tmpx = nxf, tmpy = nyf;// volatile to hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle moved after free: %d\n", i);
		if ((int)(parts[i].x+0.5f)!=x || (int)(parts[i].y+0.5f)!=y)
			printf("Provided original coords wrong for part_move (particle %d): alleged %d,%d actual %d,%d\n", i, x, y, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f));
#endif
		parts[i].x = tmpx;
		parts[i].y = tmpy;
		int nx = (int)(parts[i].x+0.5f), ny = (int)(parts[i].y+0.5f);
		if (ny!=y || nx!=x)
		{
			if (nx<CELL || nx>=XRES-CELL || ny<CELL || ny>=YRES-CELL)
			{
				// part_kill removes the particle from pmap, so use the original coords so it's removed from the correct pmap location
				part_kill(i, x, y);
			}
			else
			{
				pmap_remove(i, x, y, parts[i].type);
				pmap_add(i, nx, ny, parts[i].type);
			}
		}
	}

	void pmap_add(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (elements[t].Properties&TYPE_ENERGY)
		{
			if (pmap[y][x].count-pmap[y][x].count_notEnergy)
			{
				// If there are some energy particles already, insert at head of energy particle list
				int prevHead = pmap[y][x].first_energy;
				if (pmap[y][x].count_notEnergy)
				{
					// If there are some non-energy particles, link to end of that list
					parts[i].pmap_prev = parts[prevHead].pmap_prev;
					parts[parts[prevHead].pmap_prev].pmap_next = i;
				}
				else
				{
					parts[i].pmap_prev = -1;
				}
				parts[i].pmap_next = prevHead;
				parts[prevHead].pmap_prev = i;
			}
			else if (pmap[y][x].count)
			{
				// If there are no energy particles, then first_energy is the last non-energy particle. Insert this particle after it.
				int i_prev = pmap[y][x].first_energy;
				parts[i_prev].pmap_next = i;
				parts[i].pmap_prev = i_prev;
				parts[i].pmap_next = -1;
			}
			else
			{
				parts[i].pmap_next = -1;
				parts[i].pmap_prev = -1;
			}
			pmap[y][x].first_energy = i;
			if (!pmap[y][x].count_notEnergy)
				pmap[y][x].first = i;
		}
		else
		{
			if (pmap[y][x].count)
			{
				parts[pmap[y][x].first].pmap_prev = i;
				parts[i].pmap_next = pmap[y][x].first;
			}
			else
			{
				parts[i].pmap_next = -1;
				// If this is the only particle, it is the last non-energy particle too (which is the ID stored in first_energy when there are no energy particles)
				pmap[y][x].first_energy = i;
			}
			parts[i].pmap_prev = -1;
			pmap[y][x].first = i;
		}
		pmap[y][x].count++;
		if (!(elements[t].Properties&TYPE_ENERGY))
			pmap[y][x].count_notEnergy++;
	}
	void pmap_remove(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (parts[i].pmap_prev>=0)
			parts[parts[i].pmap_prev].pmap_next = parts[i].pmap_next;
		if (parts[i].pmap_next>=0)
			parts[parts[i].pmap_next].pmap_prev = parts[i].pmap_prev;
		if (pmap[y][x].first==i)
			pmap[y][x].first = parts[i].pmap_next;

		int energyCount = pmap[y][x].count-pmap[y][x].count_notEnergy;
		if (energyCount<=1)
		{
			if (pmap[y][x].first_energy==i)
			{
				// energyCount==1 and is first_energy: this is the only energy particle left
				// energyCount==0 and is first_energy: this is the last non-energy particle
				// In both cases, set first_energy to pmap_prev so that first_energy is the ID of the last non-energy particle
				pmap[y][x].first_energy = parts[i].pmap_prev;
			}
		}
		else if (pmap[y][x].first_energy==i)
		{
			pmap[y][x].first_energy = parts[i].pmap_next;
		}

		pmap[y][x].count--;
		if (!(elements[t].Properties&TYPE_ENERGY))
			pmap[y][x].count_notEnergy--;
	}
	int pmap_find_one(int x, int y, int t) const
	{
		int count;
		if (elements[t].Properties&TYPE_ENERGY)
			count = pmap[y][x].count;
		else
			count = pmap[y][x].count_notEnergy;
		int i = pmap[y][x].first;
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (parts[i].type==t)
				return i;
		}
		return -1;
	}
	int pmap_find_one_conductive(int x, int y, int t) const
	{
		int count = pmap[y][x].count_notEnergy;
		int i = pmap[y][x].first;
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (part_cmp_conductive(parts[i], t))
				return i;
		}
		return -1;
	}
};

void Simulation_Compat_CopyData(Simulation *sim);

extern Simulation *globalSim; // TODO: remove this

#endif

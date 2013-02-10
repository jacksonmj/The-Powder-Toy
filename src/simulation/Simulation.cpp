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

#include "powder.h"
#include "misc.h"
#include "simulation/Element.h"
#include "simulation/ElementDataContainer.h"
#include "simulation/Simulation.h"
#include <cmath>

// Declare the element initialisation functions
#define ElementNumbers_Include_Decl
#define DEFINE_ELEMENT(name, id) void name ## _init_element(ELEMENT_INIT_FUNC_ARGS);
#include "simulation/ElementNumbers.h"


Simulation *globalSim = NULL; // TODO: remove this global variable

Simulation::Simulation() :
	pfree(-1)
{
	memset(elementData, 0, sizeof(elementData));
	Clear();
}

Simulation::~Simulation()
{
	int t;
	for (t=0; t<PT_NUM; t++)
	{
		if (elementData[t])
		{
			delete elementData[t];
			elementData[t] = NULL;
		}
	}
}

void Simulation::InitElements()
{
	#define DEFINE_ELEMENT(name, id) if (id>=0 && id<PT_NUM) { name ## _init_element(this, &elements[id], id); };
	#define ElementNumbers_Include_Call
	#include "simulation/ElementNumbers.h"

	Simulation_Compat_CopyData(this);
}

void Simulation::Clear()
{
	int t;
	for (t=0; t<PT_NUM; t++)
	{
		if (elementData[t])
		{
			elementData[t]->Simulation_Cleared(this);
		}
	}
	memset(elementCount, 0, sizeof(elementCount));

	int i;
	pmap_entry *pmap_flat = (pmap_entry*)pmap;
	for (i=0; i<XRES*YRES; i++)
		pmap_flat[i].count = 0;

	pfree = 0;
}


// Completely recalculates the entire pmap.
// Avoid this as much as possible.
void Simulation::pmap_reset()
{
	int i;
	pmap_entry *pmap_flat = (pmap_entry*)pmap;
	for (i=0; i<XRES*YRES; i++)
		pmap_flat[i].count = 0;
#ifdef DEBUG_PARTSALLOC
	for (i=0; i<NPART; i++)
		partsFree[i] = true;
#endif
	for (i=0; i<NPART; i++)
	{
		if (parts[i].type)
		{
			pmap_add(i, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f), parts[i].type);
#ifdef DEBUG_PARTSALLOC
			partsFree[i] = false;
#endif
		}
	}
}

// Looks for errors in the simulation data, such as pmap linked lists
// Only to help with development and fixing bugs, it shouldn't be needed during normal use
bool Simulation::Check()
{
	bool isGood = true;
	int i, x, y;
	for (y=0; y<YRES; y++)
	{
		for (x=0; x<XRES; x++)
		{
			if (!pmap[y][x].count)
				continue;
			int count = 0;
			for (i=pmap[y][x].first; i>=0; i=parts[i].pmap_next)
			{
				if (count>NPART)
				{
					printf("Infinite loop detected for pmap list %d,%d\n", x, y);
					isGood = false;
					break;
				}
				if (i>=NPART)
				{
					printf("Invalid particle index %d in pmap list for %d,%d\n", i, x, y);
					isGood = false;
					break;
				}
				count++;
				if (x!=(int)(parts[i].x+0.5f) || y!=(int)(parts[i].y+0.5f))
				{
					printf("coordinates do not match: particle %d at %d,%d is in pmap list for %d,%d\n", i, x, y, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f));
					isGood = false;
				}
				if (i==pmap[y][x].first)
				{
					if (parts[i].pmap_prev >= 0)
					{
						printf("First particle (%d) in pmap list for %d,%d does not have pmap_prev<0\n", i, x, y);
						isGood = false;
					}
				}
				else
				{
					if (parts[i].pmap_next>=0 && parts[i].pmap_next<NPART && parts[parts[i].pmap_next].pmap_prev != i)
					{
						printf("pmap_prev (%d) for next particle (%d) in the pmap list for %d,%d is not the current particle (%d)\n", parts[parts[i].pmap_next].pmap_prev, parts[i].pmap_next, x, y, i);
						isGood = false;
					}
				}
			}
			if (count!=pmap[y][x].count)
			{
				printf("Stored length of pmap list for %d,%d does not match the actual length\n", x, y);
				isGood = false;
			}
		}
	}
	for (i=0; i<NPART; i++)
	{
		if (parts[i].type)
		{
			if (i>parts_lastActiveIndex)
			{
				printf("Particle %d is in use, but the ID is greater than parts_lastActiveIndex = %d\n", i, parts_lastActiveIndex);
				isGood = false;
			}
			// check whether the particle is in the correct pmap list
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			int pmap_i, count = 0;
			bool foundPart = false;
			if (pmap[y][x].count)
			{
				for (pmap_i=pmap[y][x].first; pmap_i>=0; pmap_i=parts[pmap_i].pmap_next)
				{
					if (count>NPART || pmap_i>=NPART)
					{
						break;
					}
					count++;
					if (pmap_i==i)
					{
						foundPart = true;
						break;
					}
				}
			}
			if (!foundPart)
			{
				printf("Particle %d at %d,%d does not appear in the pmap list for that location\n", i, x, y);
				isGood = false;
			}
		}
	}

	// Check the linked list of free particles for loops
	// (usually indicates that a particle was killed twice, or that a particle's properties were modified after killing it)
	bool *partSeen = new bool[NPART];
	for (i=0; i<NPART; i++)
		partSeen[i] = false;
	i = pfree;
	while (i>=0)
	{
		if (partSeen[i])
		{
			printf("Particle %d appears twice in the list of free particles\n", i);
			isGood = false;
			break;
		}
		partSeen[i] = true;
		i = parts[i].life;
	}
	delete[] partSeen;

	if (!isGood)
	{
		// This is mainly here to give a line to break on in a debugger
		printf("Problems found in simulation data\n");
	}
	return isGood;
}

// the function for creating a particle
// p=-1 for normal creation (checks whether the particle is allowed to be in that location first)
// p=-3 to create without checking whether the particle is allowed to be in that location
// or p = a particle number, to replace a particle
int Simulation::part_create(int p, int x, int y, int t)
{
	// This function is only for actually creating particles.
	// Not for tools, or changing things into spark, or special brush things like setting clone ctype.

	int i, oldType = PT_NONE;
	if (x<0 || y<0 || x>=XRES || y>=YRES || t<=0 || t>=PT_NUM || !elements[t].Enabled)
	{
		return -1;
	}
	// If the element has a Func_Create_Override, use that instead of the rest of this function
	if (elements[t].Func_Create_Override)
	{
		int ret = (*(elements[t].Func_Create_Override))(this, p, x, y, t);

		// returning -4 means continue with part_create as though there was no override function
		// Useful if creation should be blocked in some conditions, but otherwise no changes to this function (e.g. SPWN)
		if (ret!=-4)
			return ret;
	}

	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, p, x, y, t))
			return -1;
	}

	if (p==-1)
	{
		// Check whether the particle can be created here

		// If there is a particle, only allow creation if the new particle can occupy the same space as the existing particle
		// If there isn't a particle but there is a wall, check whether the new particle is allowed to be in it
		// If there's no particle and no wall, assume creation is allowed
		if (pmap[y][x].count ? (eval_move(t, x, y, NULL)!=2) : (bmap[y/CELL][x/CELL] && eval_move(t, x, y, NULL)!=2))
		{
			return -1;
		}
		i = part_alloc();
	}
	else if (p==-3) // skip pmap checks, e.g. for sing explosion
	{
		i = part_alloc();
	}
	else if (p>=0) // Replace existing particle
	{
		int oldX = (int)(parts[p].x+0.5f);
		int oldY = (int)(parts[p].y+0.5f);
		oldType = parts[p].type;
		if (elements[oldType].Func_ChangeType)
		{
			(*(elements[oldType].Func_ChangeType))(this, p, oldX, oldY, oldType, t);
		}
		pmap_remove(p, oldX, oldY);
		i = p;
	}
	else // Dunno, act like it was p=-3
	{
		i = part_alloc();
	}

	// Check whether a particle was successfully allocated
	if (i<0)
		return -1;

	// Set some properties
	parts[i] = elements[t].DefaultProperties;
	parts[i].type = t;
	parts[i].x = (float)x;
	parts[i].y = (float)y;
#ifdef OGLR
	parts[i].lastX = (float)x;
	parts[i].lastY = (float)y;
#endif

	// Fancy dust effects for powder types
	if((elements[t].Properties & TYPE_PART) && pretty_powder)
	{
		int colr, colg, colb, randa;
		randa = (rand()%30)-15;
		colr = (COLR(elements[t].Colour)+sandcolour_r+(rand()%20)-10+randa);
		colg = (COLG(elements[t].Colour)+sandcolour_g+(rand()%20)-10+randa);
		colb = (COLB(elements[t].Colour)+sandcolour_b+(rand()%20)-10+randa);
		colr = colr>255 ? 255 : (colr<0 ? 0 : colr);
		colg = colg>255 ? 255 : (colg<0 ? 0 : colg);
		colb = colb>255 ? 255 : (colb<0 ? 0 : colb);
		parts[i].dcolour = COLRGB(colr, colg, colb);
	}

	// Set non-static properties (such as randomly generated ones)
	if (elements[t].Func_Create)
	{
		(*(elements[t].Func_Create))(this, i, x, y, t);
	}

	pmap_add(i, x, y, t);

	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, oldType, t);
	}

	elementCount[t]++;
	return i;
}

bool Simulation::part_change_type(int i, int x, int y, int t)//changes the type of particle number i, to t.  This also changes pmap at the same time.
{
	if (x<0 || y<0 || x>=XRES || y>=YRES || i>=NPART || t<0 || t>=PT_NUM)
		return false;

	if (t==parts[i].type)
		return true;

	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, i, x, y, t))
			return false;
	}

	int oldType = parts[i].type;
	if (oldType) elementCount[oldType]--;

	if (!ptypes[t].enabled)
		t = PT_NONE;

	parts[i].type = t;
	if (t) elementCount[t]++;
	// TODO: add stuff here if separating parts and energy parts in new linked list pmap
	/*if (ptypes[t].properties & TYPE_ENERGY)
	{
		photons[y][x] = t|(i<<8);
		if ((pmap[y][x]>>8)==i)
			pmap[y][x] = 0;
	}
	else
	{
		pmap[y][x] = t|(i<<8);
		if ((photons[y][x]>>8)==i)
			photons[y][x] = 0;
	}*/
	if (elements[oldType].Func_ChangeType)
	{
		(*(elements[oldType].Func_ChangeType))(this, i, x, y, oldType, t);
	}
	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, oldType, t);
	}
	return true;
}

void Simulation::part_kill(int i)//kills particle number i
{
	int x, y;
	int t = parts[i].type;

	x = (int)(parts[i].x+0.5f);
	y = (int)(parts[i].y+0.5f);

	if (t && elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, t, PT_NONE);
	}

	pmap_remove(i, x, y);
	if (t == PT_NONE) // TODO: remove this? (//This shouldn't happen anymore, but it's here just in case)
		return;
	if (t) elementCount[t]--;
	part_free(i);
}

/* spark_conductive turns a particle into SPRK and sets ctype, life, and temperature.
 * spark_all does something similar, but behaves correctly for WIRE and INST
 *
 * spark_conductive_attempt and spark_all_attempt do the same thing, except they check whether the particle can actually be sparked (is conductive, has life of zero) first. Remember to check for INSL though. 
 * They return true if the particle was successfully sparked.
*/

void Simulation::spark_all(int i, int x, int y)
{
	if (parts[i].type==PT_WIRE)
		parts[i].ctype = PT_DUST;
	else if (parts[i].type==PT_INST)
		INST_flood_spark(this, x, y);
	else
		spark_conductive(i, x, y);
}
void Simulation::spark_conductive(int i, int x, int y)
{
	int type = parts[i].type;
	part_change_type(i, x, y, PT_SPRK);
	parts[i].ctype = type;
	if (type==PT_WATR)
		parts[i].life = 6;
	else if (type==PT_SLTW) 
		parts[i].life = 5;
	else
		parts[i].life = 4;
	if (parts[i].temp < 673.0f && !legacy_enable && (type==PT_METL || type == PT_BMTL || type == PT_BRMT || type == PT_PSCN || type == PT_NSCN || type == PT_ETRD || type == PT_NBLE || type == PT_IRON))
	{
		parts[i].temp = parts[i].temp+10.0f;
		if (parts[i].temp > 673.0f)
			parts[i].temp = 673.0f;
	}
}
bool Simulation::spark_all_attempt(int i, int x, int y)
{
	if ((parts[i].type==PT_WIRE && parts[i].ctype<=0) || (parts[i].type==PT_INST && parts[i].life<=0))
	{
		spark_all(i, x, y);
		return true;
	}
	else if (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS))
	{
		spark_conductive(i, x, y);
		return true;
	}
	return false;
}
bool Simulation::spark_conductive_attempt(int i, int x, int y)
{
	if (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS))
	{
		spark_conductive(i, x, y);
		return true;
	}
	return false;
}

// Attempts to spark all PROP_CONDUCTS particles in a particular position
int Simulation::spark_conductive_position(int x, int y)
{
	int lastSparkedIndex = -1;
	int rcount, index, rnext;
	FOR_PMAP_POSITION_SIM(x, y, rcount, index, rnext)
	{
		int type = parts[index].type;
		if (!(elements[type].Properties & PROP_CONDUCTS))
			continue;
		if (parts[index].life!=0)
			continue;
		spark_conductive(index, x, y);
		lastSparkedIndex = index;
	}
	return lastSparkedIndex;
}

void Simulation_Compat_CopyData(Simulation* sim)
{
	// TODO: this can be removed once all the code uses Simulation instead of global variables
	parts = sim->parts;


	for (int t=0; t<PT_NUM; t++)
	{
		ptypes[t].name = mystrdup(sim->elements[t].Name);
		ptypes[t].pcolors = PIXRGB(COLR(sim->elements[t].Colour), COLG(sim->elements[t].Colour), COLB(sim->elements[t].Colour));
		ptypes[t].menu = sim->elements[t].MenuVisible;
		ptypes[t].menusection = sim->elements[t].MenuSection;
		ptypes[t].enabled = sim->elements[t].Enabled;
		ptypes[t].advection = sim->elements[t].Advection;
		ptypes[t].airdrag = sim->elements[t].AirDrag;
		ptypes[t].airloss = sim->elements[t].AirLoss;
		ptypes[t].loss = sim->elements[t].Loss;
		ptypes[t].collision = sim->elements[t].Collision;
		ptypes[t].gravity = sim->elements[t].Gravity;
		ptypes[t].diffusion = sim->elements[t].Diffusion;
		ptypes[t].hotair = sim->elements[t].PressureAdd_NoAmbHeat;
		ptypes[t].falldown = sim->elements[t].Falldown;
		ptypes[t].flammable = sim->elements[t].Flammable;
		ptypes[t].explosive = sim->elements[t].Explosive;
		ptypes[t].meltable = sim->elements[t].Meltable;
		ptypes[t].hardness = sim->elements[t].Hardness;
		ptypes[t].weight = sim->elements[t].Weight;
		ptypes[t].heat = sim->elements[t].DefaultProperties.temp;
		ptypes[t].hconduct = sim->elements[t].HeatConduct;
		ptypes[t].descs = mystrdup(sim->elements[t].Description);
		ptypes[t].state = sim->elements[t].State;
		ptypes[t].properties = sim->elements[t].Properties;
		ptypes[t].update_func = sim->elements[t].Update;
		ptypes[t].graphics_func = sim->elements[t].Graphics;

		ptransitions[t].plt = sim->elements[t].LowPressureTransitionElement;
		ptransitions[t].plv = sim->elements[t].LowPressureTransitionThreshold;
		ptransitions[t].pht = sim->elements[t].HighPressureTransitionElement;
		ptransitions[t].phv = sim->elements[t].HighPressureTransitionThreshold;
		ptransitions[t].tlt = sim->elements[t].LowTemperatureTransitionElement;
		ptransitions[t].tlv = sim->elements[t].LowTemperatureTransitionThreshold;
		ptransitions[t].tht = sim->elements[t].HighTemperatureTransitionElement;
		ptransitions[t].thv = sim->elements[t].HighTemperatureTransitionThreshold;

		platent[t] = sim->elements[t].Latent;
	}
}

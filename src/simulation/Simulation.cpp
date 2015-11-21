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
#include "gravity.h"
#include "misc.h"
#ifdef LUACONSOLE
#include "luaconsole.h"
#endif

#include "simulation/Element.h"
#include "simulation/ElemDataSim.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationSharedData.h"
#include <cmath>
#include <algorithm>

#include "simulation/elements-shared/noHeatSim.h"

#include "simulation/elements/FIGH.h"
#include "simulation/elements/FILT.h"
#include "simulation/elements/INST.h"
#include "simulation/elements/PHOT.h"
#include "simulation/elements/PRTI.h"


ElemDataSim::~ElemDataSim()
{}

bool ElemDataSim::Check()
{
	return true;
}

std::array<SimPosDI,9> const Simulation::posdata_D1 =
{{{-1,-1},{ 0,-1},{ 1,-1},  {-1, 0},{ 0, 0},{ 1, 0},  {-1, 1},{ 0, 1},{ 1, 1}}};
std::array<SimPosDI,8> const Simulation::posdata_D1_noCentre =
{{{-1,-1},{ 0,-1},{ 1,-1},  {-1, 0},        { 1, 0},  {-1, 1},{ 0, 1},{ 1, 1}}};
std::array<SimPosDI,4> const Simulation::posdata_D1_noDiag_noCentre =
{{{ 0,-1},{-1, 0},{ 1, 0},{ 0, 1}}};
std::array<SimPosDI,25> const Simulation::posdata_D2 =
{{
	{-2,-2},{-1,-2},{0,-2},{ 1,-2},{ 2,-2},
	{-2,-1},{-1,-1},{0,-1},{ 1,-1},{ 2,-1},
	{-2, 0},{-1, 0},{0, 0},{ 1, 0},{ 2, 0},
	{-2, 1},{-1, 1},{0, 1},{ 1, 1},{ 2, 1},
	{-2, 2},{-1, 2},{0, 2},{ 1, 2},{ 2, 2}
}};
std::array<SimPosDI,24> const Simulation::posdata_D2_noCentre =
{{
	{-2,-2},{-1,-2},{0,-2},{ 1,-2},{ 2,-2},
	{-2,-1},{-1,-1},{0,-1},{ 1,-1},{ 2,-1},
	{-2, 0},{-1, 0},       { 1, 0},{ 2, 0},
	{-2, 1},{-1, 1},{0, 1},{ 1, 1},{ 2, 1},
	{-2, 2},{-1, 2},{0, 2},{ 1, 2},{ 2, 2}
}};

Simulation *globalSim = NULL; // TODO: remove this global variable

Simulation::Simulation(std::shared_ptr<SimulationSharedData> sd) :
	Sim_BasicData(sd),
	rngBase(),
	rng(&rngBase),
	air(this),
	walls(this),
	heat_mode(1),
	edgeMode(0),
	airMode(0),
	ambientHeatEnabled(false),
	ambientTemp(295.15f)
{
	int t;

	for (t=0; t<PT_NUM; t++)
	{
		if (elements[t].Func_SimInit)
			(*elements[t].Func_SimInit)(this, t);
	}

	clear();
	InitCanMove();
	Simulation_Compat_CopyData(this);
}

Simulation::~Simulation()
{}

void Simulation::clear()
{
	option_edgeMode(0);
	airMode = 0;
	air.clear();
	walls.clear();
	signs.clear();

	hook_cleared.Trigger();
	Sim_BasicData::clear();
}


// Looks for errors in the simulation data, such as pmap linked lists
// Only to help with development and fixing bugs, it shouldn't be needed during normal use
bool Simulation::Check()
{
	bool isGood = true;
	int i, x, y;

	for (i=0; i<PT_NUM; i++)
	{
		if (elemData(i))
			isGood = elemData(i)->Check() && isGood;
	}

	for (y=0; y<YRES; y++)
	{
		for (x=0; x<XRES; x++)
		{
			if (!pmap[y][x].count())
				continue;
			int count = 0;
			int count_notEnergy = 0;
			bool inEnergyList = false;
			int lastNotEnergyId = -1;
			int alleged_energyCount = pmap[y][x].count(PMapCategory::Energy);
			for (i=pmap[y][x].first(); i>=0; i=parts[i].pmap_next)
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
				bool isEnergyPart = !!(elements[parts[i].type].Properties&TYPE_ENERGY);
				count++;
				if (!isEnergyPart)
					count_notEnergy++;
				if (x!=(int)(parts[i].x+0.5f) || y!=(int)(parts[i].y+0.5f))
				{
					printf("coordinates do not match: particle %d at %d,%d is in pmap list for %d,%d\n", i, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f), x, y);
					isGood = false;
				}
				if (i==pmap[y][x].first())
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
				if (count > pmap[y][x].count(PMapCategory::NotEnergy))
					inEnergyList = true;

				if (isEnergyPart && !inEnergyList)
				{
					printf("Energy particle %d in pmap list for %d,%d is in the non-energy section\n", i, x, y);
					isGood = false;
				}
				if (!isEnergyPart && inEnergyList)
				{
					printf("Non-energy particle %d in pmap list for %d,%d is in the energy section\n", i, x, y);
					isGood = false;
				}
				if (!isEnergyPart)
					lastNotEnergyId = i;
			}
			if (alleged_energyCount==0 && count>0 && pmap[y][x].first_energy_!=lastNotEnergyId)
			{
				printf("pmap list for %d,%d has no energy particles but first_energy != ID of last non-energy particle (%d)\n", x, y, i);
				isGood = false;
			}
			if (count!=pmap[y][x].count())
			{
				printf("Stored total length of pmap list for %d,%d does not match the actual length\n", x, y);
				isGood = false;
			}
			if (count_notEnergy!=pmap[y][x].count(PMapCategory::NotEnergy))
			{
				printf("Stored non-energy length of pmap list for %d,%d does not match the actual length\n", x, y);
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
			SimPosI pos = SimPosF(parts[i].x, parts[i].y);
			int pmap_i, count = 0;
			bool foundPart = false;
			if (pmap(pos).count())
			{
				for (pmap_i=pmap(pos).first(); pmap_i>=0; pmap_i=parts[pmap_i].pmap_next)
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

	int *elementCountCheck = new int[PT_NUM];
	for (i=0; i<PT_NUM; i++)
	{
		elementCountCheck[i] = 0;
	}
	for (int i=0; i<NPART; i++)
	{
		if (parts[i].type)
			elementCountCheck[parts[i].type]++;
	}
	for (i=0; i<PT_NUM; i++)
	{
		if (elementCount[i]!=elementCountCheck[i])
		{
			printf("elementCount for %s is wrong: value is %d, should be %d\n", elements[i].Identifier.c_str(), elementCount[i], elementCountCheck[i]);
			isGood = false;
		}
	}
	delete[] elementCountCheck;

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
int Simulation::part_create(int p, SimPosF newPosF, int t)
{
	// This function is only for actually creating particles.
	// Not for tools, or changing things into spark, or special brush things like setting clone ctype.

	newPosF = pos_handleEdges(SimPosF(SimPosFT(newPosF)));
	SimPosI newPos = newPosF;

	int i, oldType = PT_NONE;
	if (!pos_isValid(newPos) || !element_isValidThing(t))
	{
		return -1;
	}
	// If the element has a Func_Create_Override, use that instead of the rest of this function
	if (elements[t].Func_Create_Override)
	{
		int ret = (*(elements[t].Func_Create_Override))(this, p, newPos.x, newPos.y, t);

		// returning -4 means continue with part_create as though there was no override function
		// Useful if creation should be blocked in some conditions, but otherwise no changes to this function (e.g. SPWN)
		if (ret!=-4)
			return ret;
	}

	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, p, newPos.x, newPos.y, t))
			return -1;
	}

	if (p==-1)
	{
		// Check whether the particle can be created here
		// If there is a particle, only allow creation if the new particle can occupy the same space as the existing particle
		// If there isn't a particle but there is a wall, check whether the new particle is allowed to be in it
		// If there's no particle and no wall, assume creation is allowed
		if (pmap(newPos).count() || walls.isProperWall(newPos))
		{
			MoveResult::Code moveResult = part_canMove(t, newPos);
			if (moveResult!=MoveResult::ALLOW && moveResult!=MoveResult::ALLOW_BUT_SLOW)
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
		SimPosI oldPos = SimPosF(parts[p].x, parts[p].y);
		oldType = parts[p].type;
		if (elements[oldType].Func_ChangeType)
		{
			(*(elements[oldType].Func_ChangeType))(this, p, oldPos.x, oldPos.y, oldType, t);
		}
		if (oldType) elementCount[oldType]--;
		pmap_remove(p, oldPos, oldType);
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
	parts[i].x = newPosF.x;
	parts[i].y = newPosF.y;
#ifdef OGLR
	parts[i].lastX = xf;
	parts[i].lastY = yf;
#endif

	// Fancy dust effects for powder types
	if((elements[t].Properties & TYPE_PART) && pretty_powder)
	{
		int colr, colg, colb;
		colr = PIXR(elements[t].Colour)+sandcolour*1.3+rng.randInt<-20,20>()+rng.randInt<-15,15>();
		colg = PIXG(elements[t].Colour)+sandcolour*1.3+rng.randInt<-20,20>()+rng.randInt<-15,15>();
		colb = PIXB(elements[t].Colour)+sandcolour*1.3+rng.randInt<-20,20>()+rng.randInt<-15,15>();
 		colr = colr>255 ? 255 : (colr<0 ? 0 : colr);
 		colg = colg>255 ? 255 : (colg<0 ? 0 : colg);
 		colb = colb>255 ? 255 : (colb<0 ? 0 : colb);
		parts[i].dcolour = COLARGB((rng.randInt<0,149>()), colr, colg, colb);
	}

	// Set non-static properties (such as randomly generated ones)
	if (elements[t].Func_Create)
	{
		(*(elements[t].Func_Create))(this, i, newPos.x, newPos.y, t);
	}

	pmap_add(i, newPos, t);

	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, newPos.x, newPos.y, oldType, t);
	}

	elementCount[t]++;
	return i;
}

bool Simulation::part_change_type(int i, SimPosI pos, int t)//changes the type of particle number i, to t.  This also changes pmap at the same time.
{
	if (!pos_isValid(pos) || i>=NPART || t<0 || t>=PT_NUM)
		return false;

	if (t==parts[i].type)
		return true;

	if (!t)
	{
#ifdef DEBUG_PARTSALLOC
		printf("Particle %d killed using part_change_type - part_kill should be used instead (to make it clear that particle properties should not be changed afterwards in the calling code)\n", i);
#endif
		part_kill(i);
		return true;
	}

	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, i, pos.x, pos.y, t))
			return false;
	}

	int oldType = parts[i].type;
	if (oldType) elementCount[oldType]--;

	if (!elements[t].Enabled)
		t = PT_NONE;

	parts[i].type = t;
	if (t) elementCount[t]++;

	if (pmap_category(oldType) != pmap_category(t) || !t)
	{
		pmap_remove(i, pos, oldType);
		if (t)
			pmap_add(i, pos, t);
	}

	if (elements[oldType].Func_ChangeType)
	{
		(*(elements[oldType].Func_ChangeType))(this, i, pos.x, pos.y, oldType, t);
	}
	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, pos.x, pos.y, oldType, t);
	}
	return true;
}

void Simulation::part_kill(int i, SimPosI pos)//kills particle number i, with integer coords already known (or to override coords)
{
	int t = parts[i].type;
	if (t && elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, pos.x, pos.y, t, PT_NONE);
	}

#ifdef DEBUG_PARTSALLOC
	if (partsFree[i] || !t)
		printf("Particle killed twice: %d\n", i);
#endif

	pmap_remove(i, pos, t);
	elementCount[t]--;
	part_free(i);
}

int Simulation::part_killAll(SimPosI pos, int only_type, int except_id)
{
	PMapCategory c = PMapCategory::All;
	if (only_type>0)
		c = pmap_category(only_type);

	int deletedCount = 0;
	FOR_SIM_PMAP_POS(this, c, pos, ri)
	{
		if (ri!=except_id && (!only_type || parts[ri].type==only_type))
		{
			part_kill(ri);
			deletedCount++;
		}
	}
	return deletedCount;
}


/* spark_particle turns a particle into SPRK and sets ctype, life, and temperature. It first checks whether the particle can actually be sparked (is conductive or WIRE or INST, and has life of zero) first. Remember to check for INSL though. 
 * 
 * Returns true if the particle was successfully sparked.
 * Arguments: particle index to spark, integer coordinates of that particle
 * 
 * spark_particle_nocheck sparks a particle without checking whether the particle is currently allowed to be sparked. No return value.
 * spark_particle_conductiveOnly only sparks PROP_CONDUCTS particles
*/

void Simulation::spark_particle_nocheck(int i, SimPosI pos)
{
	if (parts[i].type==PT_WIRE)
		parts[i].ctype = PT_DUST;
	else if (parts[i].type==PT_INST)
		Element_INST::flood_spark(this, pos.x, pos.y);
	else
		spark_particle_nocheck_forceSPRK(i, pos);
}
void Simulation::spark_particle_nocheck_forceSPRK(int i, SimPosI pos)
{
	// (split into a separate function so INST flood fill can use it to create SPRK without triggering another flood fill)
	int type = parts[i].type;
	part_change_type(i, pos, PT_SPRK);
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
bool Simulation::spark_particle(int i, SimPosI pos)
{
	if ((parts[i].type==PT_WIRE && parts[i].ctype<=0) || (parts[i].type==PT_INST && parts[i].life<=0) || (parts[i].type==PT_SWCH && parts[i].life==10) || (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS)))
	{
		spark_particle_nocheck(i, pos);
		return true;
	}
	return false;
}
bool Simulation::spark_particle_conductiveOnly(int i, SimPosI pos)
{
	if (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS))
	{
		spark_particle_nocheck(i, pos);
		return true;
	}
	return false;
}

// Sparks all PROP_CONDUCTS particles in a particular position
int Simulation::spark_position_conductiveOnly(SimPosI pos)
{
	int lastSparkedIndex = -1;
	FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, pos, index)
	{
		int type = parts[index].type;
		if (!(elements[type].Properties & PROP_CONDUCTS))
			continue;
		if (parts[index].life!=0)
			continue;
		spark_particle_nocheck(index, pos);
		lastSparkedIndex = index;
	}
	return lastSparkedIndex;
}

// Sparks all particles in a particular position
int Simulation::spark_position(SimPosI pos)
{
	int lastSparkedIndex = -1;
	FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, pos, index)
	{
		if (spark_particle(index, pos))
			lastSparkedIndex = index;
	}
	return lastSparkedIndex;
}

// Returns true if the element is blocked from moving to (or being created at) a position by a wall 
bool Simulation::isWallBlocking(SimPosCell c, int type)
{
	if (!walls.type(c))
		return false;

	switch (walls.type(c))
	{
	case WL_ALLOWGAS:
		return !(elements[type].Properties&TYPE_GAS);
	case WL_ALLOWENERGY:
		return !(elements[type].Properties&TYPE_ENERGY);
	case WL_ALLOWLIQUID:
		return elements[type].Falldown!=2;
	case WL_ALLOWSOLID:
		return elements[type].Falldown!=1;
	case WL_ALLOWAIR:
	case WL_WALL:
	case WL_WALLELEC:
		return true;
	case WL_EWALL:
		return !walls.electricity(c);
	default:
		return false;
	}
}

// Returns true if the element is destroyed by a wall when at a specified position
bool Simulation::isWallDeadly(SimPosCell c, int type)
{
	if (!walls.type(c))
		return false;

	bool isDeadly;
	switch (walls.type(c))
	{
	case WL_DESTROYALL:
		isDeadly = true;
		break;
	case WL_DETECT:
		isDeadly = (type==PT_METL || type==PT_SPRK);
		break;

	// Copy of isWallBlocking (copied to encourage the compiler to produce a single jump table)
	case WL_ALLOWGAS:
		isDeadly = !(elements[type].Properties&TYPE_GAS);
		break;
	case WL_ALLOWENERGY:
		isDeadly = !(elements[type].Properties&TYPE_ENERGY);
		break;
	case WL_ALLOWLIQUID:
		isDeadly = elements[type].Falldown!=2;
		break;
	case WL_ALLOWSOLID:
		isDeadly = elements[type].Falldown!=1;
		break;
	case WL_ALLOWAIR:
	case WL_WALL:
	case WL_WALLELEC:
		isDeadly = true;
		break;
	case WL_EWALL:
		isDeadly = !walls.electricity(c);
		break;
	// End isWallBlocking copy

	default:
		isDeadly = false;
		break;
	}

	return isDeadly && (type!=PT_STKM && type!=PT_STKM2 && type!=PT_FIGH);
}


void Simulation::InitCanMove()
{
	int movingType, destinationType;
	// can_move[moving type][type at destination]
	//  0 = No move/Bounce
	//  1 = Swap
	//  2 = Both particles occupy the same space, or moving into empty space
	//  3 = Varies, go run some extra checks

	// particles that don't exist shouldn't move...
	for (destinationType=0; destinationType < PT_NUM; destinationType++)
		can_move[0][destinationType] = MoveResult::BLOCK;
	// movement into empty space
	for (movingType=0; movingType < PT_NUM; movingType++)
		can_move[movingType][0] = MoveResult::ALLOW;

	//initialize everything else to swapping by default
	for (movingType=1; movingType < PT_NUM; movingType++)
		for (destinationType=1; destinationType < PT_NUM; destinationType++)
			can_move[movingType][destinationType] = MoveResult::DISPLACE;

	for (movingType=1; movingType < PT_NUM; movingType++)
	{
		for (destinationType=1; destinationType < PT_NUM; destinationType++)
		{
			// weight check, also prevents particles of same type displacing each other
			if (elements[movingType].Weight <= elements[destinationType].Weight || destinationType==PT_GEL)
				can_move[movingType][destinationType] = MoveResult::BLOCK;

			//other checks for NEUT and energy particles
			if (movingType==PT_NEUT && elements[destinationType].Properties&PROP_NEUTPASS)
				can_move[movingType][destinationType] = MoveResult::ALLOW;
			if (movingType==PT_NEUT && elements[destinationType].Properties&PROP_NEUTABSORB)
				can_move[movingType][destinationType] = MoveResult::DESTROY;
			if (movingType==PT_NEUT && elements[destinationType].Properties&PROP_NEUTPENETRATE)
				can_move[movingType][destinationType] = MoveResult::DISPLACE;
			if (destinationType==PT_NEUT && elements[movingType].Properties&PROP_NEUTPENETRATE)
				can_move[movingType][destinationType] = MoveResult::BLOCK;
			if (elements[movingType].Properties&TYPE_ENERGY && elements[destinationType].Properties&TYPE_ENERGY)
				can_move[movingType][destinationType] = MoveResult::ALLOW;
		}
	}
	for (destinationType = 0; destinationType < PT_NUM; destinationType++)
	{
		MoveResult::Code stkm_move = MoveResult::BLOCK;
		if (elements[destinationType].Properties & (TYPE_LIQUID | TYPE_GAS))
			stkm_move = MoveResult::ALLOW; // TODO: when breaking compatibility, maybe make STKM slow down when moving through these?
		if (!destinationType || destinationType==PT_SPAWN || destinationType==PT_SPAWN2)
			stkm_move = MoveResult::ALLOW;
		if (destinationType==PT_PRTO)
			stkm_move = MoveResult::ALLOW_BUT_SLOW;
		can_move[PT_STKM][destinationType] = stkm_move;
		can_move[PT_STKM2][destinationType] = stkm_move;
		can_move[PT_FIGH][destinationType] = stkm_move;

		//spark shouldn't move
		can_move[PT_SPRK][destinationType] = MoveResult::BLOCK;
	}
	for (movingType = 1; movingType < PT_NUM; movingType++)
	{
		// VACU and BHOL eat everyhing
		can_move[movingType][PT_BHOL] = MoveResult::DESTROY;
		can_move[movingType][PT_NBHL] = MoveResult::DESTROY;
		//nothing goes through stickmen
		// TODO: linked list pmap makes this redundant, since particles stacked underneath stickmen now block movement
		can_move[movingType][PT_STKM] = MoveResult::BLOCK;
		can_move[movingType][PT_STKM2] = MoveResult::BLOCK;
		can_move[movingType][PT_FIGH] = MoveResult::BLOCK;
		//INVS behaviour varies with pressure
		can_move[movingType][PT_INVIS] = MoveResult::DYNAMIC;
		//stop CNCT being displaced by other particles
		can_move[movingType][PT_CNCT] = MoveResult::BLOCK;
		//VOID and PVOD behaviour varies with powered state and ctype
		can_move[movingType][PT_PVOD] = MoveResult::DYNAMIC;
		can_move[movingType][PT_VOID] = MoveResult::DYNAMIC;
		//nothing moves through EMBR (although it's killed when it touches anything)
		can_move[movingType][PT_EMBR] = MoveResult::BLOCK;
		can_move[PT_EMBR][movingType] = MoveResult::BLOCK;
		if (elements[movingType].Properties&TYPE_ENERGY)
		{
			//VIBR and BVBR absorb energy particles
			can_move[movingType][PT_VIBR] = MoveResult::DESTROY;
			can_move[movingType][PT_BVBR] = MoveResult::DESTROY;

			// Energy particles are absorbed by pipe/portal during movement. TODO: do this for other particles too?
			can_move[movingType][PT_PIPE] = MoveResult::DYNAMIC;
			can_move[movingType][PT_PPIP] = MoveResult::DYNAMIC;
			can_move[movingType][PT_PRTI] = MoveResult::STORE;
		}
	}
	for (destinationType = 0; destinationType < PT_NUM; destinationType++)
	{
		//a list of lots of things PHOT can move through
		// TODO: replace with property?
		if (destinationType == PT_GLAS || destinationType == PT_PHOT || destinationType == PT_FILT || destinationType == PT_INVIS
		 || destinationType == PT_CLNE || destinationType == PT_PCLN || destinationType == PT_BCLN || destinationType == PT_PBCN
		 || destinationType == PT_WATR || destinationType == PT_DSTW || destinationType == PT_SLTW || destinationType == PT_GLOW
		 || destinationType == PT_ISOZ || destinationType == PT_ISZS || destinationType == PT_QRTZ || destinationType == PT_PQRT
		 || destinationType == PT_H2 || destinationType == PT_BIZR || destinationType == PT_BIZRG || destinationType == PT_BIZRS)
			can_move[PT_PHOT][destinationType] = MoveResult::ALLOW_BUT_SLOW;
		if (destinationType != PT_DMND && destinationType != PT_INSL && destinationType != PT_VOID && destinationType != PT_PVOD && destinationType != PT_VIBR && destinationType != PT_BVBR && destinationType != PT_PRTI && destinationType != PT_PRTO)
		{
			can_move[PT_PROT][destinationType] = MoveResult::ALLOW;
			can_move[PT_GRVT][destinationType] = MoveResult::ALLOW;
		}
	}
	//other special cases that weren't covered above
	can_move[PT_DEST][PT_DMND] = MoveResult::BLOCK;
	can_move[PT_DEST][PT_CLNE] = MoveResult::BLOCK;
	can_move[PT_DEST][PT_PCLN] = MoveResult::BLOCK;
	can_move[PT_DEST][PT_BCLN] = MoveResult::BLOCK;
	can_move[PT_DEST][PT_PBCN] = MoveResult::BLOCK;

	// TODO: some of these should really be MoveResult::ALLOW_BUT_SLOW to ensure reactions occur
	can_move[PT_NEUT][PT_INVIS] = MoveResult::ALLOW;
	can_move[PT_ELEC][PT_LCRY] = MoveResult::ALLOW;
	can_move[PT_ELEC][PT_EXOT] = MoveResult::ALLOW;
	can_move[PT_ELEC][PT_GLOW] = MoveResult::ALLOW;
	can_move[PT_PHOT][PT_LCRY] = MoveResult::DYNAMIC; //varies according to LCRY life

	can_move[PT_ELEC][PT_BIZR] = MoveResult::ALLOW;
	can_move[PT_ELEC][PT_BIZRG] = MoveResult::ALLOW;
	can_move[PT_ELEC][PT_BIZRS] = MoveResult::ALLOW;
	can_move[PT_BIZR][PT_FILT] = MoveResult::ALLOW;
	can_move[PT_BIZRG][PT_FILT] = MoveResult::ALLOW;

	can_move[PT_ANAR][PT_WHOL] = MoveResult::DESTROY; //WHOL eats ANAR
	can_move[PT_ANAR][PT_NWHL] = MoveResult::DESTROY;
	can_move[PT_ELEC][PT_DEUT] = MoveResult::DESTROY;
	can_move[PT_THDR][PT_THDR] = MoveResult::ALLOW;
	can_move[PT_EMBR][PT_EMBR] = MoveResult::ALLOW;
	can_move[PT_TRON][PT_SWCH] = MoveResult::DYNAMIC;
}

MoveResult::Code Simulation::part_canMove_dynamic(int pt, SimPosI newPos, int ri, MoveResult::Code result)
{
	int rt = parts[ri].type;
	if (rt==PT_LCRY)
	{
		if (pt==PT_PHOT)
			result = (parts[ri].life > 5)? MoveResult::ALLOW_BUT_SLOW : MoveResult::BLOCK;
	}
	else if (rt==PT_INVIS)
	{
		float pressure = air.pv.get(newPos);
		if (pressure>4.0f || pressure<-4.0f) result = MoveResult::ALLOW;
		else result = MoveResult::BLOCK;
	}
	else if (rt==PT_PVOD)
	{
		if (parts[ri].life == 10)
		{
			if(!parts[ri].ctype || (parts[ri].ctype==pt)!=(parts[ri].tmp&1))
				result = MoveResult::DESTROY;
			else
				result = MoveResult::BLOCK;
		}
		else result = MoveResult::BLOCK;
	}
	else if (rt==PT_VOID)
	{
		if(!parts[ri].ctype || (parts[ri].ctype==pt)!=(parts[ri].tmp&1))
			result = MoveResult::DESTROY;
		else
			result = MoveResult::BLOCK;
	}
	else if (pt == PT_TRON && rt == PT_SWCH)
	{
		if (parts[ri].life >= 10)
			result = MoveResult::ALLOW;
		else
			result = MoveResult::BLOCK;
	}
	else if (rt == PT_PIPE || rt == PT_PPIP)
	{
		if (!(parts[ri].tmp&0xFF))
			result = MoveResult::STORE;
		else
			result = MoveResult::BLOCK;
	}
	return result;
}

MoveResult::Code Simulation::part_canMove(int pt, SimPosI newPos, bool coordHandleEdgesDone)
{
	MoveResult::Code result = MoveResult::ALLOW;

	if (!coordHandleEdgesDone)
		newPos = pos_handleEdges(newPos);
	if (!pos_inMainArea(newPos))
	{
		if (!pos_isValid(newPos))
			return MoveResult::DESTROY;
		result = MoveResult::ALLOW_BUT_SLOW;
	}

	if (pmap(newPos).count(PMapCategory::NotEnergy))
	{
		FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, newPos, ri)
		{
			MoveResult::Code tmpResult = can_move[pt][parts[ri].type];
			if (tmpResult==MoveResult::DYNAMIC)
				tmpResult = part_canMove_dynamic(pt, newPos, ri, tmpResult);
			// Find the particle which restricts movement the most
			if (tmpResult<result)
				result = tmpResult;
		}
	}

	SimPosCell cellPos = newPos;
	if (walls.type(cellPos))
	{
		uint8_t wall = walls.type(cellPos);
		if (wall && isWallBlocking(cellPos, pt))
			return MoveResult::BLOCK;
		if (wall==WL_EHOLE && !walls.electricity(cellPos) && !(elements[pt].Properties&TYPE_SOLID))
		{
			bool foundSolid = false;
			FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, newPos, ri)
			{
				if (elements[parts[ri].type].Properties&TYPE_SOLID)
				{
					foundSolid = true;
					break;
				}
			}
			if (!foundSolid)
				return MoveResult::ALLOW;
		}
		if (wall==WL_DESTROYALL && result==MoveResult::ALLOW)
			return MoveResult::ALLOW_BUT_SLOW;
	}
	return result;
}

MoveResult::Code Simulation::part_move(int i, SimPosI srcPos, SimPosF destPosF)
{
	destPosF = pos_handleEdges(SimPosF(SimPosFT(destPosF)));
	SimPosI destPos = destPosF;

	if (parts[i].type == PT_NONE)
		return MoveResult::DESTROYED;

	if (!pos_isValid(destPos))
	{
		part_kill(i);
		return MoveResult::DESTROYED;
	}
	if (destPos==srcPos)
	{
		part_set_pos(i, srcPos, destPosF);
		return MoveResult::ALLOW;
	}

	int t = parts[i].type;
	MoveResult::Code moveCode = part_canMove(t, destPos, true);


	// Some checks which can't be done in part_canMove because source coords  and properties of the moving particle properties are unknown:
	// half-silvered mirror
	if (moveCode==MoveResult::BLOCK && t==PT_PHOT &&
			((pmap_find_one(destPos,PT_BMTL)>=0 && rng.chance<1,2>()) ||
	         pmap_find_one(srcPos,PT_BMTL)>=0))
		moveCode = MoveResult::ALLOW_BUT_SLOW;
	// block moving out of unpowered ehole
	if (walls.isClosedEHole(srcPos) && !walls.isClosedEHole(destPos))
		return MoveResult::BLOCK;
	// exploding GBMB does not move
	if(t==PT_GBMB&&parts[i].life>0)
		return MoveResult::BLOCK;
	//check below CNCT for another CNCT
	if (t==PT_CNCT && srcPos.y<destPos.y && pmap_find_one(srcPos+SimPosDI(0,1), PT_CNCT)>=0)
		return MoveResult::BLOCK;

	if (moveCode==MoveResult::BLOCK) //if no movement
	{
		if (elements[t].Properties & TYPE_ENERGY)
		{
			FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
			{
				int rt = parts[ri].type;
				if (!legacy_enable && t==PT_PHOT)//PHOT heat conduction
				{
					if (rt == PT_COAL || rt == PT_BCOL)
						parts[ri].temp = parts[i].temp;

					if (rt < PT_NUM && elements[rt].HeatConduct && (rt!=PT_HSWC||parts[ri].life==10) && rt!=PT_FILT)
						parts[i].temp = parts[ri].temp = tptmath::clamp_flt((parts[ri].temp+parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
				}
				if (rt==PT_CLNE || rt==PT_PCLN || rt==PT_BCLN || rt==PT_PBCN) {
					if (!parts[ri].ctype)
						parts[ri].ctype = t;
				}
				if (t==PT_PHOT)
					parts[i].ctype &= elements[rt].PhotonReflectWavelengths;
			}
		}
		return MoveResult::BLOCK;
	}
	else if (moveCode == MoveResult::ALLOW_BUT_SLOW || moveCode == MoveResult::ALLOW) //if occupy same space
	{
		FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
		{
			int rt = parts[ri].type;
			if (t==PT_PHOT)
			{
				parts[i].ctype &= elements[rt].PhotonReflectWavelengths;
				if (rt==PT_GLOW)
				{
					if (!parts[ri].life && rng.chance<1,30>())
					{
						parts[ri].life = 120;
						Element_PHOT::create_gain_photon(this, i);
					}
				}
				else if (rt==PT_FILT)
				{
					parts[i].ctype = Element_FILT::interactWavelengths(this, &parts[ri], parts[i].ctype);
				}
				else if (rt==PT_INVIS)
				{
					float pressure = air.pv.get(destPos);
					if (pressure<=4.0f && pressure>=-4.0f)
					{
						part_change_type(i,srcPos,PT_NEUT);
						parts[i].ctype = 0;
						part_set_pos(i, srcPos, destPosF);
						return MoveResult::CHANGED_TYPE;
					}
				}
				else if (rt==PT_BIZR || rt==PT_BIZRG || rt==PT_BIZRS)
				{
					part_change_type(i, srcPos, PT_ELEC);
					parts[i].ctype = 0;
					part_set_pos(i, srcPos, destPosF);
					return MoveResult::CHANGED_TYPE;
				}
				else if (rt == PT_H2 && !(parts[i].tmp&0x1))
				{
					part_change_type(i, srcPos, PT_PROT);
					parts[i].ctype = 0;
					parts[i].tmp2 = 0x1;

					part_create(ri, srcPos, PT_ELEC);
					part_set_pos(i, srcPos, destPosF);
					return MoveResult::CHANGED_TYPE;
				}
			}
			else if (t == PT_NEUT)
			{
				if (rt==PT_GLAS)
				{
					if (rng.chance<1,10>())
						Element_PHOT::create_cherenkov_photon(this, i);
				}
			}
			else if (t == PT_PROT)
			{
				if (rt == PT_INVIS)
				{
					part_change_type(i, srcPos, PT_NEUT);
					part_set_pos(i, srcPos, destPosF);
					return MoveResult::CHANGED_TYPE;
				}
			}
			else if (t==PT_BIZR || t==PT_BIZRG)
			{
				if (rt==PT_FILT)
				{
					parts[i].ctype = Element_FILT::interactWavelengths(this, &parts[ri], parts[i].ctype);
				}
			}
			else if (t==PT_ELEC)
			{
				if (rt==PT_GLOW)
				{
					part_change_type(i, srcPos, PT_PHOT);
					parts[i].ctype = 0x3FFFFFFF;
				}
			}
		}
		part_set_pos(i, srcPos, destPosF);
		return moveCode;
	}
	else if (moveCode == MoveResult::DESTROY)
	{
		// moving particle gets destroyed
		FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
		{
			int rt = parts[ri].type;
			if (rt==PT_BHOL || rt==PT_NBHL)
			{
				// blackhole heats up when it eats particles
				if (!legacy_enable)
				{
					part_add_temp(parts[ri], parts[i].temp/2);
				}
				part_kill(i);
				return MoveResult::DESTROYED;
			}
			else if (rt==PT_WHOL||rt==PT_NWHL)
			{
				if (t==PT_ANAR)
				{
					// whitehole cools down when it eats ANAR
					if (!legacy_enable)
					{
						part_add_temp(parts[ri], -(MAX_TEMP-parts[i].temp)/2);
					}
					part_kill(i);
					return MoveResult::DESTROYED;
				}
			}
			else if (rt==PT_DEUT)
			{
				if (t==PT_ELEC)
				{
					if(parts[ri].life < 6000)
						parts[ri].life += 1;
					part_set_temp(parts[ri], 0);
					part_kill(i);
					return MoveResult::DESTROYED;
				}
			}
			else if (rt==PT_VIBR || rt==PT_BVBR)
			{
				if (elements[t].Properties & TYPE_ENERGY)
				{
					parts[ri].tmp += 20;
					part_kill(i);
					return MoveResult::DESTROYED;
				}
			}
		}
		part_kill(i);
		return MoveResult::DESTROYED;
	}
	else if (moveCode==MoveResult::STORE)
	{
		// moving particle will be stored in a particle at the destination
		FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
		{
			int rt = parts[ri].type;
			if (rt==PT_PRTI)
			{
				PortalChannel *channel = elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(parts[ri]);
				SimPosDI diff = srcPos-destPos;
				int slot = PRTI_ElemDataSim::GetPosIndex(diff.x, diff.y);
				if (channel->StoreParticle(this, i, slot))
					return MoveResult::STORED;
			}
			else if (rt==PT_PIPE || rt == PT_PPIP)
			{
				if (!(parts[ri].tmp&0xFF))
				{
					parts[ri].tmp =  (parts[ri].tmp&~0xFF) | t;
					parts[ri].temp = parts[i].temp;
					parts[ri].tmp2 = parts[i].life;
					parts[ri].pavg[0] = parts[i].tmp;
					parts[ri].pavg[1] = parts[i].ctype;
					part_kill(i);
					return MoveResult::STORED;
				}
			}
		}
		return MoveResult::BLOCK;
	}
	else if (moveCode == MoveResult::DISPLACE)
	{
		//trying to swap the particles
		if (t!=PT_NEUT)
		{
			FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
			{
				// Move all particles which are displaced by this particle from the destination position
				// TODO: when breaking compatibility, add "if (part_canMove(rt, x, y)==MoveResult::DISPLACE)" here
				//int rt = parts[ri].type;
				part_set_pos(ri, destPos, srcPos);
			}
		}
		else // t==PT_NEUT
		{
			int srcNeutPenetrate = -1;
			bool srcPartFound = false;

			// e=MoveResult::DISPLACE for neutron means that target material is NEUTPENETRATE, meaning it gets moved around when neutron passes
			// First, look for NEUTPENETRATE or empty space at x,y
			FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
			{
				if (elements[parts[ri].type].Properties&PROP_NEUTPENETRATE)
				{
					srcNeutPenetrate = ri;
					break;
				}
				else
				{
					srcPartFound = true;
					break;
				}
			}
			// Move NEUTPENETRATE particles at nx,ny to x,y if there's currently a NEUTPENETRATE particle or empty space at x,y
			if (srcNeutPenetrate>=0 || !srcPartFound)
			{
				FOR_SIM_PMAP_POS(this, PMapCategory::NotEnergy, destPos, ri)
				{
					int rt = parts[ri].type;
					// Move all particles which are displaced by this particle from the destination position
					if (part_canMove(rt, srcPos)==MoveResult::DISPLACE)
					{
						part_set_pos(ri, destPos, srcPos);
					}
				}
			}
			if (srcNeutPenetrate>=0 && part_canMove(parts[srcNeutPenetrate].type, destPos, true))
			{
				part_set_pos(srcNeutPenetrate, srcPos, destPos);
			}
		}
		part_set_pos(i, srcPos, destPosF);
		return moveCode;
	}

	// All MoveResult::Codes should be handled above, except negative codes (which should never be returned by part_canMove)
	// Print warning if there's one that wasn't handled above
	printf("try_move: unhandled movement result %d (t=%d from %d,%d to %f,%f)\n", moveCode, t, srcPos.x,srcPos.y, destPosF.x,destPosF.y);
	// Assume the worst and stop further movement if we have no idea what the moveCode means
	return MoveResult::DESTROYED;
}

void Simulation::interpolateMove(InterpolateMoveResult &result, bool interact, int t, SimPosF start, SimPosDF delta, bool limitDelta, float stepSize)
{
	// interpolate to see if there is anything in the way

	float deltaSize = delta.maxComponent();
	if (deltaSize < stepSize)
	{
		// too small to need interpolation
		result.clearf = start;
		result.clear = result.clearf;
		result.destf = SimPosFT(start+delta);
		result.dest = result.destf;
		result.destCode = part_canMove(t, result.dest);
	}
	else
	{
		if (deltaSize > maxVelocity && limitDelta)
		{
			delta *= maxVelocity/deltaSize;
			deltaSize = maxVelocity;
			result.limitApplied = true;
		}
		SimPosDF step = delta * stepSize/deltaSize;
		SimPosI dest, destf = start;
		MoveResult::Code destCode = MoveResult::ALLOW;
		bool closedEholeStart = walls.isClosedEHole(start);
		while (1)
		{
			deltaSize -= stepSize;
			dest = destf = pos_handleEdges(destf+step);
			if (deltaSize <= 0.0f)
			{
				// finished interpolation, nothing found
				destf = pos_handleEdges(start+delta);
				break;
			}
			destCode = part_canMove(t, dest);
			if (!MoveResult::AllowInterpolation(destCode) || walls.isClosedEHole(dest)!=closedEholeStart)
			{
				// found an obstacle
				break;
			}
			if (interact)
				walls.detect(dest);
		}
		result.clearf = SimPosFT(destf-step);
		result.clear = result.clearf;
		result.destf = SimPosFT(destf);
		result.dest = result.destf;
		result.destCode = destCode;
	}
}


//the main function for updating particles
void Simulation::UpdateParticles()
{
	int i, j, x, y, t, nx, ny, r, surround_space, s, rt, nt, nnx, nny, q, golnum, z, neighbors;
	float mv, dx, dy, nrx, nry, dp, ctemph, ctempl, gravtot, gel_scale;
	int fin_x, fin_y, clear_x, clear_y, stagnant;
	float fin_xf, fin_yf, clear_xf, clear_yf;
	float nn, ct1, ct2, swappage;
	float pt = R_TEMP;
	float c_heat = 0.0f;
	int h_count = 0;
	int lighting_ok=1;
	unsigned int elem_properties;
	float pGravX, pGravY, pGravD;
	int excessive_stacking_found = 0;
	bool transitionOccurred;

	recalc_freeParticles();
	if (!sys_pause||framerender)
	{
		walls.simBeforeUpdate();
		air.initBlockingData();
	}

	if (sys_pause && lighting_recreate>0 && elementCount[PT_LIGH])
    {
        for (i=0; i<=parts_lastActiveIndex; i++)
        {
            if (parts[i].type==PT_LIGH && parts[i].tmp2>0)
            {
                lighting_ok=0;
                break;
            }
        }
    }
	
	if (lighting_ok)
        lighting_recreate--;

    if (lighting_recreate<0)
        lighting_recreate=1;

    if (lighting_recreate>21)
        lighting_recreate=21;
	
	if (sys_pause&&!framerender)//do nothing if paused
		return;
		
	// TODO: doesn't work with linked lists pmap yet
	/*if (force_stacking_check || rng.chance<1,10>())
	{
		force_stacking_check = 0;
		excessive_stacking_found = 0;
		for (y=0; y<YRES; y++)
		{
			for (x=0; x<XRES; x++)
			{
				// Use a threshold, since some particle stacking can be normal (e.g. BIZR + FILT)
				// Setting pmap_count[y][x] > NPART means BHOL will form in that spot
				if (pmap_count[y][x]>5)
				{
					if (bmap[y/CELL][x/CELL]==WL_EHOLE)
					{
						// Allow more stacking in E-hole
						if (pmap_count[y][x]>1500)
						{
							pmap_count[y][x] = pmap_count[y][x] + NPART;
							excessive_stacking_found = 1;
						}
					}
					// Random chance to turn into BHOL that increases with the amount of stacking, up to a threshold where it is certain to turn into BHOL
					else if (pmap_count[y][x]>=1500 || rng.randInt<0,1600>()<=(pmap_count[y][x]+100))
					{
						pmap_count[y][x] = pmap_count[y][x] + NPART;
						excessive_stacking_found = 1;
					}
				}
			}
		}
		if (excessive_stacking_found)
		{
			for (i=0; i<=parts_lastActiveIndex; i++)
			{
				if (parts[i].type)
				{
					t = parts[i].type;
					x = (int)(parts[i].x+0.5f);
					y = (int)(parts[i].y+0.5f);
					if (x>=0 && y>=0 && x<XRES && y<YRES && !(elements[t].Properties&TYPE_ENERGY))
					{
						if (pmap_count[y][x]>=NPART)
						{
							if (pmap_count[y][x]>NPART)
							{
								create_part(i, x, y, PT_NBHL);
								parts[i].temp = MAX_TEMP;
								parts[i].tmp = pmap_count[y][x]-NPART;//strength of grav field
								if (parts[i].tmp>51200) parts[i].tmp = 51200;
								pmap_count[y][x] = NPART;
							}
							else
							{
								part_kill(i);
							}
						}
					}
				}
			}
		}
	}*/

	//wire!
	if (elementCount[PT_WIRE])
	{
		for (i=0; i<=parts_lastActiveIndex; i++)
		{
			if (parts[i].type==PT_WIRE)
				parts[i].tmp=parts[i].ctype;
		}
	}

	if (ppip_changed)
	{
		for (i=0; i<=parts_lastActiveIndex; i++)
		{
			if (parts[i].type==PT_PPIP)
			{
				parts[i].tmp |= (parts[i].tmp&0xE0000000)>>3;
				parts[i].tmp &= ~0xE0000000;
			}
		}
		ppip_changed = 0;
	}

	//game of life!
	if (elementCount[PT_LIFE] && ++CGOL>=GSPEED)//GSPEED is frames per generation
	{
		CGOL=0;
		//TODO: maybe this should only loop through active particles
		for (ny=CELL; ny<YRES-CELL; ny++)
		{//go through every particle and set neighbor map
			for (nx=CELL; nx<XRES-CELL; nx++)
			{
				if (!pmap[ny][nx].count(PMapCategory::NotEnergy))
				{
					gol[ny][nx] = 0;
					continue;
				}
				r = pmap_find_one(nx, ny, PT_LIFE);
				if (r>=0)
				{
					golnum = parts[r].ctype+1;
					if (golnum<=0 || golnum>NGOL) {
						part_kill(r);
						continue;
					}
					gol[ny][nx] = golnum;
					if (parts[r].tmp == grule[golnum][9]-1) {
						for ( nnx=-1; nnx<2; nnx++)
						{
							for ( nny=-1; nny<2; nny++)//it will count itself as its own neighbor, which is needed, but will have 1 extra for delete check
							{
								int adx = ((nx+nnx+XRES-3*CELL)%(XRES-2*CELL))+CELL;
								int ady = ((ny+nny+YRES-3*CELL)%(YRES-2*CELL))+CELL;
								if (!pmap[ady][adx].count(PMapCategory::NotEnergy) || pmap_find_one(adx, ady, PT_LIFE)>=0)
								{
									//the total neighbor count is in 0
									gol2[ady][adx][0] ++;
									//insert golnum into neighbor table
									for ( i=1; i<9; i++)
									{
										if (!gol2[ady][adx][i])
										{
											gol2[ady][adx][i] = (golnum<<4)+1;
											break;
										}
										else if((gol2[ady][adx][i]>>4)==golnum)
										{
											gol2[ady][adx][i]++;
											break;
										}
									}
								}
							}
						}
					} else {
						parts[r].tmp --;
					}
				}
			}
		}
		for (ny=CELL; ny<YRES-CELL; ny++)
		{ //go through every particle again, but check neighbor map, then update particles
			for (nx=CELL; nx<XRES-CELL; nx++)
			{
				r = pmap_find_one(nx, ny, PT_LIFE);
				if (pmap[ny][nx].count(PMapCategory::NotEnergy) && r<0)
					continue;
				neighbors = gol2[ny][nx][0];
				if (neighbors)
				{
					golnum = gol[ny][nx];
					if (!pmap[ny][nx].count(PMapCategory::NotEnergy))
					{
						//Find which type we can try and create
						int creategol = 0xFF;
						for ( i=1; i<9; i++)
						{
							if (!gol2[ny][nx][i]) break;
							golnum = (gol2[ny][nx][i]>>4);
							if (grule[golnum][neighbors]>=2 && (gol2[ny][nx][i]&0xF)>=(neighbors%2)+neighbors/2)
							{
								if (golnum<creategol) creategol=golnum;
							}
						}
						if (creategol<0xFF)
							create_part(-1, nx, ny, PT_LIFE|((creategol-1)<<8));
					}
					else if (grule[golnum][neighbors-1]==0 || grule[golnum][neighbors-1]==2)//subtract 1 because it counted itself
					{
						if (parts[r].tmp==grule[golnum][9]-1)
							parts[r].tmp --;
					}
					for ( z = 0; z<9; z++)
						gol2[ny][nx][z] = 0;
				}
				//we still need to kill things with 0 neighbors (higher state life)
				if (r>=0 && parts[r].tmp<=0)
						part_kill(r);
			}
		}
	}
	hook_beforeUpdate.Trigger();
	for (i=0; i<=parts_lastActiveIndex; i++)
		if (parts[i].type)
		{
			t = parts[i].type;
#ifdef OGLR
			parts[i].lastX = parts[i].x;
			parts[i].lastY = parts[i].y;
#endif
			if (t<0 || t>=PT_NUM)
			{
				part_kill(i);
				continue;
			}
			elem_properties = elements[t].Properties;
			if (parts[i].life>0 && (elem_properties&PROP_LIFE_DEC))
			{
				// automatically decrease life
				parts[i].life--;
				if (parts[i].life<=0 && (elem_properties&(PROP_LIFE_KILL_DEC|PROP_LIFE_KILL)))
				{
					// kill on change to no life
					part_kill(i);
					continue;
				}
			}
			else if (parts[i].life<=0 && (elem_properties&PROP_LIFE_KILL))
			{
				// kill if no life
				part_kill(i);
				continue;
			}
		}
	//the main particle loop function, goes over all particles.
	for (i=0; i<=parts_lastActiveIndex; i++)
		if (parts[i].type)
		{
			t = parts[i].type;
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);

			SimPosCell cellPos = SimPosI(x,y);

			//this kills any particle out of the screen, or in a wall where it isn't supposed to go
			if (!pos_inMainArea(cellPos) || (walls.type(cellPos) && isWallDeadly(cellPos,t)))
			{
				part_kill(i);
				continue;
			}
			walls.detect(cellPos);

			//adding to velocity from the particle's velocity
			air.vx.multiply(cellPos, elements[t].AirLoss);
			air.vy.multiply(cellPos, elements[t].AirLoss);
			air.vx.add(cellPos, elements[t].AirDrag*parts[i].vx);
			air.vy.add(cellPos, elements[t].AirDrag*parts[i].vy);

			if (elements[t].PressureAdd_NoAmbHeat)
			{
				if (t==PT_GAS||t==PT_NBLE)
				{
					if (air.pv.get(cellPos)<3.5f)
						air.pv.blend_legacy(cellPos, 3.5f, elements[t].PressureAdd_NoAmbHeat);
					if (y+CELL<YRES)
					{
						SimPosCell c = SimPosCell(cellPos.x,cellPos.y+1);
						if (air.pv.get(c)<3.5f)
							air.pv.blend_legacy(c, 3.5f, elements[t].PressureAdd_NoAmbHeat);
					}
					if (x+CELL<XRES)
					{
						SimPosCell c = SimPosCell(cellPos.x+1,cellPos.y);
						if (air.pv.get(c)<3.5f)
							air.pv.blend_legacy(c, 3.5f, elements[t].PressureAdd_NoAmbHeat);
						if (y+CELL<YRES)
						{
							SimPosCell c = SimPosCell(cellPos.x+1,cellPos.y+1);
							if (air.pv.get(c)<3.5f)
								air.pv.blend_legacy(c, 3.5f, elements[t].PressureAdd_NoAmbHeat);
						}
					}
				}
				else//add the hotair variable to the pressure map, like black hole, or white hole.
				{
					air.pv.add(cellPos, elements[t].PressureAdd_NoAmbHeat);
					if (y+CELL<YRES)
						air.pv.add(SimPosCell(cellPos.x,cellPos.y+1), elements[t].PressureAdd_NoAmbHeat);
					if (x+CELL<XRES)
					{
						air.pv.add(SimPosCell(cellPos.x+1,cellPos.y), elements[t].PressureAdd_NoAmbHeat);
						if (y+CELL<YRES)
							air.pv.add(SimPosCell(cellPos.x+1,cellPos.y+1), elements[t].PressureAdd_NoAmbHeat);
					}
				}
			}

			if (elements[t].Gravity || !(elements[t].Properties & TYPE_SOLID))
			{
				//Gravity mode by Moach
				switch (gravityMode)
				{
				default:
				case 0:
					pGravX = 0.0f;
					pGravY = elements[t].Gravity;
					break;
				case 1:
					pGravX = pGravY = 0.0f;
					break;
				case 2:
					pGravD = 0.01f - hypotf((x - XCNTR), (y - YCNTR));
					pGravX = elements[t].Gravity * ((float)(x - XCNTR) / pGravD);
					pGravY = elements[t].Gravity * ((float)(y - YCNTR) / pGravD);
				}
				//Get some gravity from the gravity map
				if (t==PT_ANAR)
				{
					// perhaps we should have a elements variable for this
					pGravX -= gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
					pGravY -= gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
				}
				else if(t!=PT_STKM && t!=PT_STKM2 && t!=PT_FIGH && !(elements[t].Properties & TYPE_SOLID))
				{
					pGravX += gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
					pGravY += gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
				}
			}
			else
				pGravX = pGravY = 0;
			//velocity updates for the particle
			if (!(parts[i].flags&FLAG_MOVABLE))
			{
				parts[i].vx *= elements[t].Loss;
				parts[i].vy *= elements[t].Loss;
			}
			//particle gets velocity from the vx and vy maps
			parts[i].vx += elements[t].Advection*air.vx.get(cellPos) + pGravX;
			parts[i].vy += elements[t].Advection*air.vy.get(cellPos) + pGravY;


			if (elements[t].Diffusion)//the random diffusion that gases have
			{
#ifdef REALISTIC
				//The magic number controlls diffusion speed
				parts[i].vx += 0.05f*sqrtf(parts[i].temp)*elements[t].Diffusion*rng.randFloat(-1.0f,1.0f);
				parts[i].vy += 0.05f*sqrtf(parts[i].temp)*elements[t].Diffusion*rng.randFloat(-1.0f,1.0f);
#else
				parts[i].vx += elements[t].Diffusion*rng.randFloat(-1.0f,1.0f);
				parts[i].vy += elements[t].Diffusion*rng.randFloat(-1.0f,1.0f);
#endif
			}

			gel_scale = 1.0f;
			if (t==PT_GEL)
				gel_scale = parts[i].tmp*2.55f;

			if (!legacy_enable)
			{
				if (y-2 >= 0 && y-2 < YRES && (elements[t].Properties&TYPE_LIQUID) && (t!=PT_GEL || rng.chance(parts[i].tmp,100))) {//some heat convection for liquids
					int swapIndex = pmap_find_one(x,y-2,t);
					if (swapIndex>=0) {
						if (parts[i].temp>parts[swapIndex].temp) {
							swappage = parts[i].temp;
							parts[i].temp = parts[swapIndex].temp;
							parts[swapIndex].temp = swappage;
						}
					}
				}
			}

			transitionOccurred = false;

			j = surround_space = nt = 0;//if nt is greater than 1 after this, then there is a particle around the current particle, that is NOT the current particle's type, for water movement.

			for (nx=-1; nx<2; nx++)
				for (ny=-1; ny<2; ny++) {
					if (nx||ny) {
						if (!pmap[y+ny][x+nx].count(PMapCategory::NotEnergy))
							surround_space++;//there is empty space
						if (pmap_find_one(x+nx,y+ny,t)<0)
							nt++;//there is nothing or a different particle
					}
				}

			if (!legacy_enable)
			{
				//heat transfer code
				//this is probably a bit slower now, with the removal of the fixed size surround_hconduct array
				if (t&&(t!=PT_HSWC||parts[i].life==10) && rng.chance(elements[t].HeatConduct*gel_scale,250))
				{
					if (ambientHeatEnabled && !(elements[t].Properties&PROP_NOAMBHEAT))
					{
						c_heat = (air.hv.get(cellPos)-parts[i].temp)*0.04;
						c_heat = tptmath::clamp_flt(c_heat, -TEMP_RANGE, TEMP_RANGE);
						part_add_temp(parts[i], c_heat);
						air.hv.add(cellPos, -c_heat);
					}

					h_count = 0;
					c_heat = 0.0f;
					int rcount, ri, rnext, rx, ry;
					for (rx=-1; rx<2; rx++)
					{
						for (ry=-1; ry<2; ry++)
						{
							FOR_PMAP_POSITION_NOENERGY(this, x+rx, y+ry, rcount, ri, rnext)
							{
								rt = parts[ri].type;
								// ri!=i instead of just using all particles found because this loop excludes energy particles so might not include particle i. Particle i included below in pt calculation. 
								if (ri!=i&&elements[rt].HeatConduct&&(rt!=PT_HSWC||parts[ri].life==10)
									&&(t!=PT_FILT||(rt!=PT_BRAY&&rt!=PT_BIZR&&rt!=PT_BIZRG))
									&&(rt!=PT_FILT||(t!=PT_BRAY&&t!=PT_PHOT&&t!=PT_BIZR&&t!=PT_BIZRG))
									&&(t!=PT_ELEC||rt!=PT_DEUT)
									&&(t!=PT_DEUT||rt!=PT_ELEC))
								{
									c_heat += parts[ri].temp;
									h_count++;
								}
							}
						}
					}
					pt = (c_heat+parts[i].temp)/(h_count+1);
					pt = parts[i].temp = tptmath::clamp_flt(pt, MIN_TEMP, MAX_TEMP);
					for (rx=-1; rx<2; rx++)
					{
						for (ry=-1; ry<2; ry++)
						{
							FOR_PMAP_POSITION_NOENERGY(this, x+rx, y+ry, rcount, ri, rnext)
							{
								rt = parts[ri].type;
								if (elements[rt].HeatConduct&&(rt!=PT_HSWC||parts[ri].life==10)
									&&(t!=PT_FILT||(rt!=PT_BRAY&&rt!=PT_BIZR&&rt!=PT_BIZRG))
									&&(rt!=PT_FILT||(t!=PT_BRAY&&t!=PT_PHOT&&t!=PT_BIZR&&t!=PT_BIZRG))
									&&(t!=PT_ELEC||rt!=PT_DEUT)
									&&(t!=PT_DEUT||rt!=PT_ELEC))
								{
									parts[ri].temp = pt;
								}
							}
						}
					}

					ctemph = ctempl = pt;
					// change boiling point with pressure
					if ((elements[t].State==ST_LIQUID && elements[t].HighTemperatureTransitionElement>-1 && elements[t].HighTemperatureTransitionElement<PT_NUM
							&& elements[elements[t].HighTemperatureTransitionElement].State==ST_GAS)
					        || t==PT_LNTG || t==PT_SLTW)
						ctemph -= 2.0f*air.pv.get(cellPos);
					else if ((elements[t].State==ST_GAS && elements[t].LowTemperatureTransitionElement>-1 && elements[t].LowTemperatureTransitionElement<PT_NUM
							 && elements[elements[t].LowTemperatureTransitionElement].State==ST_LIQUID)
					         || t==PT_WTRV)
						ctempl -= 2.0f*air.pv.get(cellPos);
					s = 1;

					//A fix for ice with ctype = 0
					if ((t==PT_ICEI || t==PT_SNOW) && (!parts[i].ctype || !IsValidElement(parts[i].ctype) || parts[i].ctype==PT_ICEI || parts[i].ctype==PT_SNOW))
						parts[i].ctype = PT_WATR;

					if (ctemph>=elements[t].HighTemperatureTransitionThreshold && elements[t].HighTemperatureTransitionElement>-1) {
						// particle type change due to high temperature

						if (elements[t].HighTemperatureTransitionElement!=PT_NUM)
							t = elements[t].HighTemperatureTransitionElement;
						else if (t==PT_ICEI || t==PT_SNOW) {
							if (IsValidElement(parts[i].ctype) && parts[i].ctype!=t) {
								if (elements[parts[i].ctype].LowTemperatureTransitionElement==PT_ICEI || elements[parts[i].ctype].LowTemperatureTransitionElement==PT_SNOW)
								{
									if (pt<elements[parts[i].ctype].LowTemperatureTransitionThreshold)
										s = 0;
								}
								else if (pt<273.15f)
 									s = 0;

								if (s)
								{
									t = parts[i].ctype;
									parts[i].ctype = PT_NONE;
									parts[i].life = 0;
								}
							}
							else s = 0;
						}
						else if (t==PT_SLTW) {
							if (rng.chance<1,4>()) t = PT_SALT;
							else t = PT_WTRV;
						}
						else if (t == PT_BRMT)
						{
							if (parts[i].ctype == PT_TUNG)
							{
								if (ctemph < elements[parts[i].ctype].HighTemperatureTransitionThreshold)
									s = 0;
								else
								{
									t = PT_LAVA;
									parts[i].ctype = PT_TUNG;
								}
							}
							else
								t = PT_LAVA;
						}
						else s = 0;
					} else if (ctempl<elements[t].LowTemperatureTransitionThreshold && elements[t].LowTemperatureTransitionElement>-1) {
						// particle type change due to low temperature
						if (elements[t].LowTemperatureTransitionElement!=PT_NUM)
							t = elements[t].LowTemperatureTransitionElement;
						else if (t==PT_WTRV) {
							if (pt<273.0f) t = PT_RIME;
							else t = PT_DSTW;
						}
						else if (t==PT_LAVA) {
							if (IsValidElement(parts[i].ctype) && parts[i].ctype!=PT_LAVA) {
								if (parts[i].ctype==PT_THRM && pt>=elements[PT_BMTL].HighTemperatureTransitionThreshold) s = 0;
								else if ((parts[i].ctype==PT_VIBR || parts[i].ctype==PT_BVBR) && pt>=273.15f) s = 0;
								else if (parts[i].ctype==PT_TUNG) {
									// TUNG does its own melting in its update function, so HighTemperatureTransition is not LAVA so it won't be handled by the code for HighTemperatureTransition==PT_LAVA below
									// However, the threshold is stored in HighTemperature to allow it to be changed from Lua
									if (pt>=elements[parts[i].ctype].HighTemperatureTransitionThreshold)
										s = 0;
								}
								else if (elements[parts[i].ctype].HighTemperatureTransitionElement==PT_LAVA) {
									if (pt>=elements[parts[i].ctype].HighTemperatureTransitionThreshold) s = 0;
								}
								else if (pt>=973.0f) s = 0; // freezing point for lava with any other (not set to turn into lava at high temperature) ctype
								if (s) {
									t = parts[i].ctype;
									parts[i].ctype = PT_NONE;
									if (t==PT_THRM) {
										parts[i].tmp = 0;
										t = PT_BMTL;
									}
									if (t==PT_PLUT)
									{
										parts[i].tmp = 0;
										t = PT_LAVA;
									}
								}
							}
							else if (pt<973.0f) t = PT_STNE;
							else s = 0;
						}
						else s = 0;
					}
					else s = 0;
					if (s) { // particle type change occurred
						if (t==PT_LAVA && parts[i].type==PT_BRMT && parts[i].ctype==PT_TUNG)
						{}// ctype already set correctly
						else if (t==PT_ICEI||t==PT_LAVA||t==PT_SNOW)
							parts[i].ctype = parts[i].type;
						if (!(t==PT_ICEI&&parts[i].ctype==PT_FRZW)) parts[i].life = 0;
						if (elements[t].State==ST_GAS&&elements[parts[i].type].State!=ST_GAS)
							air.pv.add(cellPos, 0.50f);
						if (t==PT_NONE)
						{
							part_kill(i);
							goto killed;
						}
						part_change_type(i,x,y,t);
						if (t==PT_FIRE)
						{
							parts[i].tmp = 0;// if tmp isn't 0 the FIRE might turn into DSTW later
						}
						if (t==PT_FIRE||t==PT_PLSM||t==PT_HFLM)
						{
							parts[i].life = rng.randInt<120,120+49>();
						}
						if (t==PT_LAVA) {
							if (parts[i].ctype==PT_BRMT) parts[i].ctype = PT_BMTL;
							else if (parts[i].ctype==PT_SAND) parts[i].ctype = PT_GLAS;
							else if (parts[i].ctype==PT_BGLA) parts[i].ctype = PT_GLAS;
							else if (parts[i].ctype==PT_PQRT) parts[i].ctype = PT_QRTZ;
							parts[i].life = rng.randInt<240,240+119>();
						}
						transitionOccurred = true;
					}

					pt = parts[i].temp = tptmath::clamp_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
					if (t==PT_LAVA) {
						parts[i].life = tptmath::clamp_flt((parts[i].temp-700)/7, 0, 400);
						if (parts[i].ctype==PT_THRM&&parts[i].tmp>0)
						{
							parts[i].tmp--;
							parts[i].temp = 3500;
						}
						if (parts[i].ctype==PT_PLUT&&parts[i].tmp>0)
						{
							parts[i].tmp--;
							parts[i].temp = MAX_TEMP;
						}
					}
				}
				else
				{
					air.blockh_inc(cellPos);
					parts[i].temp = tptmath::clamp_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
				}
			}

			if (t==PT_LIFE)
			{
				part_add_temp(parts[i], -50.0f);
			}
			//spark updates from walls
			if ((elements[t].Properties&PROP_CONDUCTS) || t==PT_SPRK)
			{
				nx = x % CELL;
				if (nx == 0)
					nx = x/CELL - 1;
				else if (nx == CELL-1)
					nx = x/CELL + 1;
				else
					nx = x/CELL;
				ny = y % CELL;
				if (ny == 0)
					ny = y/CELL - 1;
				else if (ny == CELL-1)
					ny = y/CELL + 1;
				else
					ny = y/CELL;
				SimPosCell c(nx, ny);
				if (pos_isValid(c))
				{
					if (t!=PT_SPRK)
					{
						if (walls.electricity(c)==12 && !parts[i].life)
						{
							if (spark_particle_conductiveOnly(i, x, y))
								t = PT_SPRK;
						}
					}
					else if (walls.isConductive(c))
						walls.makeSpark(c);
				}
			}

			//the basic explosion, from the .explosive variable
			if ((elements[t].Explosive&2) && air.pv.get(cellPos)>2.5f)
			{
				parts[i].life = rng.randInt<180,180+79>();
				// TODO: add to existing temp instead of setting temp? Might break compatibility.
				part_set_temp(parts[i], elements[PT_FIRE].DefaultProperties.temp + (elements[t].Flammable/2));
				t = PT_FIRE;
				part_change_type(i,x,y,t);
				air.pv.add(cellPos, 0.25f * CFDS);
			}


			s = 1;
			gravtot = fabs(gravy[(y/CELL)*(XRES/CELL)+(x/CELL)])+fabs(gravx[(y/CELL)*(XRES/CELL)+(x/CELL)]);
			if (air.pv.get(cellPos)>elements[t].HighPressureTransitionThreshold && elements[t].HighPressureTransitionElement>-1) {
				// particle type change due to high pressure
				if (elements[t].HighPressureTransitionElement!=PT_NUM)
					t = elements[t].HighPressureTransitionElement;
				else if (t==PT_BMTL) {
					if (air.pv.get(cellPos)>2.5f)
						t = PT_BRMT;
					else if (air.pv.get(cellPos)>1.0f && parts[i].tmp==1)
						t = PT_BRMT;
					else s = 0;
				}
				else s = 0;
			} else if (air.pv.get(cellPos)<elements[t].LowPressureTransitionThreshold && elements[t].LowPressureTransitionElement>-1) {
				// particle type change due to low pressure
				if (elements[t].LowPressureTransitionElement!=PT_NUM)
					t = elements[t].LowPressureTransitionElement;
				else s = 0;
			} else if (gravtot>(elements[t].HighPressureTransitionThreshold/4.0f) && elements[t].HighPressureTransitionElement>-1) {
				// particle type change due to high gravity
				if (elements[t].HighPressureTransitionElement!=PT_NUM)
					t = elements[t].HighPressureTransitionElement;
				else if (t==PT_BMTL) {
					if (gravtot>0.625f)
						t = PT_BRMT;
					else if (gravtot>0.25f && parts[i].tmp==1)
						t = PT_BRMT;
					else s = 0;
				}
				else s = 0;
			} else s = 0;
			if (s) { // particle type change occurred
				parts[i].life = 0;
				if (t==PT_NONE)
				{
					part_kill(i);
					goto killed;
				}
				part_change_type(i,x,y,t);
				if (t==PT_FIRE)
					parts[i].life = rng.randInt<120,120+49>();
				transitionOccurred = true;
			}

			//call the particle update function, if there is one
#ifdef LUACONSOLE
			if (elements[t].Update && lua_el_mode[t] != 2)
#else
			if (elements[t].Update)
#endif
			{
				if ((*(elements[t].Update))(this, i,x,y,surround_space,nt,parts))
					continue;
				else if (t==PT_WARP)
				{
					// Warp does some movement in its update func, update variables to avoid incorrect data in pmap
					x = (int)(parts[i].x+0.5f);
					y = (int)(parts[i].y+0.5f);
				}
			}
#ifdef LUACONSOLE
			if(lua_el_mode[t])
			{
				if(luacon_part_update(t,i,x,y,surround_space,nt))
					continue;
				// Need to update variables, in case they've been changed by Lua
				x = (int)(parts[i].x+0.5f);
				y = (int)(parts[i].y+0.5f);
			}
#endif
			if (legacy_enable)//if heat sim is off
				ElementsShared_noHeatSim::update(this, i,x,y,surround_space,nt,parts);

killed:
			if (parts[i].type == PT_NONE)//if its dead, skip to next particle
				continue;

			if (transitionOccurred)
				continue;

			if (!parts[i].vx&&!parts[i].vy)//if its not moving, skip to next particle, movement code is next
				continue;

			InterpolateMoveResult imove;
			interpolateMove(imove, true, parts[i]);
			if (imove.limitApplied)
			{
				float deltaSize = std::max(std::abs(parts[i].vx), std::abs(parts[i].vy));
				parts[i].vx *= maxVelocity/deltaSize;
				parts[i].vy *= maxVelocity/deltaSize;
			}

			fin_x = imove.dest.x, fin_y = imove.dest.y;
			clear_x = imove.clear.x, clear_y = imove.clear.y;
			fin_xf = imove.destf.x, fin_yf = imove.destf.y;
			clear_xf = imove.clearf.x, clear_yf = imove.clearf.y;

			stagnant = parts[i].flags & FLAG_STAGNANT;
			parts[i].flags &= ~FLAG_STAGNANT;

			if (t==PT_STKM || t==PT_STKM2 || t==PT_FIGH)
			{
				//head movement, let head pass through anything
				if (edgeMode != 2)
				{
					part_set_pos(i, x, y, parts[i].x+parts[i].vx, parts[i].y+parts[i].vy);
				}
				else
				{
					int nx = (int)((float)parts[i].x+parts[i].vx+0.5f);
					int ny = (int)((float)parts[i].y+parts[i].vy+0.5f);
					int diffx = 0, diffy = 0;
					if (nx < CELL)
						diffx = XRES-CELL*2;
					if (nx >= XRES-CELL)
						diffx = -(XRES-CELL*2);
					if (ny < CELL)
						diffy = YRES-CELL*2;
					if (ny >= YRES-CELL)
						diffy = -(YRES-CELL*2);
					if (diffx || diffy) //when moving from left to right stickmen might be able to fall through solid things, fix with "part_canMove(t, nx+diffx, ny+diffy)" but then they die instead
					{
						//adjust stickmen legs
						Stickman_data* playerp = Stickman_data::get(this, parts[i]);
						if (playerp)
							for (int i = 0; i < 16; i+=2)
							{
								playerp->legs[i] += diffx;
								playerp->legs[i+1] += diffy;
							}
					}
					part_set_pos(i, x, y, parts[i].x+parts[i].vx+diffx, parts[i].y+parts[i].vy+diffy);
				}
				continue;
			}
			else if (elements[t].Properties & TYPE_ENERGY)
			{
				if (t == PT_PHOT)
				{
					// refraction and total internal reflection

					if (parts[i].flags&FLAG_SKIPMOVE)
					{
						parts[i].flags &= ~FLAG_SKIPMOVE;
						continue;
					}

					int ri = pmap_find_one(fin_x, fin_y, PT_GLAS);
					int li = pmap_find_one(x, y, PT_GLAS);

					if (MoveResult::WillSucceed(part_canMove(PT_PHOT, fin_x, fin_y)) && ((ri>=0 && li<0) || (ri<0 && li>=0))) {
						if (!get_normal_interp(REFRACT|t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy, &nrx, &nry)) {
							part_kill(i);
							continue;
						}

						r = Element_PHOT::get_wavelength_bin(this, &parts[i].ctype);
						if (r == -1) {
							part_kill(i);
							continue;
						}
						nn = GLASS_IOR - GLASS_DISP*(r-15)/15.0f;
						nn *= nn;
						nrx = -nrx;
						nry = -nry;
						if (ri>=0 && li<0) //if entering glass
							nn = 1.0f/nn;
						ct1 = parts[i].vx*nrx + parts[i].vy*nry;
						ct2 = 1.0f - (nn*nn)*(1.0f-(ct1*ct1));
						if (ct2 < 0.0f) {
							// total internal reflection
							parts[i].vx -= 2.0f*ct1*nrx;
							parts[i].vy -= 2.0f*ct1*nry;
							fin_xf = parts[i].x;
							fin_yf = parts[i].y;
							fin_x = x;
							fin_y = y;
						} else {
							// refraction
							ct2 = sqrtf(ct2);
							ct2 = ct2 - nn*ct1;
							parts[i].vx = nn*parts[i].vx + ct2*nrx;
							parts[i].vy = nn*parts[i].vy + ct2*nry;
						}
					}
				}
				if (stagnant)//FLAG_STAGNANT set, was reflected on previous frame
				{
					// cast coords as int then back to float for compatibility with existing saves. TODO: remove when breaking compatibility
					MoveResult::Code moveResult = part_move(i, x, y, (float)fin_x, (float)fin_y);
					if (MoveResult::WasBlocked(moveResult))
					{
						part_kill(i);
						continue;
					}
					else if (MoveResult::WasKilled(moveResult))
					{
						continue;
					}
				}
				else if (MoveResult::WasBlocked(part_move(i, x, y, fin_xf, fin_yf)))
				{
					// reflection
					parts[i].flags |= FLAG_STAGNANT;
					if (t==PT_NEUT && rng.chance<1,10>())
					{
						part_kill(i);
						continue;
					}

					if (get_normal_interp(t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy, &nrx, &nry)) {
						dp = nrx*parts[i].vx + nry*parts[i].vy;
						parts[i].vx -= 2.0f*dp*nrx;
						parts[i].vy -= 2.0f*dp*nry;
						// leave the actual movement until next frame so that reflection of fast particles and refraction happen correctly
					} else {
						if (t!=PT_NEUT)
							part_kill(i);
						continue;
					}
					if (!(parts[i].ctype&0x3FFFFFFF) && t == PT_PHOT) {
						part_kill(i);
						continue;
					}
				}
				goto movedone;
			}
			else if (elements[t].Falldown==0)
			{
				// gases and solids (but not powders)
				if (MoveResult::WasBlocked(part_move(i, x, y, fin_xf, fin_yf)))
				{
					// can't move there, so bounce off
					// TODO
					if (fin_x>x+ISTP) fin_x=x+ISTP;
					if (fin_x<x-ISTP) fin_x=x-ISTP;
					if (fin_y>y+ISTP) fin_y=y+ISTP;
					if (fin_y<y-ISTP) fin_y=y-ISTP;
					if (MoveResult::Succeeded(part_move(i, x, y, 0.25f+(float)(2*x-fin_x), 0.25f+fin_y)))
					{
						parts[i].vx *= elements[t].Collision;
					}
					else if (MoveResult::Succeeded(part_move(i, x, y, 0.25f+fin_x, 0.25f+(float)(2*y-fin_y))))
					{
						parts[i].vy *= elements[t].Collision;
					}
					else
					{
						parts[i].vx *= elements[t].Collision;
						parts[i].vy *= elements[t].Collision;
					}
				}
				goto movedone;
			}
			else
			{
				if (water_equal_test && elements[t].Falldown == 2 && rng.chance<1,400>())//checking stagnant is cool, but then it doesn't update when you change it later.
				{
					if (!flood_water(x,y,i,y, parts[i].flags&FLAG_WATEREQUAL))
						goto movedone;
				}
				// liquids and powders
				// First try to move in the direction of the particle velocity
				if (MoveResult::WasBlocked(part_move(i, x, y, fin_xf, fin_yf)))
				{
					MoveResult::Code moveResult;
					// Now try moving a little less in the direction of the particle velocity
					if (fin_x!=x && MoveResult::Succeeded_MaybeKilled(moveResult=part_move(i, x, y, fin_xf, clear_yf)))
					{
						if (!MoveResult::WasKilled(moveResult))
						{
							parts[i].vx *= elements[t].Collision;
							parts[i].vy *= elements[t].Collision;
						}
					}
					else if (fin_y!=y && MoveResult::Succeeded_MaybeKilled(moveResult=part_move(i, x, y, clear_xf, fin_yf)))
					{
						if (!MoveResult::WasKilled(moveResult))
						{
							parts[i].vx *= elements[t].Collision;
							parts[i].vy *= elements[t].Collision;
						}
					}
					else
					{
						// Movement in velocity direction is blocked, try moving diagonally and (for liquids) horizontally
						s = 1;
						r = rng.randInt<0,1>()*2 - 1;// position search direction (left/right first)
						if ((clear_x!=x || clear_y!=y || nt || surround_space) &&
							(fabsf(parts[i].vx)>0.01f || fabsf(parts[i].vy)>0.01f))
						{
							// allow diagonal movement if target position is blocked
							// but no point trying this if particle is stuck in a block of identical particles
							dx = parts[i].vx - parts[i].vy*r;
							dy = parts[i].vy + parts[i].vx*r;
							if (fabsf(dy)>fabsf(dx))
								mv = fabsf(dy);
							else
								mv = fabsf(dx);
							dx /= mv;
							dy /= mv;
							if (MoveResult::Succeeded_MaybeKilled(moveResult=part_move(i, x, y, clear_xf+dx, clear_yf+dy)))
							{
								if (!MoveResult::WasKilled(moveResult))
								{
									parts[i].vx *= elements[t].Collision;
									parts[i].vy *= elements[t].Collision;
								}
								goto movedone;
							}
							if (MoveResult::Succeeded_MaybeKilled(moveResult=part_move(i, x, y, clear_xf+dy*r, clear_yf-dx*r)))// perpendicular to previous vector
							{
								if (!MoveResult::WasKilled(moveResult))
								{
									parts[i].vx *= elements[t].Collision;
									parts[i].vy *= elements[t].Collision;
								}
								goto movedone;
							}
						}
						if (elements[t].Falldown>1 && !ngrav_enable && gravityMode==0 && parts[i].vy>fabsf(parts[i].vx))
						{
							moveResult = MoveResult::BLOCK;
							// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
							if (!stagnant || nt) //nt is if there is an something else besides the current particle type, around the particle
								rt = 30;//slight less water lag, although it changes how it moves a lot
							else
								rt = 10;

							if (t==PT_GEL)
								rt = parts[i].tmp*0.20f+5.0f;

							for (j=clear_x+r; j>=0 && j>=clear_x-rt && j<clear_x+rt && j<XRES; j+=r)
							{
								if (pmap_find_one(j, fin_y, t)<0 || walls.type(SimPosI(j,fin_y)))
								{
									moveResult = part_move(i, x, y, (float)j, fin_yf);
									if (MoveResult::Succeeded_MaybeKilled(moveResult))
									{
										if (!MoveResult::WasKilled(moveResult))
										{
											nx = (int)(parts[i].x+0.5f);
											ny = (int)(parts[i].y+0.5f);
										}
										break;
									}
								}
								if (fin_y!=clear_y && (pmap_find_one(j, clear_y, t)<0 || walls.type(SimPosI(j,clear_y))))
								{
									moveResult = part_move(i, x, y, (float)j, clear_yf);
									if (MoveResult::Succeeded_MaybeKilled(moveResult))
									{
										if (!MoveResult::WasKilled(moveResult))
										{
											nx = (int)(parts[i].x+0.5f);
											ny = (int)(parts[i].y+0.5f);
										}
										break;
									}
								}
								if (pmap_find_one(j, clear_y, t)<0 || walls.isProperWall(SimPosI(j,clear_y)))
									break;
							}
							if (MoveResult::WasKilled(moveResult))
								continue;
							if (MoveResult::Succeeded(moveResult))
							{
								if (parts[i].vy>0)
									r = 1;
								else
									r = -1;
								parts[i].vx *= elements[t].Collision;
								parts[i].vy *= elements[t].Collision;
								for (j=ny+r; j>=0 && j<YRES && j>=ny-rt && j<ny+rt; j+=r)
								{
									bool tmp = (pmap_find_one(nx, j, t)<0);// true if no particles of the same type are at nx,j
									if ((tmp || walls.type(SimPosI(nx,j))) && MoveResult::Succeeded_MaybeKilled(part_move(i, nx, ny, (float)nx, (float)j)))
										break;
									if (tmp || walls.isProperWall(SimPosI(nx,j)))
										break;
								}
							}
							else
							{
								parts[i].vx *= elements[t].Collision;
								parts[i].vy *= elements[t].Collision;
								if ((clear_x!=x||clear_y!=y) && part_move(i, x, y, clear_xf, clear_yf)!=0) {}
								else parts[i].flags |= FLAG_STAGNANT;
							}
						}
						// fabsf stuff here is checking whether the component of the velocity parallel to the gravity direction is greater than the perpendicular component, indicating the particle is fairly stationary, with the velocity being due mainly to the acceleration by gravity
						else if (elements[t].Falldown>1 && fabsf(pGravX*parts[i].vx+pGravY*parts[i].vy)>fabsf(pGravY*parts[i].vx-pGravX*parts[i].vy))
						{
							parts[i].vx *= elements[t].Collision;
							parts[i].vy *= elements[t].Collision;
							float nxf, nyf, pGravX, pGravY, prev_pGravX, prev_pGravY, ptGrav = elements[t].Gravity;
							moveResult = MoveResult::BLOCK;
							// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
							if (!stagnant || nt) //nt is if there is an something else besides the current particle type, around the particle
								rt = 30;//slight less water lag, although it changes how it moves a lot
							else
								rt = 10;
							// clear_xf, clear_yf is the last known position that the particle should almost certainly be able to move to
							nxf = clear_xf;
							nyf = clear_yf;
							nx = clear_x;
							ny = clear_y;
							// Look for spaces to move horizontally (perpendicular to gravity direction), keep going until a space is found or the number of positions examined = rt
							for (j=0;j<rt;j++)
							{
								GetGravityAccel(nx,ny, ptGrav, 1.0f, pGravX, pGravY);
								// Scale gravity vector so that the largest component is 1 pixel
								if (fabsf(pGravY)>fabsf(pGravX))
									mv = fabsf(pGravY);
								else
									mv = fabsf(pGravX);
								if (mv<0.0001f) break;
								pGravX /= mv;
								pGravY /= mv;
								// Move 1 pixel perpendicularly to gravity
								// r is +1/-1, to try moving left or right at random
								if (j)
								{
									// Not quite the gravity direction
									// Gravity direction + last change in gravity direction
									// This makes liquid movement a bit less frothy, particularly for balls of liquid in radial gravity. With radial gravity, instead of just moving along a tangent, the attempted movement will follow the curvature a bit better.
									nxf += r*(pGravY*2.0f-prev_pGravY);
									nyf += -r*(pGravX*2.0f-prev_pGravX);
								}
								else
								{
									nxf += r*pGravY;
									nyf += -r*pGravX;
								}
								prev_pGravX = pGravX;
								prev_pGravY = pGravY;
								nx = (int)(nxf+0.5f);
								ny = (int)(nyf+0.5f);
								// Check whether movement is allowed
								if (!InBounds(nx,ny))
									break;
								if (pmap_find_one(nx,ny,t)<0 || walls.type(SimPosI(nx,ny)))
								{
									moveResult = part_move(i, x, y, nxf, nyf);
									if (MoveResult::Succeeded_MaybeKilled(moveResult))
									{
										if (!MoveResult::WasKilled(moveResult))
										{
											nx = (int)(parts[i].x+0.5f);
											ny = (int)(parts[i].y+0.5f);
										}
										break;
									}
									// A particle of a different type, or a wall, was found. Stop trying to move any further horizontally unless the wall should be completely invisible to particles.
									if (walls.isProperWall(SimPosI(nx,ny)) || pmap_differentElemExists(SimPosI(nx,ny), t, PMapCategory::NotEnergy))
										break;
								}
							}
							if (MoveResult::WasKilled(moveResult))
								continue;
							if (MoveResult::Succeeded(moveResult))
							{
								// The particle managed to move horizontally, now try to move vertically (parallel to gravity direction)
								// Keep going until the particle is blocked (by something that isn't the same element) or the number of positions examined = rt
								clear_x = nx;
								clear_y = ny;
								for (j=0;j<rt;j++)
								{
									// Calculate overall gravity direction
									GetGravityAccel(nx,ny, ptGrav, 1.0f, pGravX, pGravY);
									// Scale gravity vector so that the largest component is 1 pixel
									if (fabsf(pGravY)>fabsf(pGravX))
										mv = fabsf(pGravY);
									else
										mv = fabsf(pGravX);
									if (mv<0.0001f) break;
									pGravX /= mv;
									pGravY /= mv;
									// Move 1 pixel in the direction of gravity
									nxf += pGravX;
									nyf += pGravY;
									nx = (int)(nxf+0.5f);
									ny = (int)(nyf+0.5f);
									if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
										break;
									// If the space is anything except the same element (so is a wall, empty space, or occupied by a particle of a different element), try to move into it
									if (pmap_find_one(nx,ny,t)<0 || walls.type(SimPosI(nx,ny)))
									{
										moveResult = part_move(i, clear_x, clear_y, nxf, nyf);
										if (MoveResult::Succeeded(moveResult) || walls.isProperWall(SimPosI(nx,ny)) || pmap_differentElemExists(SimPosI(nx,ny), t, PMapCategory::NotEnergy))
											break;// found the edge of the liquid and movement into it succeeded, so stop moving down
									}
								}
							}
							else if ((clear_x!=x||clear_y!=y) && part_move(i, x, y, clear_xf, clear_yf)) {} // try moving to the last clear position
							else parts[i].flags |= FLAG_STAGNANT;
						}
						else
						{
							// if interpolation was done, try moving to last clear position
							if ((clear_x!=x||clear_y!=y) && part_move(i, x, y, clear_xf, clear_yf)!=0) {}
							else parts[i].flags |= FLAG_STAGNANT;
							parts[i].vx *= elements[t].Collision;
							parts[i].vy *= elements[t].Collision;
						}
					}
				}
			}
movedone:
			continue;
		}

	hook_afterUpdate.Trigger();
}



void Simulation::option_edgeMode(short newMode)
{
	edgeMode = newMode;
	switch(edgeMode)
	{
	case 0:
	case 2:
		walls.setBorder(WL_NONE);
		break;
	case 1:
		walls.setBorder(WL_WALL);
		break;
	default:
		option_edgeMode(0);
		break;
	}
}


#include "simulation/Element_UI.h"

// TODO: remove these
AirData undo_airData;
WallsData undo_wallsData;

void Simulation_Compat_CopyData(Simulation* sim)
{
	// TODO: this can be removed once all the code uses Simulation instead of global variables
	globalSim = sim;
	parts = sim->parts;
	for (int i=0; quickmenu[i].icon!=nullptr; i++)
	{
		if (quickmenu[i].name==std::string("Ambient heat"))
			quickmenu[i].variable = &sim->ambientHeatEnabled;
	}
}


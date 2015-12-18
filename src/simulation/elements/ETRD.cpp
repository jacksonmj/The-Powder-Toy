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

#include "simulation/ElementsCommon.h"
#include "ETRD.hpp"

#include <algorithm>

ETRD_ElemDataShared::ETRD_ElemDataShared(SimulationSharedData *s, int t) : ElemDataShared(s,t)
{
	// TODO: this value needs tuning:
	initDeltaPos(12);
}

void ETRD_ElemDataShared::initDeltaPos(int maxLength_)
{
	maxLength = maxLength_;
	deltaPos.clear();
	for (int ry=-maxLength; ry<=maxLength; ry++)
		for (int rx=-maxLength; rx<=maxLength; rx++)
		{
			SimPosDI d(rx, ry);
			if (d.length_1()<=maxLength)
				deltaPos.push_back({d, d.length_1()});
		}
	auto compareFunc = [](const ETRD_deltaWithLength &a, const ETRD_deltaWithLength &b)
	{
		return a.length < b.length;
	};
	std::stable_sort(deltaPos.begin(), deltaPos.end(), compareFunc);
}


ETRD_ElemDataSim::ETRD_ElemDataSim(Simulation *s, int t) :
	ElemDataSim(s,t),
	obs_simCleared(sim->hook_cleared, this, &ETRD_ElemDataSim::invalidate),
	obs_simBeforeUpdate(sim->hook_beforeUpdate, this, &ETRD_ElemDataSim::invalidate),
	obs_simAfterUpdate(sim->hook_afterUpdate, this, &ETRD_ElemDataSim::invalidate)
{
	invalidate();
}

void ETRD_ElemDataSim::invalidate()
{
	isValid = false;
	countLife0 = 0;
}

int Element_ETRD::nearestSparkablePart(Simulation *sim, int targetId)
{
	/* Using SimPosF and length_sq() would be slightly faster, since no conversions from float to int are needed, and all arithmetic is add/sub/mul with no branches/conditional moves. However, this can't currently be changed due to compatibility. */

	if (!sim->elementCount[PT_ETRD])
		return -1;
	ETRD_ElemDataSim *ed = sim->elemData<ETRD_ElemDataSim>(PT_ETRD);
	ETRD_ElemDataShared *eds = sim->elemDataShared<ETRD_ElemDataShared>(PT_ETRD);
	if (ed->isValid && ed->countLife0<=0)
		return -1;

	particle *parts = sim->parts;
	int foundDistance = SimPosDI(XRES,YRES).length_1();
	int foundI = -1;
	SimPosI targetPos = SimPosF(parts[targetId].x, parts[targetId].y);

	if (ed->isValid)
	{
		// countLife0 doesn't need recalculating, so just focus on finding the nearest particle

		// If the simulation contains lots of particles, check near the target position first since going through all particles will be slow.
		// Threshold = number of positions checked, *2 because it's likely to access memory all over the place (less cache friendly) and there's extra logic needed
		// TODO: probably not optimal if excessive stacking is used
		if (sim->parts_lastActiveIndex > (int)eds->deltaPos.size()*2)
		{
			for (ETRD_deltaWithLength delta : eds->deltaPos)
			{
				SimPosI checkPos = targetPos + delta.d;
				int checkDistance = delta.length;
				if (foundDistance<checkDistance)
				{
					// eds->deltaPos is sorted in order of ascending length, so foundDistance<checkDistance means all later items are further away.
					break;
				}
				if (sim->pos_isValid(checkPos) && checkDistance<=foundDistance)
				{
					FOR_SIM_PMAP_POS(sim, PMapCategory::NotEnergy, checkPos, i)
					{
						// "i<foundI || foundI<0" to match the results from an all-particles loop, which selects the particle with the lowest ID at that distance
						if (parts[i].type==PT_ETRD && !parts[i].life && i!=targetId && (checkDistance<foundDistance || i<foundI || foundI<0))
						{
							foundDistance = checkDistance;
							foundI = i;
						}
					}
				}
			}
		}
		// If neighbour search didn't find a suitable particle, search all particles
		if (foundI<0)
		{
			for (int i=0; i<=sim->parts_lastActiveIndex; i++)
			{
				if (parts[i].type==PT_ETRD && !parts[i].life)
				{
					SimPosI checkPos = SimPosF(parts[i].x, parts[i].y);
					int checkDistance = (checkPos-targetPos).length_1();
					if (checkDistance<foundDistance && i!=targetId)
					{
						foundDistance = checkDistance;
						foundI = i;
					}
				}
			}
		}
	}
	else
	{
		// Recalculate countLife0, and search for the closest suitable particle
		int countLife0 = 0;
		for (int i=0; i<=sim->parts_lastActiveIndex; i++)
		{
			if (parts[i].type==PT_ETRD && !parts[i].life)
			{
				countLife0++;
				SimPosI checkPos = SimPosF(parts[i].x, parts[i].y);
				int checkDistance = (checkPos-targetPos).length_1();
				if (checkDistance<foundDistance && i!=targetId)
				{
					foundDistance = checkDistance;
					foundI = i;
				}
			}
		}
		ed->countLife0 = countLife0;
		ed->isValid = true;
	}
	return foundI;
}

void ETRD_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	// NB: for ETRD countLife0 tracking to work, life value must be set to the new value before calling part_change_type with new type==ETRD

	ETRD_ElemDataSim *eds = sim->elemData<ETRD_ElemDataSim>(PT_ETRD);
	if (eds->isValid)
	{
		if (from==PT_ETRD && sim->parts[i].life==0)
			eds->countLife0--;
		if (to==PT_ETRD && sim->parts[i].life==0)
		{
			eds->countLife0++;
		}
	}
}

void ETRD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_ETRD";
	elem->ui->Name = "ETRD";
	elem->Colour = COLPACK(0x404040);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Electrode. Creates a surface that allows Plasma arcs. (Use sparingly)";

	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = NULL;
	elem->Func_ChangeType = &ETRD_ChangeType;
	simSD->elemData_create<ETRD_ElemDataShared>(t);
	elem->Func_SimInit = &SimInit_createElemData<ETRD_ElemDataSim>;
}


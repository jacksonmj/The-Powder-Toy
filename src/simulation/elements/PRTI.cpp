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
#include "simulation/elements/PRTI.h"
#include "simulation/elements/SOAP.h"
#include "simulation/elements/STKM.h"

#include <iostream>

/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
const int portal_rx[8] = {-1, 0, 1, 1, 1, 0,-1,-1};
const int portal_ry[8] = {-1,-1,-1, 0, 1, 1, 1, 0};

PRTI_ElemDataSim::PRTI_ElemDataSim(Simulation *s, int t, float chanStep, int chanCount)
	: ElemDataSim_channels(s,t),
	  obs_simCleared(sim->hook_cleared, this, &PRTI_ElemDataSim::ClearPortalContents),
	  channelStep(chanStep),
	  channelCount(chanCount)
{
	if (channelStep<1)
		channelStep = 1;
	if (channelCount<0)//default value=-1 : set channelCount automatically based on channelStep
		channelCount = 1 + int((MAX_TEMP-73.15f)/channelStep+1) - int((MIN_TEMP-73.15f)/channelStep+1);

	channels = new PortalChannel[channelCount];
	ClearPortalContents();
}

PRTI_ElemDataSim::~PRTI_ElemDataSim()
{
	delete[] channels;
}

void PRTI_ElemDataSim::ClearPortalContents()
{
	for (int i=0; i<channelCount; i++)
		channels[i].ClearContents();
}

bool PRTI_ElemDataSim::Check()
{
	bool ok = true;
	for (int i=0; i<channelCount; i++)
	{
		if (!channels[i].Check())
		{
			std::cout << "Check failed for portal channel " << i << std::endl;
			ok = false;
		}
	}
	return ok;
}

void PortalChannel::ClearContents()
{
	memset(portalp, 0, sizeof(portalp));
	memset(particleCount, 0, sizeof(particleCount));
}

particle * PortalChannel::AllocParticle(int slot)
{
	if (particleCount[slot]>=storageSize)
		return NULL;
	for (int nnx=0; nnx<storageSize; nnx++)
		if (!portalp[slot][nnx].type)
		{
			particleCount[slot]++;
			return &(portalp[slot][nnx]);
		}
	return NULL;
}

// Store a particle in a given slot (one of the 8 neighbour positions) for this portal channel, then kills the original
// Does not check whether the particle should be in a portal
// Returns a pointer to the particle on success, or NULL if the portal is full
particle * PortalChannel::StoreParticle(Simulation *sim, int store_i, int slot)
{
	if (particleCount[slot]>=storageSize)
		return NULL;
	Stickman_data *stkdata = NULL;
	if (sim->parts[store_i].type==PT_STKM || sim->parts[store_i].type==PT_STKM2 || sim->parts[store_i].type==PT_FIGH)
	{
		stkdata = Stickman_data::get(sim, sim->parts[store_i]);
		if (stkdata && stkdata->part != &sim->parts[store_i])
			return NULL;
	}
	for (int nnx=0; nnx<storageSize; nnx++)
		if (!portalp[slot][nnx].type)
		{
			portalp[slot][nnx] = sim->parts[store_i];
			particleCount[slot]++;
			if (sim->parts[store_i].type==PT_SOAP)
				SOAP_detach(sim, store_i);

			if (sim->parts[store_i].type==PT_SPRK)
				sim->part_change_type(store_i,(int)(sim->parts[store_i].x+0.5f),(int)(sim->parts[store_i].y+0.5f),sim->parts[store_i].ctype);
			else if (stkdata)
			{
				sim->elemData<STK_common_ElemDataSim>(sim->parts[store_i].type)->storageActionPending = true;
				sim->part_kill(store_i);
				stkdata->set_particle(&portalp[slot][nnx]);
			}
			else
			{
				sim->part_kill(store_i);
			}
			return &portalp[slot][nnx];
		}
	return NULL;
}

bool PortalChannel::Check()
{
	bool ok = true;
	for (int slot=0; slot<8; slot++)
	{
		int count = 0;
		for (int i=0; i<storageSize; i++)
		{
			if (portalp[slot][i].type)
			{
				count++;
			}
		}
		if (count!=particleCount[slot])
		{
			std::cout << "Portal particle count incorrect for slot " << slot << " (count " << particleCount[slot] << " actual " << count << ")" << std::endl;
			ok = false;
		}
	}
	return ok;
}

int PRTI_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elemData(PT_PRTI))
		return 0;
	int r, rx, ry, rt, fe = 0;
	int rcount, ri, rnext;
	int count =0;
	PortalChannel *channel = sim->elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(sim->parts[i]);
	for (count=0; count<8; count++)
	{
		if (channel->particleCount[count] >= channel->storageSize)
			continue;
		rx = portal_rx[count];
		ry = portal_ry[count];
		if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			if (!sim->pmap[y+ry][x+rx].count(PMapCategory::NotEnergy)) fe = 1;
			FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
			{
				rt = parts[ri].type;
				if (rt==PT_PRTI || rt==PT_PRTO || (!(sim->elements[rt].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)) && rt!=PT_SPRK))
					continue;

				if (channel->StoreParticle(sim, ri, count))
					fe = 1;
			}
		}
	}


	if (fe) {
		int orbd[4] = {0, 0, 0, 0};	//Orbital distances
		int orbl[4] = {0, 0, 0, 0};	//Orbital locations
		if (!parts[i].life) parts[i].life = sim->rng.randUint32();
		if (!parts[i].ctype) parts[i].ctype = sim->rng.randUint32();
		Element_PRTI::orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
		for (r = 0; r < 4; r++) {
			if (orbd[r]>12){
				// effect pixels move inwards while rotating slightly
				orbd[r] -= 12;
				orbl[r] = (orbl[r]+2)%256;
			} else {
				orbd[r] = sim->rng.randInt<128,255>();
				orbl[r] = sim->rng.randInt<0,255>();
			}
		}
		Element_PRTI::orbitalparts_set(&parts[i].life, &parts[i].ctype, orbd, orbl);
	} else {
		parts[i].life = 0;
		parts[i].ctype = 0;
	}
	return 0;
}

int PRTI_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 8;
	*firer = 255;
	*fireg = 0;
	*fireb = 0;
	*pixel_mode |= EFFECT_GRAVIN;
	*pixel_mode |= EFFECT_LINES;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_ADD;
	return 1;
}

void PRTI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_PRTI";
	elem->ui->Name = "PRTI";
	elem->Colour = COLPACK(0xEB5917);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = -0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Portal IN. Particles go in here. Also has temperature dependent channels. (same as WIFI)";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PRTI_update;
	elem->Graphics = &PRTI_graphics;
	elem->Func_SimInit = &SimInit_createElemData<PRTI_ElemDataSim>;
}


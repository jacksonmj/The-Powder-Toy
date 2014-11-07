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
#include "simulation/elements/FIGH.h"

int STKM_graphics(GRAPHICS_FUNC_ARGS);
void STKM_init_legs(playerst* playerp, int i);

int FIGH_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp<0)
	{
		sim->part_kill(i);
		return 1;
	}

	playerst* figh = ((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Get(parts[i].tmp);

	unsigned int tarx, tary;

	parts[i].tmp2 = 0; //0 - stay in place, 1 - seek a stick man

	//Set target coords
	if (player2.spwn)
	{
			if (player.spwn && (pow(player.legs[2]-x, 2) + pow(player.legs[3]-y, 2))<=
					(pow(player2.legs[2]-x, 2) + pow(player2.legs[3]-y, 2)))
			{
				tarx = (unsigned int)player.legs[2];
				tary = (unsigned int)player.legs[3];
			}
			else
			{
				tarx = (unsigned int)player2.legs[2];
				tary = (unsigned int)player2.legs[3];
			}
			parts[i].tmp2 = 1;
	}
	else if (player.spwn)
	{
		tarx = (unsigned int)player.legs[2];
		tary = (unsigned int)player.legs[3];
		parts[i].tmp2 = 1;
	}

	switch (parts[i].tmp2)
	{
		case 1:
			if ((pow(tarx-x, 2) + pow(tary-y, 2))<600)
			{
				if (figh->elem == PT_LIGH || figh->elem == PT_NEUT 
						|| ptypes[figh->elem].properties&(PROP_DEADLY|PROP_RADIOACTIVE) 
						|| ptypes[figh->elem].heat>=323 || ptypes[figh->elem].heat<=243)
					figh->comm = (int)figh->comm | 0x08;
			}
			else if (tarx<x)
			{
				if(figh->rocketBoots || !(eval_move(PT_FIGH, figh->legs[4]-10, figh->legs[5]+6, NULL) 
				  && eval_move(PT_FIGH, figh->legs[4]-10, figh->legs[5]+3, NULL)))
					figh->comm = 0x01;
				else
					figh->comm = 0x02;

				if (figh->rocketBoots)
				{
					if (tary<y)
						figh->comm = (int)figh->comm | 0x04;
				}
				else if (!eval_move(PT_FIGH, figh->legs[4]-4, figh->legs[5]-1, NULL) 
						|| !eval_move(PT_FIGH, figh->legs[12]-4, figh->legs[13]-1, NULL)
						|| eval_move(PT_FIGH, 2*figh->legs[4]-figh->legs[6], figh->legs[5]+5, NULL))
					figh->comm = (int)figh->comm | 0x04;
			}
			else
			{ 
				if (figh->rocketBoots || !(eval_move(PT_FIGH, figh->legs[12]+10, figh->legs[13]+6, NULL)
				   && eval_move(PT_FIGH, figh->legs[12]+10, figh->legs[13]+3, NULL)))
					figh->comm = 0x02;
				else
					figh->comm = 0x01;

				if (figh->rocketBoots)
				{
					if (tary<y)
						figh->comm = (int)figh->comm | 0x04;
				}
				else if (!eval_move(PT_FIGH, figh->legs[4]+4, figh->legs[5]-1, NULL) 
						|| !eval_move(PT_FIGH, figh->legs[4]+4, figh->legs[5]-1, NULL)
						|| eval_move(PT_FIGH, 2*figh->legs[12]-figh->legs[14], figh->legs[13]+5, NULL))
					figh->comm = (int)figh->comm | 0x04;
			}
			break;
		default:
			figh->comm = 0;
			break;
	}

	figh->pcomm = figh->comm;

	run_stickman(figh, UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

bool FIGH_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return ((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->CanAlloc();
}

void FIGH_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (to==PT_FIGH)
	{
		sim->parts[i].tmp = ((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Alloc();
		if (sim->parts[i].tmp>=0)
		{
			playerst* figh = ((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Get(sim->parts[i].tmp);
			figh->spwn = 1;
			figh->elem = PT_DUST;
			figh->rocketBoots = false;
			STKM_init_legs(figh, i);
		}
	}
	else
	{
		((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Free(sim->parts[i].tmp);
	}
}

void FIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FIGH";
	elem->Name = "FIGH";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP+14.6f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Fighter. Tries to kill stickmen. You must first give it an element to kill him with.";

	elem->State = ST_NONE;
	elem->Properties = 0;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.life = 100;

	elem->Update = &FIGH_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Allowed = &FIGH_create_allowed;
	elem->Func_ChangeType = &FIGH_ChangeType;

	if (sim->elementData[t])
	{
		delete sim->elementData[t];
	}
	sim->elementData[t] = new FIGH_ElementDataContainer;
}


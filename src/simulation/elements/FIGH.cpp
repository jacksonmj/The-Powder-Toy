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

int FIGH_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elemData(PT_FIGH))
		return 0;
	Stickman_data *figh = sim->elemData<FIGH_ElemDataSim>(PT_FIGH)->GetFighterData(parts[i]);

	if (!figh)
	{
		sim->part_kill(i);
		return 1;
	}

	Stickman_data &player = sim->elemData<STKM_ElemDataSim>(PT_STKM)->player;
	Stickman_data &player2 = sim->elemData<STKM_ElemDataSim>(PT_STKM2)->player;
	int tarx, tary;

	parts[i].tmp2 = 0; //0 - stay in place, 1 - seek a stick man

	//Set target coords
	if (player2.isOnscreen())
	{
			if (player.isOnscreen() && (pow(player.legs[2]-x, 2) + pow(player.legs[3]-y, 2))<=
					(pow(player2.legs[2]-x, 2) + pow(player2.legs[3]-y, 2)))
			{
				tarx = (int)player.legs[2];
				tary = (int)player.legs[3];
			}
			else
			{
				tarx = (int)player2.legs[2];
				tary = (int)player2.legs[3];
			}
			parts[i].tmp2 = 1;
	}
	else if (player.isOnscreen())
	{
		tarx = (int)player.legs[2];
		tary = (int)player.legs[3];
		parts[i].tmp2 = 1;
	}

	switch (parts[i].tmp2)
	{
		case 1:
			if ((pow(tarx-x, 2) + pow(tary-y, 2))<600)
			{
				if (figh->elem == PT_LIGH || figh->elem == PT_NEUT 
						|| sim->elements[figh->elem].Properties&(PROP_DEADLY|PROP_RADIOACTIVE)
						|| sim->elements[figh->elem].DefaultProperties.temp>=323 || sim->elements[figh->elem].DefaultProperties.temp<=243)
					figh->commandOn(STKM_commands::Action);
			}
			else if (tarx<x)
			{
				if(figh->rocketBoots || MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[4]-10, figh->legs[5]+6)) || MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[4]-10, figh->legs[5]+3)))
					figh->comm = STKM_commands::Left;
				else
					figh->comm = STKM_commands::Right;

				if (figh->rocketBoots)
				{
					if (tary<y)
						figh->commandOn(STKM_commands::Jump);
				}
				else if (MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[4]-4, figh->legs[5]-1))
						|| MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[12]-4, figh->legs[13]-1))
						|| !MoveResult::WillBlock(sim->part_canMove(PT_FIGH, 2*figh->legs[4]-figh->legs[6], figh->legs[5]+5)))
					figh->commandOn(STKM_commands::Jump);
			}
			else
			{ 
				if (figh->rocketBoots || MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[12]+10, figh->legs[13]+6)) || MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[12]+10, figh->legs[13]+3)))
					figh->comm = STKM_commands::Right;
				else
					figh->comm = STKM_commands::Left;

				if (figh->rocketBoots)
				{
					if (tary<y)
						figh->commandOn(STKM_commands::Jump);
				}
				else if (MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[4]+4, figh->legs[5]-1))
						|| MoveResult::WillBlock(sim->part_canMove(PT_FIGH, figh->legs[4]+4, figh->legs[5]-1))
						|| !MoveResult::WillBlock(sim->part_canMove(PT_FIGH, 2*figh->legs[12]-figh->legs[14], figh->legs[13]+5)))
					figh->commandOn(STKM_commands::Jump);
			}
			break;
		default:
			figh->comm = 0;
			break;
	}

	figh->pcomm = figh->comm;

	return run_stickman(figh, UPDATE_FUNC_SUBCALL_ARGS);
}

bool FIGH_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return sim->elemData<FIGH_ElemDataSim>(PT_FIGH)->create_allowed();
}

void FIGH_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	FIGH_ElemDataSim *fighdata = sim->elemData<FIGH_ElemDataSim>(PT_FIGH);
	if (to==PT_FIGH)
	{
		fighdata->on_part_create(sim->parts[i]);
	}
	else
	{
		fighdata->on_part_kill(sim->parts[i]);
	}
}

void FIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_FIGH";
	elem->ui->Name = "FIGH";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SPECIAL;
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
	elem->ui->Description = "Fighter. Tries to kill stickmen. You must first give it an element to kill him with.";

	elem->Properties = PROP_NOCTYPEDRAW;

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
	elem->Func_SimInit = &SimInit_createElemData<FIGH_ElemDataSim>;
}


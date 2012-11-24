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

int STKM_graphics(GRAPHICS_FUNC_ARGS);
void STKM_init_legs(playerst* playerp, int i);

int STKM2_update(UPDATE_FUNC_ARGS)
{
	run_stickman(&player2, UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

int STKM2_create_override(ELEMENT_CREATE_OVERRIDE_FUNC_ARGS)
{
	int i;

	if (player2.spwn)
		return -1;

	if (p==-1)
	{
		if (pmap[y][x] ? (eval_move(t, x, y, NULL)!=2) : (bmap[y/CELL][x/CELL] && eval_move(t, x, y, NULL)==0))
		{
			if ((pmap[y][x]&0xFF)!=PT_SPAWN&&(pmap[y][x]&0xFF)!=PT_SPAWN2)
			{
				return -1;
			}
		}
		i = sim->part_alloc();
	}
	else if (p<0)
	{
		i = sim->part_alloc();
	}
	else
	{
		int oldX = (int)(parts[p].x+0.5f);
		int oldY = (int)(parts[p].y+0.5f);
		sim->pmap_remove(p, oldX, oldY);
		i = p;
	}

	if (i<0)
		return -1;

	sim->parts[i] = sim->elements[t].DefaultProperties;
	sim->parts[i].type = t;
	sim->parts[i].x = (float)x;
	sim->parts[i].y = (float)y;
#ifdef OGLR
	sim->parts[i].lastX = (float)x;
	sim->parts[i].lastY = (float)y;
#endif
	parts[i].life = 100;
	parts[i].temp = sim->elements[t].CreationTemperature;
	STKM_init_legs(&player2, i);
	player2.spwn = 1;

	create_part(-3, x, y, PT_SPAWN2);

	sim->elementCount[t]++;
	sim->pmap_add(i, x, y, t);
	return i;
}

void STKM2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STKM2";
	elem->Name = "STK2";
	elem->Colour = COLPACK(0x6464FF);
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

	elem->CreationTemperature = R_TEMP+14.6f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Second Stickman. Don't kill him!";

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

	elem->Update = &STKM2_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Override = &STKM2_create_override;
}


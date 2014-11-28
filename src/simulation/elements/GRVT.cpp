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

int GRVT_update(UPDATE_FUNC_ARGS)
{
	//at higher tmps they just go completely insane
	if (parts[i].tmp >= 100)
		parts[i].tmp = 100;
	if (parts[i].tmp <= -100)
		parts[i].tmp = -100;

	gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = 0.2f*parts[i].tmp;
	return 0;
}

void GRVT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = (rand()%360)*3.14159f/180.0f;
	parts[i].life = 250 + rand()%200;
	parts[i].vx = 2.0f*cosf(a);
	parts[i].vy = 2.0f*sinf(a);
}

int GRVT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 5;
	*firer = 0;
	*fireg = 250;
	*fireb = 170;

	*pixel_mode |= FIRE_BLEND;
	return 1;
}

void GRVT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GRVT";
	elem->Name = "GRVT";
	elem->Colour = COLPACK(0x00EE76);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+273.15f;
	elem->HeatConduct = 61;
	elem->Latent = 0;
	elem->Description = "Gravitons. Create Newtonian Gravity.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = 7;

	elem->Update = &GRVT_update;
	elem->Graphics = &GRVT_graphics;
	elem->Func_Create = &GRVT_create;
}


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

int VINE_update(UPDATE_FUNC_ARGS)
{
	int r, np, rx =(rand()%3)-1, ry=(rand()%3)-1;
	if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
	{
		r = pmap[y+ry][x+rx];
		if (1>rand()%15)
			part_change_type(i,x,y,PT_PLNT);
		else if (!r)
		{
			np = sim->part_create(-1,x+rx,y+ry,PT_VINE);
			if (np<0) return 0;
			parts[np].temp = parts[i].temp;
			parts[i].tmp = 1;
			part_change_type(i,x,y,PT_PLNT);
		}
	}
	return 0;
}

void VINE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VINE";
	elem->Name = "VINE";
	elem->Colour = COLPACK(0x079A00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 10;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 65;
	elem->Latent = 0;
	elem->Description = "Vine, grows";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 573.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &VINE_update;
	elem->Graphics = NULL;
}


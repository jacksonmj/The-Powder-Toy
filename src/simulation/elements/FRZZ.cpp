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

int FRZZ_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)==PT_WATR&&5>rand()%100)
				{
					part_change_type(r>>8,x+rx,y+ry,PT_FRZW);
					parts[r>>8].life = 100;
					parts[i].type = PT_NONE;
				}

			}
	if (parts[i].type==PT_NONE) {
		kill_part(i);
		return 1;
	}
	return 0;
}

void FRZZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRZZ";
	elem->Name = "FRZZ";
	elem->Colour = COLPACK(0xC0E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.90f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.05f;
	elem->Diffusion = 0.01f;
	elem->PressureAdd_NoAmbHeat = -0.00005f* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 50;

	elem->CreationTemperature = 253.15f;
	elem->HeatConduct = 46;
	elem->Latent = 0;
	elem->Description = "Freeze powder. When melted, forms ice that always cools. Spreads with regular water.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 1.8f;
	elem->HighPressureTransitionElement = PT_SNOW;
	elem->LowTemperatureTransitionThreshold = 50.0f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 273.15f;
	elem->HighTemperatureTransitionElement = PT_WATR;

	elem->Update = &FRZZ_update;
	elem->Graphics = NULL;
}


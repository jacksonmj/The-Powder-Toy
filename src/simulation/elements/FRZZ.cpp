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
	int rx, ry;
	int rcount, ri, rnext;
	bool killPart = false;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type==PT_WATR && !(rand()%20))
					{
						part_change_type(ri,x+rx,y+ry,PT_FRZW);
						parts[ri].life = 100;
						killPart = true;
					}
				}
			}
	if (killPart) {
		kill_part(i);
		return 1;
	}
	return 0;
}

void FRZZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_FRZZ";
	elem->ui->Name = "FRZZ";
	elem->Colour = COLPACK(0xC0E0FF);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWDERS;
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

	elem->DefaultProperties.temp = 253.15f;
	elem->HeatConduct = 46;
	elem->Latent = 0;
	elem->ui->Description = "Freeze powder. When melted, forms ice that always cools. Spreads with regular water.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 1.8f;
	elem->HighPressureTransitionElement = PT_SNOW;
	elem->LowTemperatureTransitionThreshold = 50.0f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 273.15f;
	elem->HighTemperatureTransitionElement = PT_FRZW;

	elem->Update = &FRZZ_update;
	elem->Graphics = NULL;
}


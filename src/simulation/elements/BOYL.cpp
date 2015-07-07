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

int BOYL_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	for (ry=-1; ry<2; ry++)
	{
		for (rx=-1; rx<2; rx++)
		{
			SimPosCell c(x/CELL+rx,y/CELL+ry);
			if (sim->pos_isValid(c))
				sim->air.pv.blend_legacy(c, (parts[i].temp/100), 0.001f);
		}
	}
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 &&
			        x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int rt = parts[ri].type;
					if (rt==PT_WATR && sim->rng.chance<1,30>())
					{
						sim->part_change_type(ri,x+rx,y+ry,PT_FOG);
					}
					else if (rt==PT_O2 && sim->rng.chance<1,9>())
					{
						sim->part_kill(ri);
						sim->part_change_type(i,x,y,PT_WATR);
						sim->air.pv.add(SimPosI(x,y), 4.0f);
					}
				}
			}
	return 0;
}

void BOYL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_BOYL";
	elem->ui->Name = "BOYL";
	elem->Colour = COLPACK(0x0A3200);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.18f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+2.0f	+273.15f;
	elem->HeatConduct = 42;
	elem->Latent = 0;
	elem->ui->Description = "Boyle, variable pressure gas. Expands when heated.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BOYL_update;
	elem->Graphics = NULL;
}


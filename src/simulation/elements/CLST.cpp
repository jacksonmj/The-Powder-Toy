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

int CLST_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	float cxy = 0;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int rt = parts[ri].type;
					if (rt==PT_WATR)
					{
						if (!(rand()%1500))
						{
							part_change_type(i,x,y,PT_PSTS);
							kill_part(ri);
						}
					}
					if (rt==PT_NITR)
					{
						sim->part_create(i, x, y, PT_BANG);
						sim->part_create(ri, x+rx, y+ry, PT_BANG);
					}
					if (rt==PT_CLST)
					{
						if(parts[i].temp <195)
							cxy = 0.05;
						else if(parts[i].temp <295)
							cxy = 0.015;
						else if(parts[i].temp <350)
							cxy = 0.01;
						else
							cxy = 0.005;
						parts[i].vx += cxy*rx;
						parts[i].vy += cxy*ry;//These two can be set not to calculate over 350 later. They do virtually nothing over 0.005.
					}
				}
			}
	return 0;
}

int CLST_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = cpart->tmp - 5;//speckles!
	*colr += z * 16;
	*colg += z * 16;
	*colb += z * 16;
	return 0;
}

void CLST_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = (rand()%7);
}

void CLST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_CLST";
	elem->ui->Name = "CLST";
	elem->Colour = COLPACK(0xE4A4A4);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.2f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 55;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->ui->Description = "Clay dust. Produces paste when mixed with water.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1256.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &CLST_update;
	elem->Graphics = &CLST_graphics;
	elem->Func_Create = &CLST_create;
}


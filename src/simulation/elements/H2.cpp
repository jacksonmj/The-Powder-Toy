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

int H2_update(UPDATE_FUNC_ARGS)
{
	int rx,ry,rt;
	int rcount, ri, rnext;
	bool typeChanged = false;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (pv[y/CELL][x/CELL] > 8.0f && rt == PT_DESL) // This will not work well. DESL turns to fire above 5.0 pressure
					{
						part_change_type(ri,x+rx,y+ry,PT_WATR);
						part_change_type(i,x,y,PT_OIL);
						typeChanged = true;
					}
					if (pv[y/CELL][x/CELL] > 45.0f)
					{
						if (parts[ri].temp > 2273.15)
							continue;
					}
					else
					{
						if (rt==PT_FIRE)
						{
							if(parts[ri].tmp&0x02)
								parts[ri].temp=3473;
							else
								parts[ri].temp=2473.15f;
							parts[ri].tmp |= 1;
							sim->part_create(i,x,y,PT_FIRE);
							typeChanged = true;
							parts[i].temp+=(rand()%100);
							parts[i].tmp |= 1;
						}
						else if ((rt==PT_PLSM && !(parts[ri].tmp&4)) || (rt==PT_LAVA && parts[ri].ctype != PT_BMTL))
						{
							sim->part_create(i,x,y,PT_FIRE);
							typeChanged = true;
							parts[i].temp+=(rand()%100);
							parts[i].tmp |= 1;
						}
					}
				}
			}
	if (parts[i].temp > 2273.15 && pv[y/CELL][x/CELL] > 50.0f)
	{
		if (!(rand()%5))
		{
			int j;
			float temp = parts[i].temp;
			sim->part_create(i,x,y,PT_NBLE);
			typeChanged = true;

			j = sim->part_create(-3,x+rand()%3-1,y+rand()%3-1,PT_NEUT);
			if (j>=0)
				parts[j].temp = temp;
			if (!(rand()%10))
			{
				j = sim->part_create(-3,x+rand()%3-1,y+rand()%3-1,PT_ELEC);
				if (j>=0)
					parts[j].temp = temp;
			}
			j = sim->part_create(-3,x+rand()%3-1,y+rand()%3-1,PT_PHOT);
			if (j>=0)
			{
				parts[j].ctype = 0x7C0000;
				parts[j].temp = temp;
			}

			j = sim->part_create(-3,x+rand()%3-1,y+rand()%3-1,PT_PLSM);
			if (j>=0)
			{
				parts[j].temp = temp;
				parts[j].tmp |= 4;
			}

			parts[i].temp = temp+750+rand()%500;
			pv[y/CELL][x/CELL] += 30;
		}
	}
	if (typeChanged)
		return 1;
	return 0;
}

void H2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_H2";
	elem->Name = "HYGN";
	elem->Colour = COLPACK(0x5070FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.10f;
	elem->Gravity = 0.00f;
	elem->Diffusion = 3.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Hydrogen. Combusts with OXYG to make WATR. Undergoes fusion at high temperature and pressure";

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

	elem->Update = &H2_update;
	elem->Graphics = NULL;
}


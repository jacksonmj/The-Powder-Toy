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

int CO2_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
                if (parts[i].ctype==5 && 20>(rand()%40000))
				{
					if (create_part(-1, x+rx, y+ry, PT_WATR)>=0)
						parts[i].ctype = 0;
				}
				if ((r>>8)>=NPART || !r)
					continue;
				if ((r&0xFF)==PT_FIRE){
					kill_part(r>>8);
						if(1>(rand()%150)){
							kill_part(i);
							return 1;
						}
				}
				if (((r&0xFF)==PT_WATR || (r&0xFF)==PT_DSTW) && 1>(rand()%250))
				{
					part_change_type(r>>8, x+rx, y+ry, PT_CBNW);
					if (parts[i].ctype==5) //conserve number of water particles - ctype=5 means this CO2 hasn't released the water particle from BUBW yet
						create_part(i, x, y, PT_WATR);
					else
						kill_part(i);
				}
			}
	if (parts[i].temp > 9773.15 && pv[y/CELL][x/CELL] > 200.0f)
	{
		if (rand()%5 < 1)
		{
			int j;
			create_part(i,x,y,PT_O2);

			j = create_part(-3,x+rand()%3-1,y+rand()%3-1,PT_NEUT); if (j != -1) parts[j].temp = 15000;
			if (!(rand()%50)) { j = create_part(-3,x+rand()%3-1,y+rand()%3-1,PT_ELEC); if (j != -1) parts[j].temp = 15000; }

			parts[i].temp = 15000;
			pv[y/CELL][x/CELL] += 100;
		}
	}
	return 0;
}

void CO2_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CO2";
	elem->Name = "CO2";
	elem->Colour = COLPACK(0x666666);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 1.0f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+273.15f;
	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->Description = "Carbon Dioxide";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 194.65f;
	elem->LowTemperatureTransitionElement = PT_DRIC;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &CO2_update;
	elem->Graphics = NULL;
}


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

int BRMT_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, tempFactor;
	if (parts[i].temp > (250.0f+273.15f))
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					rt = parts[r>>8].type;
					tempFactor = 1000 - (((250.0f+273.15f)-parts[i].temp)*2);
					if(tempFactor < 2)
						tempFactor = 2;
					if ((rt==PT_BREL) && 1 > (rand()%tempFactor))
					{
						if(rand()%2)
						{
							create_part(r>>8, x+rx, y+ry, PT_THRM);
						}
						else
						{	create_part(i, x, y, PT_THRM);
						}
						return 1;
						//part_change_type(r>>8,x+rx,y+ry,PT_BMTL);
						//parts[r>>8].tmp=(parts[i].tmp<=7)?parts[i].tmp=1:parts[i].tmp-(rand()%5);//rand()/(RAND_MAX/300)+100;
					}
				}
	}
	return 0;
}

void BRMT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BRMT";
	elem->Name = "BRMT";
	elem->Colour = COLPACK(0x705060);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.3f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 2;
	elem->Hardness = 2;

	elem->Weight = 90;

	elem->CreationTemperature = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 211;
	elem->Latent = 0;
	elem->Description = "Broken metal.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &BRMT_update;
	elem->Graphics = NULL;
}


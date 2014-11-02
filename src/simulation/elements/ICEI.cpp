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

int ICE_update(UPDATE_FUNC_ARGS) //currently used for snow as well
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	if (parts[i].ctype==PT_FRZW)//get colder if it is from FRZW
	{
		parts[i].temp = restrict_flt(parts[i].temp-1.0f, MIN_TEMP, MAX_TEMP);
	}
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if ((rt==PT_SALT || rt==PT_SLTW) && parts[i].temp > ptransitions[PT_SLTW].tlv && 1>(rand()%1000))
					{
						part_change_type(i,x,y,PT_SLTW);
						part_change_type(ri,x+rx,y+ry,PT_SLTW);
					}
					if ((rt==PT_FRZZ) && (parts[i].ctype=PT_FRZW) && 1>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_ICEI);
				}
			}
	return 0;
}

void ICEI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ICEI";
	elem->Name = "ICE";
	elem->Colour = COLPACK(0xA0C0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = -0.0003f* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP-50.0f+273.15f;
	elem->HeatConduct = 46;
	elem->Latent = 1095;
	elem->Description = "Solid. Freezes water. Crushes under pressure. Cools down air.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 0.8f;
	elem->HighPressureTransitionElement = PT_SNOW;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 252.05f;
	elem->HighTemperatureTransitionElement = ST;

	elem->DefaultProperties.ctype = PT_WATR;

	elem->Update = &ICE_update;
	elem->Graphics = NULL;
}


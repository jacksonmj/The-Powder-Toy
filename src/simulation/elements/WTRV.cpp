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

int WTRV_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if ((parts[ri].type==PT_RBDM||parts[ri].type==PT_LRBD) && !legacy_enable && parts[i].temp>(273.15f+12.0f) && 1>(rand()%500))
					{
						part_change_type(i,x,y,PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
						return 1;
					}
				}
			}
	if(parts[i].temp>1273&&parts[i].ctype==PT_FIRE)
		parts[i].temp-=parts[i].temp/1000;
	return 0;
}

void WTRV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WTRV";
	elem->Name = "WTRV";
	elem->Colour = COLPACK(0xA0A0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = -0.1f;
	elem->Diffusion = 0.75f;
	elem->PressureAdd_NoAmbHeat = 0.0003f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 4;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+100.0f+273.15f;
	elem->HeatConduct = 48;
	elem->Latent = 0;
	elem->Description = "Steam, heats up air, produced from hot water.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 371.0f;
	elem->LowTemperatureTransitionElement = ST;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WTRV_update;
	elem->Graphics = NULL;
}


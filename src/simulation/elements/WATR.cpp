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

int WATR_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt==PT_SALT)
					{
						if (!(rand()%83))
						{
							part_change_type(i,x,y,PT_SLTW);
							// on average, convert 3 WATR to SLTW before SALT turns into SLTW
							if (!(rand()%3))
								part_change_type(ri,x+rx,y+ry,PT_SLTW);
						}
					}
					else if (rt==PT_RBDM||rt==PT_LRBD)
					{
						if ((legacy_enable||parts[i].temp>(273.15f+12.0f)) && !(rand()%166))
						{
							part_change_type(i,x,y,PT_FIRE);
							parts[i].life = 4;
							parts[i].ctype = PT_WATR;
						}
					}
					else if (rt==PT_FIRE)
					{
						kill_part(ri);
						if (parts[ri].ctype!=PT_WATR)
						{
							if(!(rand()%50)){
								kill_part(i);
								return 1;
							}
						}
					}
					else if (rt==PT_SLTW)
					{
						if (!(rand()%3333))
						{
							part_change_type(i,x,y,PT_SLTW);
						}
					}
					/*if ((r&0xFF)==PT_CNCT && 1>(rand()%500))	Concrete+Water to paste, not very popular
					{
						part_change_type(i,x,y,PT_PSTE);
						kill_part(r>>8);
					}*/
				}
			}
	return 0;
}

void WATR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WATR";
	elem->Name = "WATR";
	elem->Colour = COLPACK(0x2030D0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 7500;
	elem->Description = "Conducts electricity, freezes, and extinguishes fires.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &WATR_update;
	elem->Graphics = NULL;
}


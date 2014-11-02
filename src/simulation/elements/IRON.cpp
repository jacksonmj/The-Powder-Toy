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

int IRON_update(UPDATE_FUNC_ARGS)
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
					if (((rt == PT_SALT && 15>(rand()/(RAND_MAX/700))) ||
							(rt == PT_SLTW && 30>(rand()/(RAND_MAX/2000))) ||
							(rt == PT_WATR && 5 >(rand()/(RAND_MAX/6000))) ||
							(rt == PT_O2   && 2 >(rand()/(RAND_MAX/500))) ||
							(rt == PT_LO2))&&
							(!(parts[i].life))
					   )
					{
						part_change_type(i,x,y,PT_BMTL);
						parts[i].tmp=(rand()/(RAND_MAX/10))+20;
					}
				}
			}
	return 0;
}

void IRON_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_IRON";
	elem->Name = "IRON";
	elem->Colour = COLPACK(0x707070);
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
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 50;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Rusts with salt, can be used for electrolysis of WATR";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1687.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &IRON_update;
	elem->Graphics = NULL;
}


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

int THDR_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry;
	int rcount, ri, rnext;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
				{
					rt = parts[ri].type;
					if ((ptypes[rt].properties&PROP_CONDUCTS) && parts[ri].life==0 && !(rt==PT_WATR||rt==PT_SLTW))
					{
						parts[i].type = PT_NONE;
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
					}
					else if (rt!=PT_CLNE&&rt!=PT_THDR&&rt!=PT_SPRK&&rt!=PT_DMND&&rt!=PT_FIRE&&!(ptypes[rt].properties&TYPE_ENERGY))
					{
						pv[y/CELL][x/CELL] += 100.0f;
						if (legacy_enable&&1>(rand()%200))
						{
							parts[i].life = rand()%50+120;
							part_change_type(i,x,y,PT_FIRE);
						}
						else
						{
							parts[i].type = PT_NONE;
						}
					}
				}
			}
	if (parts[i].type==PT_NONE) {
		kill_part(i);
		return 1;
	}
	return 0;
}

int THDR_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 160;
	*fireg = 192;
	*fireb = 255;
	*firer = 144;
	*pixel_mode |= FIRE_ADD;
	return 1;
}

void THDR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_THDR";
	elem->Name = "THDR";
	elem->Colour = COLPACK(0xFFFFA0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.0f;
	elem->Loss = 0.30f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.6f;
	elem->Diffusion = 0.62f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = 9000.0f		+273.15f;
	elem->HeatConduct = 1;
	elem->Latent = 0;
	elem->Description = "Lightning! Very hot, inflicts damage upon most materials, transfers current to metals.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &THDR_update;
	elem->Graphics = &THDR_graphics;
}


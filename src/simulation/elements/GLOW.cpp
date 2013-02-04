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

int GLOW_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
				{
					if (parts[ri].type==PT_WATR&&5>(rand()%2000))
					{
						parts[i].type = PT_NONE;
						part_change_type(ri,x+rx,y+ry,PT_DEUT);
						parts[ri].life = 10;
					}
				}
			}
	parts[i].ctype = pv[y/CELL][x/CELL]*16;

	parts[i].tmp = abs((int)((vx[y/CELL][x/CELL]+vy[y/CELL][x/CELL])*16.0f)) + abs((int)((parts[i].vx+parts[i].vy)*64.0f));
	//printf("%f %f\n", parts[i].vx, parts[i].vy);
	if (parts[i].type==PT_NONE) {
		kill_part(i);
		return 1;
	}
	return 0;
}

int GLOW_graphics(GRAPHICS_FUNC_ARGS)
{
	*firer = restrict_flt(cpart->temp-(275.13f+32.0f), 0, 128)/50.0f;
	*fireg = restrict_flt(cpart->ctype, 0, 128)/50.0f;
	*fireb = restrict_flt(cpart->tmp, 0, 128)/50.0f;

	*colr = restrict_flt(64.0f+cpart->temp-(275.13f+32.0f), 0, 255);
	*colg = restrict_flt(64.0f+cpart->ctype, 0, 255);
	*colb = restrict_flt(64.0f+cpart->tmp, 0, 255);
	
	*pixel_mode |= FIRE_ADD;
	return 0;
}

void GLOW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GLOW";
	elem->Name = "GLOW";
	elem->Colour = COLPACK(0x445464);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.3f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.15f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 2;

	elem->Weight = 40;

	elem->DefaultProperties.temp = R_TEMP+20.0f+273.15f;
	elem->HeatConduct = 44;
	elem->Latent = 0;
	elem->Description = "Glow, Glows under pressure";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GLOW_update;
	elem->Graphics = &GLOW_graphics;
}


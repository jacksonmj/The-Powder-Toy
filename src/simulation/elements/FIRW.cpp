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
#include "hmap.h"

int FIRW_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, rt, np;
	int rcount, ri, rnext;
	if (parts[i].tmp<=0) {
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if (rt==PT_FIRE||rt==PT_PLSM||rt==PT_THDR)
						{
							float gx, gy, multiplier;
							get_gravity_field(x, y, ptypes[PT_FIRW].gravity, 1.0f, &gx, &gy);
							if (gx*gx+gy*gy < 0.001f)
							{
								float angle = (rand()%6284)*0.001f;//(in radians, between 0 and 2*pi)
								gx += sinf(angle)*ptypes[PT_FIRW].gravity*0.5f;
								gy += cosf(angle)*ptypes[PT_FIRW].gravity*0.5f;
							}
							parts[i].tmp = 1;
							parts[i].life = rand()%10+20;
							multiplier = (parts[i].life+20)*0.2f/sqrtf(gx*gx+gy*gy);
							parts[i].vx -= gx*multiplier;
							parts[i].vy -= gy*multiplier;
							return 0;
						}
					}
				}
	}
	else if (parts[i].tmp==1) {
		if (parts[i].life<=0) {
			parts[i].tmp=2;
		} else {
			parts[i].flags &= ~FLAG_STAGNANT;
		}
	}
	else if (parts[i].tmp>=2) {
		float angle, magnitude;
		int caddress = (rand()%200)*3;
		int n;
		unsigned col = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
		for (n=0; n<40; n++)
		{
			np = sim->part_create(-3, x, y, PT_EMBR);
			if (np>-1)
			{
				magnitude = ((rand()%60)+40)*0.05f;
				angle = (rand()%6284)*0.001f;//(in radians, between 0 and 2*pi)
				parts[np].vx = parts[i].vx*0.5f + cosf(angle)*magnitude;
				parts[np].vy = parts[i].vy*0.5f + sinf(angle)*magnitude;
				parts[np].ctype = col;
				parts[np].tmp = 1;
				parts[np].life = rand()%40+70;
				parts[np].temp = (rand()%500)+5750.0f;
				parts[np].dcolour = parts[i].dcolour;
			}
		}
		pv[y/CELL][x/CELL] += 8.0f;
		kill_part(i);
		return 1;
	}
	return 0;
}

int FIRW_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->tmp > 0)
	{
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void FIRW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FIRW";
	elem->Name = "FIRW";
	elem->Colour = COLPACK(0xFFA040);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.2f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 55;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Fireworks!";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FIRW_update;
	elem->Graphics = &FIRW_graphics;
}


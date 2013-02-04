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

int create_n_parts(Simulation *sim, int n, int x, int y, float vx, float vy, float temp, int t)//testing a new deut create part
{
	int i, c;
	n = (n/50);
	if (n<1) {
		n = 1;
	}
	if (n>340) {
		n = 340;
	}
	if (x<0 || y<0 || x>=XRES || y>=YRES || t<0 || t>=PT_NUM || !ptypes[t].enabled)
		return -1;

	for (c=0; c<n; c++) {
		float r = (rand()%128+128)/127.0f;
		float a = (rand()%360)*M_PI/180.0f;
		i = sim->part_alloc();
		if (i<0)
			return -1;

		parts[i].x = (float)x;
		parts[i].y = (float)y;
#ifdef OGLR
		parts[i].lastX = (float)x;
		parts[i].lastY = (float)y;
#endif
		parts[i].type = t;
		parts[i].life = rand()%480+480;
		parts[i].vx = r*cosf(a);
		parts[i].vy = r*sinf(a);
		parts[i].ctype = 0;
		parts[i].temp = temp;
		parts[i].tmp = 0;
		sim->pmap_add(i, x, y, t);
		sim->elementCount[t]++;

		pv[y/CELL][x/CELL] += 6.0f * CFDS;
	}
	return 0;
}

int NEUT_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	int pressureFactor = 3 + (int)pv[y/CELL][x/CELL];
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
				{
					rt = parts[ri].type;
					if (rt==PT_WATR || rt==PT_ICEI || rt==PT_SNOW)
					{
						parts[i].vx *= 0.995;
						parts[i].vy *= 0.995;
					}
					if (rt==PT_PLUT && pressureFactor>(rand()%1000))
					{
						if (33>rand()%100)
						{
							sim->part_create(ri, x+rx, y+ry, rand()%3 ? PT_LAVA : PT_URAN);
							parts[ri].temp = MAX_TEMP;
							if (parts[ri].type==PT_LAVA) {
								parts[ri].tmp = 100;
								parts[ri].ctype = PT_PLUT;
							}
						}
						else
						{
							sim->part_create(ri, x+rx, y+ry, PT_NEUT);
							parts[ri].vx = 0.25f*parts[ri].vx + parts[i].vx;
							parts[ri].vy = 0.25f*parts[ri].vy + parts[i].vy;
						}
						pv[y/CELL][x/CELL] += 10.0f * CFDS; //Used to be 2, some people said nukes weren't powerful enough
						update_PYRO(UPDATE_FUNC_SUBCALL_ARGS);
					}
	#ifdef SDEUT
					else if (rt==PT_DEUT && (pressureFactor+1+(parts[ri].life/100))>(rand()%1000))
					{
						create_n_parts(sim, parts[ri].life, x+rx, y+ry, parts[i].vx, parts[i].vy, restrict_flt(parts[ri].temp + parts[ri].life*500, MIN_TEMP, MAX_TEMP), PT_NEUT);
						kill_part(ri);
					}
	#else
					else if (rt==PT_DEUT && (pressureFactor+1)>(rand()%1000))
					{
						sim->part_create(ri, x+rx, y+ry, PT_NEUT);
						parts[ri].vx = 0.25f*parts[ri].vx + parts[i].vx;
						parts[ri].vy = 0.25f*parts[ri].vy + parts[i].vy;
						if (parts[ri].life>0)
						{
							parts[ri].life --;
							parts[ri].temp = restrict_flt(parts[ri].temp + parts[ri].life*17, MIN_TEMP, MAX_TEMP);
							pv[y/CELL][x/CELL] += 6.0f * CFDS;
						}
						else
							kill_part(ri);
					}
	#endif
					else if (rt==PT_GUNP && 15>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_DUST);
					else if (rt==PT_DYST && 15>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_YEST);
					else if (rt==PT_YEST)
						part_change_type(ri,x+rx,y+ry,PT_DYST);
					else if (rt==PT_WATR && 15>(rand()%100))
						part_change_type(ri,x+rx,y+ry,PT_DSTW);
					else if (rt==PT_PLEX && 15>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_GOO);
					else if (rt==PT_NITR && 15>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_DESL);
					else if (rt==PT_PLNT && 5>(rand()%100))
						sim->part_create(ri, x+rx, y+ry, PT_WOOD);
					else if (rt==PT_DESL && 15>(rand()%1000))
						part_change_type(ri,x+rx,y+ry,PT_GAS);
					else if (rt==PT_COAL && 5>(rand()%100))
						sim->part_create(ri, x+rx, y+ry, PT_WOOD);
					else if (rt==PT_DUST && 5>(rand()%100))
						part_change_type(ri, x+rx, y+ry, PT_FWRK);
					else if (rt==PT_FWRK && 5>(rand()%100))
						parts[ri].ctype = PT_DUST;
					else if (rt==PT_ACID && 5>(rand()%100))
						sim->part_create(ri, x+rx, y+ry, PT_ISOZ);
					else if (rt==PT_TTAN && 5>(rand()%100))
					{
						kill_part(i);
						return 1;
					}
					else if (rt==PT_EXOT && 5>(rand()%100))
						parts[ri].life = 1500;
					/*if(parts[r>>8].type>1 && parts[r>>8].type!=PT_NEUT && parts[r>>8].type-1!=PT_NEUT && parts[r>>8].type-1!=PT_STKM &&
					  (ptypes[parts[r>>8].type-1].menusection==SC_LIQUID||
					  ptypes[parts[r>>8].type-1].menusection==SC_EXPLOSIVE||
					  ptypes[parts[r>>8].type-1].menusection==SC_GAS||
					  ptypes[parts[r>>8].type-1].menusection==SC_POWDERS) && 15>(rand()%1000))
					  parts[r>>8].type--;*/
				}
			}
	return 0;
}

int NEUT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 120;
	*firer = 10;
	*fireg = 80;
	*fireb = 120;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD;
	return 1;
}

void NEUT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float r = (rand()%128+128)/127.0f;
	float a = (rand()%360)*3.14159f/180.0f;
	sim->parts[i].life = rand()%480+480;
	sim->parts[i].vx = r*cosf(a);
	sim->parts[i].vy = r*sinf(a);
}

void NEUT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_NEUT";
	elem->Name = "NEUT";
	elem->Colour = COLPACK(0x20E0FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.01f;
	elem->PressureAdd_NoAmbHeat = 0.002f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+4.0f	+273.15f;
	elem->HeatConduct = 60;
	elem->Latent = 0;
	elem->Description = "Neutrons. Interact with matter in odd ways.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &NEUT_update;
	elem->Graphics = &NEUT_graphics;
	elem->Func_Create = &NEUT_create;
}


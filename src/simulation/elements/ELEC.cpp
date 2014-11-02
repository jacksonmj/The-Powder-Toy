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

int ELEC_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry, nb, rrx, rry;
	int rcount, ri, rnext;
	float rr, rrr;
	parts[i].pavg[0] = x;
	parts[i].pavg[1] = y;
	FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
	{
		if(parts[ri].type==PT_GLOW)
		{
			part_change_type(i, x, y, PT_PHOT);
		}
	}
	for (rx=-2; rx<=2; rx++)
		for (ry=-2; ry<=2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt==PT_GLAS)
					{
						for (rrx=-1; rrx<=1; rrx++)
						{
							for (rry=-1; rry<=1; rry++)
							{
								if (x+rx+rrx>=0 && y+ry+rry>=0 && x+rx+rrx<XRES && y+ry+rry<YRES) {
									nb = sim->part_create(-1, x+rx+rrx, y+ry+rry, PT_EMBR);
									if (nb!=-1) {
										parts[nb].tmp = 0;
										parts[nb].life = 50;
										parts[nb].temp = parts[i].temp*0.8f;
										parts[nb].vx = rand()%20-10;
										parts[nb].vy = rand()%20-10;
									}
								}
							}
						}
						fire_r[y/CELL][x/CELL] += rand()%200;   //D: Doesn't work with OpenGL, also shouldn't be here
						fire_g[y/CELL][x/CELL] += rand()%200;
						fire_b[y/CELL][x/CELL] += rand()%200;
						/* possible alternative, but doesn't work well at the moment because FIRE_ADD divides firea by 8, so the glow isn't strong enough
						sim->part_create(i, x, y, PT_EMBR);
						parts[i].tmp = 2;
						parts[i].life = 2;
						parts[i].ctype = ((rand()%200)<<16) | ((rand()%200)<<8) | (rand()%200);
						*/
						kill_part(i);
						return 1;
					}
					if (rt==PT_LCRY)
					{
						parts[ri].tmp2 = 5+rand()%5;
					}
					if (rt==PT_WATR || rt==PT_DSTW || rt==PT_SLTW || rt==PT_CBNW)
					{
						if(rand()<RAND_MAX/3)
						{
							sim->part_create(ri, x+rx, y+ry, PT_O2);
						}
						else
						{
							sim->part_create(ri, x+rx, y+ry, PT_H2);
						}
						kill_part(i);
						return 1;
					}
					if (rt==PT_NEUT && !sim->pmap[y+ry][x+rx].count_notEnergy)
					{
						part_change_type(ri, x+rx, y+ry, PT_H2);
						parts[ri].life = 0;
						parts[ri].ctype = 0;
					}
					if (rt==PT_DEUT)
					{
						if(parts[ri].life < 6000)
							parts[ri].life += 1;
						parts[ri].temp = 0;
						kill_part(i);
						return 1;
					}
					if (rt==PT_EXOT)
					{
						parts[ri].tmp2 += 5;
						parts[ri].life = 1000;
					}
					if (ptypes[rt].properties & PROP_CONDUCTS && (rt!=PT_NBLE||parts[i].temp<2273.15))
					{
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
						kill_part(i);
						return 1;
					}
				}
			}
	return 0;
}

int ELEC_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 70;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode |= FIRE_ADD;
	return 0;
}

void ELEC_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = (rand()%360)*3.14159f/180.0f;
	sim->parts[i].life = 680;
	sim->parts[i].vx = 2.0f*cosf(a);
	sim->parts[i].vy = 2.0f*sinf(a);
}

void ELEC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ELEC";
	elem->Name = "ELEC";
	elem->Colour = COLPACK(0xDFEFFF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+200.0f+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Electrons";

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

	elem->Update = &ELEC_update;
	elem->Graphics = &ELEC_graphics;
	elem->Func_Create = &ELEC_create;
}


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

int VIBR_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	int trade, transfer;
	if (!parts[i].life) //if not exploding
	{
		//Heat absorption code
		if (parts[i].temp > 274.65f)
		{
			parts[i].tmp++;
			parts[i].temp -= 3;
		}
		else if (parts[i].temp < 271.65f)
		{
			parts[i].tmp--;
			parts[i].temp += 3;
		}
		//Pressure absorption code
		if (pv[y/CELL][x/CELL] > 2.5)
		{
			parts[i].tmp += 7;
			pv[y/CELL][x/CELL]--;
		}
		else if (pv[y/CELL][x/CELL] < -2.5)
		{
			parts[i].tmp -= 2;
			pv[y/CELL][x/CELL]++;
		}
		//initiate explosion counter
		if (parts[i].tmp > 1000)
			parts[i].life = 750;
	}
	else //if it is exploding
	{
		//Release sparks before explode
		if (parts[i].life < 300)
		{
			int random = rand();
			rx = random%3-1;
			ry = (random>>3)%3-1;
			if(x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type != PT_BREL && (sim->elements[parts[ri].type].Properties&PROP_CONDUCTS) && !parts[ri].life)
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
				}
			}
		}
		//Release all heat
		if (parts[i].life < 500)
		{
			int random = rand();
			rx = random%7-3;
			ry = (random>>3)%7-3;
			if(x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt!=PT_VIBR && rt!=PT_BVBR && sim->elements[rt].HeatConduct && (rt!=PT_HSWC||parts[ri].life==10))
					{
						parts[ri].temp += parts[i].tmp*3;
						parts[i].tmp = 0;
					}
				}
			}
		}
		//Explosion code
		if (parts[i].life == 1)
		{
			if (!parts[i].tmp2)
			{
				int random = rand(), index;
				sim->part_create(i, x, y, PT_EXOT);
				parts[i].tmp2 = rand()%1000;
				index = sim->part_create(-3,x+((random>>4)&3)-1,y+((random>>6)&3)-1,PT_ELEC);
				if (index != -1)
					parts[index].temp = 7000;
				index = sim->part_create(-3,x+((random>>8)&3)-1,y+((random>>10)&3)-1,PT_PHOT);
				if (index != -1)
					parts[index].temp = 7000;
				index = sim->part_create(-1,x+((random>>12)&3)-1,y+rand()%3-1,PT_BREL);
				if (index != -1)
					parts[index].temp = 7000;
				parts[i].temp=9000;
				pv[y/CELL][x/CELL] += 50;

				return 1;
			}
			else
			{
				parts[i].tmp2 = 0;
				parts[i].temp = 273.15f;
				parts[i].tmp = 0;
			}
		}
	}
	//Neighbor check loop
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (parts[i].life)
					{
						if (rt==PT_VIBR  || rt==PT_BVBR)
						{
							if (!parts[ri].life)
							{
								// spread explosion
								parts[ri].tmp += 10;
							}
							else if (parts[i].tmp2 && rand()%2)
							{
								// spread defuse
								parts[ri].tmp2 = 1;
								parts[i].tmp = 0;
							}
						}
						else if (rt==PT_HFLM)
						{
							parts[i].tmp2 = 1;
							parts[i].tmp = 0;
						}
					}
					else
					{
						//Melts into EXOT
						if (rt == PT_EXOT)
						{
							if (!(rand()%25))
								sim->part_create(i, x, y, PT_EXOT);
						}
						//Absorbs energy particles
						else if ((sim->elements[rt].Properties & TYPE_ENERGY))
						{
							parts[i].tmp += 20;
							kill_part(ri);
						}
					}
					//VIBR+ANAR=BVBR
					if (rt == PT_ANAR && parts[i].type != PT_BVBR)
					{
						part_change_type(i,x,y,PT_BVBR);
						pv[y/CELL][x/CELL] -= 1;
					}
				}
			}
	if (parts[i].tmp>0)
	{
		for (trade = 0; trade < 9; trade++)
		{
			int random = rand();
			rx = random%7-3;
			ry = (random>>3)%7-3;
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type != PT_VIBR && parts[ri].type != PT_BVBR)
						continue;
					if (parts[i].tmp > parts[ri].tmp)
					{
						transfer = parts[i].tmp - parts[ri].tmp;
						if (transfer == 1)
						{
							parts[ri].tmp += 1;
							parts[i].tmp -= 1;
							trade = 9;
							break;
						}
						else if (transfer > 0)
						{
							parts[ri].tmp += transfer/2;
							parts[i].tmp -= transfer/2;
							trade = 9;
							break;
						}
					}
				}
			}
		}
	}
	if (parts[i].tmp < 0)
		parts[i].tmp = 0; // only preventing because negative tmp doesn't save
	return 0;
}

int VIBR_graphics(GRAPHICS_FUNC_ARGS)
{
	int gradient = cpart->tmp/10;
	if (gradient >= 100 || cpart->life)
	{
		*colr = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
		if (cpart->tmp2)
		{
			*colg = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
			*colb = 255;
		}
		else
		{
			*colg = 255;
			*colb = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
		}
		*firea = 90;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode = PMODE_NONE;
		*pixel_mode |= FIRE_BLEND;
	}
	else if (gradient < 100)
	{
		*colr += (int)restrict_flt(gradient*2.0f,0,255);
		*colg += (int)restrict_flt(gradient*2.0f,0,175);
		*colb += (int)restrict_flt(gradient*2.0f,0,255);
		*firea = (int)restrict_flt(gradient*.6f,0,60);
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*pixel_mode |= FIRE_BLEND;
	}
	return 0;
}

void VIBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VIBR";
	elem->Name = "VIBR";
	elem->Colour = COLPACK(0x002900);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.85f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f  +273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Vibranium. Stores energy and releases it in violent explosions.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &VIBR_update;
	elem->Graphics = &VIBR_graphics;
}


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
		if (sim->air.pv.get(SimPosI(x,y)) > 2.5)
		{
			parts[i].tmp += 7;
			sim->air.pv.add(SimPosI(x,y), -1.0f);
		}
		else if (sim->air.pv.get(SimPosI(x,y)) < -2.5)
		{
			parts[i].tmp -= 2;
			sim->air.pv.add(SimPosI(x,y), 1.0f);
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
			sim->randomRelPos_1(&rx,&ry);
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
			rx = sim->rng.randInt<-3,3>();
			ry = sim->rng.randInt<-3,3>();
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
				int index;
				sim->part_create(i, x, y, PT_EXOT);
				parts[i].tmp2 = sim->rng.randInt<0,999>();
				sim->randomRelPos_1(&rx,&ry);
				index = sim->part_create(-3,x+rx, y+ry,PT_ELEC);
				if (index != -1)
					sim->part_set_temp(parts[index], 7000);
				sim->randomRelPos_1(&rx,&ry);
				index = sim->part_create(-3,x+rx,y+ry,PT_PHOT);
				if (index != -1)
					sim->part_set_temp(parts[index], 7000);
				sim->randomRelPos_1(&rx,&ry);
				index = sim->part_create(-1,x+rx,y+ry,PT_BREL);
				if (index != -1)
					sim->part_set_temp(parts[index], 7000);
				sim->part_set_temp(parts[i], 9000);
				sim->air.pv.add(SimPosI(x,y), 50.0f);

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
							else if (parts[i].tmp2 && parts[i].life > 75 && sim->rng.chance<1,2>())
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
							if (sim->rng.chance<1,25>())
							{
								sim->part_create(i, x, y, PT_EXOT);
								return 1;
							}
						}
						//Absorbs energy particles
						else if ((sim->elements[rt].Properties & TYPE_ENERGY))
						{
							parts[i].tmp += 20;
							sim->part_kill(ri);
						}
					}
					//VIBR+ANAR=BVBR
					if (rt == PT_ANAR && parts[i].type != PT_BVBR)
					{
						sim->part_change_type(i,x,y,PT_BVBR);
						sim->air.pv.add(SimPosI(x,y), -1.0f);
					}
				}
			}
	if (parts[i].tmp>0)
	{
		for (trade = 0; trade < 9; trade++)
		{
			rx = sim->rng.randInt<-3,3>();
			ry = sim->rng.randInt<-3,3>();

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
			*colg = *colr;
			*colb = 255;
		}
		else
		{
			*colg = 255;
			*colb = *colr;
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
		*colr += (int)tptmath::clamp_flt(gradient*2.0f,0,255);
		*colg += (int)tptmath::clamp_flt(gradient*2.0f,0,175);
		*colb += (int)tptmath::clamp_flt(gradient*2.0f,0,255);
		*firea = (int)tptmath::clamp_flt(gradient*.6f,0,60);
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*pixel_mode |= FIRE_BLEND;
	}
	return 0;
}

void VIBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_VIBR";
	elem->ui->Name = "VIBR";
	elem->Colour = COLPACK(0x002900);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
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
	elem->ui->Description = "Vibranium. Stores energy and releases it in violent explosions.";

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


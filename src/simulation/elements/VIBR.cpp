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
	int r, rx, ry;
	int trade, transfer;
	if (parts[i].ctype == 1) //leaving in, just because
	{
		if (pv[y/CELL][x/CELL] > -2.5 || parts[i].tmp)
		{
			parts[i].ctype = 0;
			part_change_type(i, x, y, PT_VIBR);
		}
	}
	else if (!parts[i].life) //if not exploding
	{
		//Heat absorption code
		if (parts[i].temp > 274.65f)
		{
			parts[i].tmp++;
			parts[i].temp -= 3;
		}
		if (parts[i].temp < 271.65f)
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
		if (pv[y/CELL][x/CELL] < -2.5)
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
			rx = rand()%3-1;
			ry = rand()%3-1;
			r = pmap[y+ry][x+rx];
			if ((r&0xFF) && (r&0xFF) != PT_BREL && (ptypes[r&0xFF].properties&PROP_CONDUCTS) && !parts[r>>8].life)
			{
				parts[r>>8].life = 4;
				parts[r>>8].ctype = r&0xFF;
				part_change_type(r>>8,x+rx,y+ry,PT_SPRK);
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
				r = pmap[y+ry][x+rx];
				if ((r&0xFF) && (r&0xFF)!=PT_VIBR  && (r&0xFF)!=PT_BVBR && ptypes[r&0xFF].hconduct && ((r&0xFF)!=PT_HSWC||parts[r>>8].life==10))
				{
					parts[r>>8].temp += parts[i].tmp*3;
					parts[i].tmp = 0;
				}
			}
		}
		//Explosion code
		if (parts[i].life == 1)
		{
			int random = rand(), index;
			create_part(i, x, y, PT_EXOT);
			parts[i].tmp2 = rand()%1000;
			index = create_part(-3,x+((random>>4)&3)-1,y+((random>>6)&3)-1,PT_ELEC);
			if (index != -1)
				parts[index].temp = 7000;
			index = create_part(-3,x+((random>>8)&3)-1,y+((random>>10)&3)-1,PT_PHOT);
			if (index != -1)
				parts[index].temp = 7000;
			index = create_part(-1,x+((random>>12)&3)-1,y+rand()%3-1,PT_BREL);
			if (index != -1)
				parts[index].temp = 7000;
			parts[i].temp=9000;
			pv[y/CELL][x/CELL] += 50;

			return 1;
		}
	}
	//Neighbor check loop
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					r = photons[y+ry][x+rx];
				if (!r)
					continue;
				//Melts into EXOT
				if ((r&0xFF) == PT_EXOT && !(rand()%250))
				{
					create_part(i, x, y, PT_EXOT);
				}
				else if ((r&0xFF) == PT_ANAR)
				{
					part_change_type(i,x,y,PT_BVBR);
					pv[y/CELL][x/CELL] -= 1;
				}
				else if (parts[i].life && ((r&0xFF)==PT_VIBR  || (r&0xFF)==PT_BVBR) && !parts[r>>8].life)
				{
					parts[r>>8].tmp += 10;
				}
				//Absorbs energy particles
				if ((ptypes[r&0xFF].properties & TYPE_ENERGY))
				{
					parts[i].tmp += 20;
					kill_part(r>>8);
				}
			}
	for (trade = 0; trade < 9; trade++)
	{
		int random = rand();
		rx = random%7-3;
		ry = (random>>3)%7-3;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			r = pmap[y+ry][x+rx];
			if ((r&0xFF) != PT_VIBR && (r&0xFF) != PT_BVBR)
				continue;
			if (parts[i].tmp > parts[r>>8].tmp)
			{
				transfer = parts[i].tmp - parts[r>>8].tmp;
				if (transfer == 1)
				{
					parts[r>>8].tmp += 1;
					parts[i].tmp -= 1;
					trade = 9;
				}
				else if (transfer > 0)
				{
					parts[r>>8].tmp += transfer/2;
					parts[i].tmp -= transfer/2;
					trade = 9;
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
		*colg = 255;
		*colb = (int)(fabs(sin(exp((750.0f-cpart->life)/170)))*200.0f);
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


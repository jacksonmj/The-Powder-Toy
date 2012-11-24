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

int SPNG_update(UPDATE_FUNC_ARGS)
{
	int r, trade, rx, ry, tmp, np;
	int limit = 50;
	if (parts[i].life<limit && pv[y/CELL][x/CELL]<=3&&pv[y/CELL][x/CELL]>=-3&&parts[i].temp<=374.0f)
	{
		int absorbChanceDenom = parts[i].life*10000/limit + 500;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (((r&0xFF)==PT_WATR || (r&0xFF)==PT_DSTW || (r&0xFF)==PT_FRZW) && parts[i].life<limit && 500>rand()%absorbChanceDenom)
					{
						parts[i].life++;
						kill_part(r>>8);
					}
					if ((r&0xFF)==PT_SLTW && parts[i].life<limit && 50>rand()%absorbChanceDenom)
					{
						parts[i].life++;
						if (rand()%4)
							kill_part(r>>8);
						else
							part_change_type(r>>8, x+rx, y+ry, PT_SALT);
					}
					if ((r&0xFF)==PT_CBNW && parts[i].life<limit && 100>rand()%absorbChanceDenom)
					{
						parts[i].life++;
						part_change_type(r>>8, x+rx, y+ry, PT_CO2);
					}
					if ((r&0xFF)==PT_PSTE && parts[i].life<limit && 20>rand()%absorbChanceDenom)
					{
						parts[i].life++;
						create_part(r>>8, x+rx, y+ry, PT_CLST);
					}
				}
	}
	else
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if ((!r)&&parts[i].life>=1)//if nothing then create water
					{
						np = create_part(-1,x+rx,y+ry,PT_WATR);
						if (np>-1) parts[i].life--;
					}
				}
	for ( trade = 0; trade<9; trade ++)
	{
		rx = rand()%5-2;
		ry = rand()%5-2;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if ((r&0xFF)==PT_SPNG&&(parts[i].life>parts[r>>8].life)&&parts[i].life>0)//diffusion
			{
				tmp = parts[i].life - parts[r>>8].life;
				if (tmp ==1)
				{
					parts[r>>8].life ++;
					parts[i].life --;
					trade = 9;
				}
				else if (tmp>0)
				{
					parts[r>>8].life += tmp/2;
					parts[i].life -= tmp/2;
					trade = 9;
				}
			}
		}
	}
	tmp = 0;
	if (parts[i].life>0)
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)==PT_FIRE)
					{
						tmp++;
						if (parts[r>>8].life>60)
							parts[r>>8].life -= parts[r>>8].life/60;
						else if (parts[r>>8].life>2)
							parts[r>>8].life--;
					}
				}
	}
	if (tmp && parts[i].life>3)
		parts[i].life -= parts[i].life/3;
	if (tmp>1)
		tmp = tmp/2;
	if (tmp || parts[i].temp>=374)
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if ((!r)&&parts[i].life>=1)//if nothing then create steam
					{
						np = create_part(-1,x+rx,y+ry,PT_WTRV);
						if (np>-1)
						{
							parts[np].temp = parts[i].temp;
							tmp--;
							parts[i].life--;
							parts[i].temp -= 20.0f;
						}
					}
				}
	if (tmp>0)
	{
		if (parts[i].life>tmp)
			parts[i].life -= tmp;
		else
			parts[i].life = 0;
	}
	return 0;
}

int SPNG_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr -= cpart->life*15;
	*colg -= cpart->life*15;
	*colb -= cpart->life*15;
	if (*colr<=50)
		*colr = 50;
	if (*colg<=50)
		*colg = 50;
	if (*colb<=20)
		*colb = 20;
	return 0;
}

void SPNG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPNG";
	elem->Name = "SPNG";
	elem->Colour = COLPACK(0xFFBE30);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.00f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.0f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "A sponge, absorbs water.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2730.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &SPNG_update;
	elem->Graphics = &SPNG_graphics;
}


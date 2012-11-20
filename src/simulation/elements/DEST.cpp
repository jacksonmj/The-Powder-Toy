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

int DEST_update(UPDATE_FUNC_ARGS)
{
	int r,rx,ry,topv;
	rx=rand()%5-2;
	ry=rand()%5-2;

	r = pmap[y+ry][x+rx];
	if (!r || (r&0xFF)==PT_DEST || (r&0xFF)==PT_DMND || (r&0xFF)==PT_BCLN  || (r&0xFF)==PT_CLNE  || (r&0xFF)==PT_PCLN  || (r&0xFF)==PT_PBCN)
		return 0;

	if (parts[i].life<=0 || parts[i].life>37)
	{
		parts[i].life=30+rand()%20;
		parts[i].temp+=20000;
		pv[y/CELL][x/CELL]+=60.0f;
	}
	parts[i].temp+=10000;
	if ((r&0xFF)==PT_PLUT || (r&0xFF)==PT_DEUT)
	{
		pv[y/CELL][x/CELL]+=20.0f;
		parts[i].temp+=18000;
		if (rand()%2==0)
		{
			float orig_temp = parts[r>>8].temp;
			create_part(r>>8, x+rx, y+ry, PT_NEUT);
			parts[r>>8].temp = restrict_flt(orig_temp+40000.0f, MIN_TEMP, MAX_TEMP);
			pv[y/CELL][x/CELL] += 10.0f;
			parts[i].life-=4;
		}
	}
	else if ((r&0xFF)==PT_INSL)
	{
		create_part(r>>8, x+rx, y+ry, PT_PLSM);
	}
	else if (rand()%3==0)
	{
		kill_part(r>>8);
		parts[i].life -= 4*((ptypes[r&0xFF].properties&TYPE_SOLID)?3:1);
		if (parts[i].life<=0)
			parts[i].life=1;
		parts[i].temp+=10000;
	}
	else
	{
		if (ptypes[r&0xFF].hconduct) parts[r>>8].temp = restrict_flt(parts[r>>8].temp+10000.0f, MIN_TEMP, MAX_TEMP);
	}
	topv=pv[y/CELL][x/CELL]/9+parts[r>>8].temp/900;
	if (topv>40.0f)
		topv=40.0f;
	pv[y/CELL][x/CELL]+=40.0f+topv;
	parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
	return 0;
}

int DEST_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life)
	{
		*pixel_mode |= PMODE_LFLARE;
	}
	else
	{
		*pixel_mode |= PMODE_SPARK;
	}
	return 0;
}

void DEST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DEST";
	elem->Name = "DEST";
	elem->Colour = COLPACK(0xFF3311);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = -0.05f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.95f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 101;

	elem->CreationTemperature = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "More destructive Bomb.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DEST_update;
	elem->Graphics = &DEST_graphics;
}


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

int WARP_update(UPDATE_FUNC_ARGS)
{
	int trade, rx, ry, rt;
	int rcount, ri, rnext;

	if (parts[i].tmp2>2000)
	{
		parts[i].temp = 10000;
		pv[y/CELL][x/CELL] += (parts[i].tmp2/5000) * CFDS;
		if (2>rand()%100)
			sim->part_create(-3, x, y, PT_ELEC);
	}
	for ( trade = 0; trade<5; trade ++)
	{
		rx = rand()%3-1;
		ry = rand()%3-1;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
			{
				rt = parts[ri].type;
				if (rt!=PT_WARP&&rt!=PT_STKM&&rt!=PT_STKM2&&rt!=PT_DMND&&rt!=PT_CLNE&&rt!=PT_BCLN&&rt!=PT_PCLN)
				{
					float tmpx = parts[i].x, tmpy = parts[i].y;
					sim->part_move(i, x, y, parts[ri].x, parts[ri].y);
					sim->part_move(ri, x+rx, y+ry, tmpx, tmpy);
					parts[ri].vx = (rand()%4)-1.5;
					parts[ri].vy = (rand()%4)-2;
					parts[i].life += 4;
					trade = 5;
					break;
				}
			}
		}
	}
	return 0;
}

int WARP_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode &= ~PMODE;
	return 0;
}

void WARP_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = rand()%95+70;
}

void WARP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WARP";
	elem->Name = "WARP";
	elem->Colour = COLPACK(0x101010);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.8f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.9f;
	elem->Loss = 0.70f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 3.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP +273.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "Displaces other elements.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WARP_update;
	elem->Graphics = &WARP_graphics;
	elem->Func_Create = &WARP_create;
}


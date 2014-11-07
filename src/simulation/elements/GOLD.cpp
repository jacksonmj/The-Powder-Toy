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

int GOLD_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, blocking = 0;
	static int checkCoordsX[] = { -4, 4, 0, 0 };
	static int checkCoordsY[] = { 0, 0, -4, 4 };
	int rcount, ri, rnext;
	//Find nearby rusted iron (BMTL with tmp 1+)
	for(int j = 0; j < 8; j++){
		rx = (rand()%9)-4;
		ry = (rand()%9)-4;
		if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (!rx != !ry)) {
			FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
			{
				if (parts[ri].type==PT_BMTL && parts[ri].tmp)
				{
					parts[ri].tmp = 0;
					sim->part_change_type(ri, x+rx, y+ry, PT_IRON);
				}
			}
		}
	}
	//Find sparks
	if(!parts[i].life)
	{
		for(int j = 0; j < 4; j++){
			rx = checkCoordsX[j];
			ry = checkCoordsY[j];
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if(parts[ri].type==PT_SPRK && parts[ri].life && parts[ri].life<4)
					{
						sim->spark_particle(i, x, y);
					}
				}
			}
		}
	}
	FOR_PMAP_POSITION_ONLYENERGY(sim, x, y, rcount, ri, rnext)
	{
		if (parts[ri].type == PT_NEUT)
		{
			if (!(rand()%7))
			{
				sim->part_kill(ri);
			}
		}
	}
	return 0;
}

int GOLD_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr += rand()%10-5;
	*colg += rand()%10-5;
	*colb += rand()%10-5;
	return 0;
}

void GOLD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GOLD";
	elem->Name = "GOLD";
	elem->Colour = COLPACK(0xDCAD2C);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 1;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Corrosion resistant metal, will reverse corrosion of iron.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_HOT_GLOW|PROP_LIFE_DEC|PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1337.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &GOLD_update;
	elem->Graphics = &GOLD_graphics;
}


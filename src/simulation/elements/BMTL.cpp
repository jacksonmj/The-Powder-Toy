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

int BMTL_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	if (parts[i].tmp>1)
	{
		parts[i].tmp--;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if ((parts[ri].type==PT_METL || parts[ri].type==PT_IRON) && sim->rng.chance<1,100>())
						{
							sim->part_change_type(ri,x+rx,y+ry,PT_BMTL);
							if (parts[i].tmp<=7)
								parts[ri].tmp = parts[i].tmp = 1;
							else
								parts[ri].tmp = parts[i].tmp - sim->rng.randInt<0,4>();
						}
					}
				}
	}
	else if (parts[i].tmp==1 && sim->rng.chance<1,1000>())
	{
		parts[i].tmp = 0;
		sim->part_change_type(i,x,y,PT_BRMT);
	}
	return 0;
}

void BMTL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_BMTL";
	elem->ui->Name = "BMTL";
	elem->Colour = COLPACK(0x505070);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Breakable metal. Common conductive building material, can melt and break under pressure.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 1.0f;
	elem->HighPressureTransitionElement = ST;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &BMTL_update;
	elem->Graphics = NULL;
}


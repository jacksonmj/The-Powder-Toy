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
#include "LIFE.hpp"

int BCLN_update(UPDATE_FUNC_ARGS)
{
	if (!parts[i].life && sim->air.pv.get(SimPosI(x,y))>4.0f)
		parts[i].life = sim->rng.randInt<80,80+39>();
	if (parts[i].life)
	{
		float advection = 0.1f;
		parts[i].vx += advection*sim->air.vx.get(SimPosI(x,y));
		parts[i].vy += advection*sim->air.vy.get(SimPosI(x,y));
	}
	if (parts[i].ctype<=0 || !sim->IsValidElement(parts[i].ctype) || (parts[i].ctype==PT_LIFE && !Element_LIFE::isValidType(sim, parts[i].tmp)))
	{
		int rx, ry;
		int rcount, ri, rnext;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						int rt = parts[ri].type;
						if (rt!=PT_CLNE && rt!=PT_PCLN &&
							rt!=PT_BCLN && rt!=PT_STKM &&
							rt!=PT_PBCN && rt!=PT_STKM2)
						{
							parts[i].ctype = rt;
							if (rt==PT_LIFE || rt==PT_LAVA)
								parts[i].tmp = parts[ri].ctype;
						}
					}
				}
	}
	else {
		int rx, ry;
		sim->randomRelPos_1_noCentre(&rx,&ry);
		// TODO: change this create_part
		if (parts[i].ctype==PT_LIFE) create_part(-1, x+rx, y+ry, parts[i].ctype|(parts[i].tmp<<8));
		else if (parts[i].ctype==PT_SPRK)
			sim->spark_position(x+rx, y+ry);
		else if (parts[i].ctype!=PT_LIGH || sim->rng.chance<1,30>())
		{
			int np = sim->part_create(-1, x+rx, y+ry, parts[i].ctype);
			if (np>=0)
			{
				if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && sim->elements[parts[i].tmp].HighTemperatureTransitionElement==PT_LAVA)
					parts[np].ctype = parts[i].tmp;
			}
		}
	}
	return 0;
}

void BCLN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_BCLN";
	elem->ui->Name = "BCLN";
	elem->Colour = COLPACK(0xFFD040);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.50f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 12;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Breakable Clone.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC | PROP_DRAWONCTYPE | PROP_NOCTYPEDRAW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BCLN_update;
	elem->Graphics = NULL;
}


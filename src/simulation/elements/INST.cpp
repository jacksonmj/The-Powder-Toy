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
#include "simulation/CoordStack.h"
#include "simulation/elements/INST.h"

bool pmap_contains_sparkable_INST(Simulation *sim, int x, int y)
{
	int rcount, ri, rnext;
	FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
	{
		if (parts[ri].type==PT_INST && parts[ri].life<=0)
			return true;
	}
	return false;
}

int Element_INST::flood_spark(Simulation *sim, int x, int y)
{
	int x1, x2;
	const int cm = PT_INST;
	int created_something = 0;
	int rcount, ri, rnext;
	bool found;

	ri = sim->pmap_find_one(x, y, cm);
	if (ri<0 || parts[ri].life!=0)
		return 0;

	try
	{
		CoordStack cs;
		cs.push(x, y);

		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			// go left as far as possible
			while (x1>=CELL)
			{
				if (!pmap_contains_sparkable_INST(sim, x1-1, y)) break;
				x1--;
			}
			// go right as far as possible
			while (x2<XRES-CELL)
			{
				if (!pmap_contains_sparkable_INST(sim, x2+1, y)) break;
				x2++;
			}
			// fill span
			for (x=x1; x<=x2; x++)
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
				{
					if (parts[ri].type==cm && parts[ri].life<=0)
					{
						sim->spark_particle_nocheck_forceSPRK(ri, x, y);
						created_something = 1;
					}
				}
			}

			// add vertically adjacent pixels to stack
			// (wire crossing for INST)
			if (y>=CELL+1 && x1==x2 &&
					sim->pmap_find_one_conductive(x1-1,y-1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1,  y-1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1+1,y-1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1-1,y-2,cm)<0 &&
					sim->pmap_find_one_conductive(x1,  y-2,cm)>=0 &&
					sim->pmap_find_one_conductive(x1+1,y-2,cm)<0)
			{
				// travelling vertically up, skipping a horizontal line
				if (pmap_contains_sparkable_INST(sim, x1, y-2))
					cs.push(x1, y-2);
			}
			else if (y>=CELL+1)
			{
				for (x=x1; x<=x2; x++)
				{
					// if at the end of a horizontal section, or if it's a T junction or not a 1px wire crossing
					if (x==x1 || x==x2 || y>=YRES-CELL-1 || sim->pmap_find_one_conductive(x,y+1,cm)<0 || sim->pmap_find_one_conductive(x+1,y+1,cm)>=0 || sim->pmap_find_one_conductive(x-1,y+1,cm)>=0)
					{
						if (pmap_contains_sparkable_INST(sim, x, y-1))
							cs.push(x, y-1);
					}
				}
			}

			if (y<YRES-CELL-1 && x1==x2 &&
					sim->pmap_find_one_conductive(x1-1,y+1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1,  y+1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1+1,y+1,cm)>=0 &&
					sim->pmap_find_one_conductive(x1-1,y+2,cm)<0 &&
					sim->pmap_find_one_conductive(x1,  y+2,cm)>=0 &&
					sim->pmap_find_one_conductive(x1+1,y+2,cm)<0)
			{
				// travelling vertically down, skipping a horizontal line
				if (pmap_contains_sparkable_INST(sim, x1, y+2))
					cs.push(x1, y+2);
			}
			else if (y<YRES-CELL-1)
			{
				for (x=x1; x<=x2; x++)
				{
					// if at the end of a horizontal section, or if it's a T junction or not a 1px wire crossing
					if (x==x1 || x==x2 || y<0 || sim->pmap_find_one_conductive(x,y-1,cm)<0 || sim->pmap_find_one_conductive(x+1,y-1,cm)>=0 || sim->pmap_find_one_conductive(x-1,y-1,cm)>=0)
					{
						if (pmap_contains_sparkable_INST(sim, x, y+1))
							cs.push(x, y+1);
					}
				}
			}
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		return -1;
	}

	return created_something;
}

void INST_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_INST";
	elem->ui->Name = "INST";
	elem->Colour = COLPACK(0x404039);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
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
	elem->ui->Description = "Instantly conducts, PSCN to charge, NSCN to take.";

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

	elem->Update = NULL;
	elem->Graphics = NULL;
}


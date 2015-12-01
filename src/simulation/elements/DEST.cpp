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
	int rx,ry;
	int rcount, ri, rnext;
	float topv = 40.0f;
	sim->randomRelPos_2_noCentre(&rx,&ry);
	FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
	{
		int rt = parts[ri].type;
		if (rt==PT_DEST || rt==PT_DMND || rt==PT_BCLN  || rt==PT_CLNE  || rt==PT_PCLN  || rt==PT_PBCN)
			return 0;
	}

	if (parts[i].life<=0 || parts[i].life>37)
	{
		parts[i].life=sim->rng.randInt<30,30+19>();
		sim->air.pv.add(SimPosI(x,y), 60.0f);
	}
	FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
	{
		int rt = parts[ri].type;
		if (rt==PT_PLUT || rt==PT_DEUT)
		{
			sim->air.pv.add(SimPosI(x,y), 20.0f);
			if (sim->rng.chance<1,2>())
			{
				sim->part_create(ri, x+rx, y+ry, PT_NEUT);
				parts[ri].temp = MAX_TEMP;
				sim->air.pv.add(SimPosI(x,y), 10.0f);
				parts[i].life-=4;
			}
		}
		else if (rt==PT_INSL)
		{
			sim->part_create(ri, x+rx, y+ry, PT_PLSM);
		}
		else if (sim->rng.chance<1,3>())
		{
			sim->part_kill(ri);
			parts[i].life -= 4*((sim->elements[rt].Properties&TYPE_SOLID)?3:1);
			if (parts[i].life<=0)
				parts[i].life=1;
		}
		else
		{
			if (sim->elements[rt].HeatConduct)
				parts[ri].temp = MAX_TEMP;
		}
		topv += sim->air.pv.get(SimPosI(x,y))/9+parts[ri].temp/900;
	}
	if (topv>80.0f)
		topv = 80.0f;
	sim->air.pv.add(SimPosI(x,y), topv);
	parts[i].temp = MAX_TEMP;
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
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_DEST";
	elem->ui->Name = "DEST";
	elem->Colour = COLPACK(0xFF3311);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->ui->Description = "More destructive Bomb, can break through virtually anything.";

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


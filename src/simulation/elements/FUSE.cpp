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

int FUSE_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	int rcount, ri, rnext;
	if (parts[i].life<=0)
	{
		r = sim->part_create(i, x, y, PT_PLSM);
		if (r>=0)
			parts[r].life = 50;
		return 1;
	}
	else if (parts[i].life < 40)
	{
		parts[i].life--;
		if (!(rand()%100)) {
			r = sim->part_create(-1, x+rand()%3-1, y+rand()%3-1, PT_PLSM);
			if (r>=0)
				parts[r].life = 50;
		}
	}
	else
	{
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type==PT_SPRK || (parts[i].temp>=973.15 && !(rand()%20)))
						{
							parts[i].life = 39;
							goto doneTriggerCheck;
						}
					}
				}
	}
doneTriggerCheck:
	if ((pv[y/CELL][x/CELL] > 2.7f)&&parts[i].tmp>40)
		parts[i].tmp=39;
	else if (parts[i].tmp<=0) {
		sim->part_create(i, x, y, PT_FSEP);
		return 1;
	}
	else if (parts[i].tmp<40)
		parts[i].tmp--;

	return 0;
}

void FUSE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FUSE";
	elem->Name = "FUSE";
	elem->Colour = COLPACK(0x0A5706);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.0f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 200;
	elem->Latent = 0;
	elem->Description = "Solid. Burns slowly. Ignites at somewhat high temperatures and electricity.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 50;
	elem->DefaultProperties.tmp = 50;

	elem->Update = &FUSE_update;
	elem->Graphics = NULL;
}


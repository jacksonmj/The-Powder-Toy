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

int isRedBRAY(Simulation *sim, int xc, int yc)
{
	int rcount, ri, rnext;
	FOR_PMAP_POSITION_NOENERGY(sim, xc, yc, rcount, ri, rnext)
	{
		if (parts[ri].type == PT_BRAY && parts[ri].tmp == 2)
			return 1;
	}
	return 0;
}

int SWCH_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry;
	int rcount, ri, rnext;
	if (parts[i].life>0 && parts[i].life!=10)
		parts[i].life--;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				if (sim->pmap[y+ry][x+rx].count_notEnergy<=0 || sim->is_spark_blocked(x,y,x+rx,y+ry))
					continue;
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt==PT_SWCH)
					{
						if (parts[i].life>=10&&parts[ri].life<10&&parts[ri].life>0)
							parts[i].life = 9;
						else if (parts[i].life==0&&parts[ri].life>=10)
						{
							//Set to other particle's life instead of 10, otherwise spark loops form when SWCH is sparked while turning on
							parts[i].life = parts[ri].life;
						}
					}
					else if (rt==PT_SPRK && parts[i].life==10 && parts[ri].life>0 && parts[ri].ctype!=PT_PSCN && parts[ri].ctype!=PT_NSCN) {
						sim->spark_particle(i, x, y);
					}
				}
			}
	//turn SWCH on/off from two red BRAYS. There must be one either above or below, and one either left or right to work, and it can't come from the side, it must be a diagonal beam
	if (!isRedBRAY(sim,x-1,y-1) && !isRedBRAY(sim,x+1,y-1) && (isRedBRAY(sim, x, y-1) || isRedBRAY(sim, x, y+1)) && (isRedBRAY(sim, x+1, y) || isRedBRAY(sim, x-1, y)))
	{
		if (parts[i].life == 10)
			parts[i].life = 9;
		else if (parts[i].life <= 5)
			parts[i].life = 14;
	}
	return 0;
}

int SWCH_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life >= 10)
	{
		*colr = 17;
		*colg = 217;
		*colb = 24;
		*pixel_mode |= PMODE_GLOW;
	}
	return 0;
}

void SWCH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_SWCH";
	elem->ui->Name = "SWCH";
	elem->Colour = COLPACK(0x103B11);
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
	elem->PressureAdd_NoAmbHeat = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Only conducts when switched on. (PSCN switches on, NSCN switches off)";

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

	elem->Update = &SWCH_update;
	elem->Graphics = &SWCH_graphics;
}


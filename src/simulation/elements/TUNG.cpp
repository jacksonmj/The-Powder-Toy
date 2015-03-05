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

int TUNG_update(UPDATE_FUNC_ARGS)
{
	bool splode = false;
	const float TUNG_MELTING_POINT = sim->elements[PT_TUNG].HighTemperatureTransitionThreshold;

	if(parts[i].temp > 2400.0f)
	{
		int rcount, ri, rnext, rx, ry;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if(parts[ri].type == PT_O2)
						{
							splode = true;
						}
					}
				}
	}
	if((parts[i].temp > TUNG_MELTING_POINT && sim->rng.chance<1,20>()) || splode)
	{
		if(sim->rng.chance<1,50>())
		{
			pv[y/CELL][x/CELL] += 50.0f;
		}
		else if(sim->rng.chance<1,100>())
		{
			sim->part_change_type(i, x, y, PT_FIRE);
			parts[i].life = sim->rng.randInt<0,499>();
			return 1;
		}
		else
		{
			sim->part_change_type(i, x, y, PT_LAVA);
			parts[i].ctype = PT_TUNG;
			return 1;
		}
		if(splode)
		{
			sim->part_set_temp(parts[i], TUNG_MELTING_POINT + sim->rng.randInt<200,799>());
		}
		parts[i].vx += sim->rng.randInt<-50,50>();
		parts[i].vy += sim->rng.randInt<-50,50>();
		return 1;
	} 
	parts[i].pavg[0] = parts[i].pavg[1];
	parts[i].pavg[1] = pv[y/CELL][x/CELL];
	if (parts[i].pavg[1]-parts[i].pavg[0] > 0.50f || parts[i].pavg[1]-parts[i].pavg[0] < -0.50f)
	{
		sim->part_change_type(i,x,y,PT_BRMT);
		parts[i].ctype = PT_TUNG;
	}
	return 0;
}

void TUNG_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].pavg[1] = pv[y/CELL][x/CELL];
}

int TUNG_graphics(GRAPHICS_FUNC_ARGS)
{
	const float TUNG_MELTING_POINT = sim->elements[PT_TUNG].HighTemperatureTransitionThreshold;
	double startTemp = (TUNG_MELTING_POINT - 1500.0);
	double tempOver = (((cpart->temp - startTemp)/1500.0)*M_PI) - (M_PI/2.0);
	if(tempOver > -(M_PI/2.0))
	{
		if(tempOver > (M_PI/2.0))
			tempOver = (M_PI/2.0);
		double gradv = sin(tempOver) + 1.0;
		*firer = (int)(gradv * 258.0);
		*fireg = (int)(gradv * 156.0);
		*fireb = (int)(gradv * 112.0);
		*firea = 30;

		*colr += *firer;
		*colg += *fireg;
		*colb += *fireb;
		*pixel_mode |= FIRE_ADD;
	}
	return 0;
}

void TUNG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_TUNG";
	elem->ui->Name = "TUNG";
	elem->Colour = COLPACK(0x505050);
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
	elem->ui->Description = "Tungsten. Brittle metal with a very high melting point.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 3695.0f;// TUNG melts in its update function instead of in the normal way, but store the threshold here so that it can be changed from Lua
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &TUNG_update;
	elem->Graphics = &TUNG_graphics;
	elem->Func_Create = &TUNG_create;
}


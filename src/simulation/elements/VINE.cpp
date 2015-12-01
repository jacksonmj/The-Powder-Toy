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

int VINE_update(UPDATE_FUNC_ARGS)
{
	int np, rx, ry;
	sim->randomRelPos_1_noCentre(&rx,&ry);
	if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
	{
		if (sim->rng.chance<1,15>())
			sim->part_change_type(i,x,y,PT_PLNT);
		else
		{
			np = sim->part_create(-1,x+rx,y+ry,PT_VINE);
			if (np>=0)
			{
				parts[np].temp = parts[i].temp;
				sim->part_change_type(i,x,y,PT_PLNT);
			}
		}
	}
	if (parts[i].temp > 350 && parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;
	return 0;
}

int VINE_graphics(GRAPHICS_FUNC_ARGS)
{
	float maxtemp = cpart->temp;
	if (maxtemp < cpart->tmp2)
		maxtemp = cpart->tmp2;
	if (maxtemp > 300)
	{
		*colr += (int)tptmath::clamp_flt((maxtemp-300)/5,0,58);
		*colg -= (int)tptmath::clamp_flt((maxtemp-300)/2,0,102);
		*colb += (int)tptmath::clamp_flt((maxtemp-300)/5,0,70);
	}
	if (maxtemp < 273)
	{
		*colg += (int)tptmath::clamp_flt((273-maxtemp)/4,0,255);
		*colb += (int)tptmath::clamp_flt((273-maxtemp)/1.5,0,255);
	}
	return 0;
}

void VINE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_VINE";
	elem->ui->Name = "VINE";
	elem->Colour = COLPACK(0x079A00);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 10;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 65;
	elem->Latent = 0;
	elem->ui->Description = "Vine, can grow along WOOD.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 573.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.tmp = 1;

	elem->Update = &VINE_update;
	elem->Graphics = &VINE_graphics;
}


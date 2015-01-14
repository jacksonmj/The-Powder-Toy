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
#include "simulation/elements-shared/pyro.h"

int FIRE_graphics(GRAPHICS_FUNC_ARGS)
{
	int caddress = tptmath::clamp_int(cpart->life, 0, 200-1)*3;
	*colr = (unsigned char)flm_data[caddress];
	*colg = (unsigned char)flm_data[caddress+1];
	*colb = (unsigned char)flm_data[caddress+2];
	
	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	
	*pixel_mode = PMODE_NONE; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_ADD;
	//Returning 0 means dynamic, do not cache
	return 0;
}

void FIRE_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.randInt<120,120+49>();
}

int FIRE_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rcount, ri, rnext;
	if(parts[i].life <= 1)
	{
		if ((parts[i].tmp&3)==3)
		{
			sim->part_change_type(i,x,y,PT_DSTW);
			parts[i].life = 0;
			parts[i].ctype = PT_FIRE;
		}
		else if (parts[i].temp<625)
		{
			sim->part_change_type(i,x,y,PT_SMKE);
			parts[i].life = sim->rng.randInt<250,250+19>();
		}
	}
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (ElementsShared_pyro::update_neighbour(UPDATE_FUNC_SUBCALL_ARGS, rx, ry, parts[ri].type, ri)==1)
						return 1;
				}
			}
	return 0;
}

void FIRE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_FIRE";
	elem->ui->Name = "FIRE";
	elem->Colour = COLPACK(0xFF1000);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.9f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.20f;
	elem->Collision = 0.0f;
	elem->Gravity = -0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.001f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 2;

	elem->DefaultProperties.temp = R_TEMP+400.0f+273.15f;
	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->ui->Description = "Ignites flammable materials. Heats air.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2773.0f;
	elem->HighTemperatureTransitionElement = PT_PLSM;

	elem->Update = &FIRE_update;
	elem->Graphics = &FIRE_graphics;
	elem->Func_Create = &FIRE_create;
}


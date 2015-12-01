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

int THDR_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry;
	int rcount, ri, rnext;
	bool killPart = false;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if ((sim->elements[rt].Properties&PROP_CONDUCTS) && parts[ri].life==0 && !(rt==PT_WATR||rt==PT_SLTW))
					{
						killPart = true;
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
					}
					else if (rt!=PT_CLNE&&rt!=PT_THDR&&rt!=PT_SPRK&&rt!=PT_DMND&&rt!=PT_FIRE)
					{
						sim->air.pv.add(SimPosI(x,y), 100.0f);
						if (sim->option_heatMode()==HeatMode::Legacy && sim->rng.chance<1,200>())
						{
							parts[i].life = sim->rng.randInt<120,120+49>();
							sim->part_change_type(i,x,y,PT_FIRE);
						}
						else
						{
							killPart = true;
						}
					}
				}
			}
	if (killPart) {
		sim->part_kill(i);
		return 1;
	}
	return 0;
}

int THDR_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 160;
	*fireg = 192;
	*fireb = 255;
	*firer = 144;
	*pixel_mode |= FIRE_ADD;
	return 1;
}

void THDR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_THDR";
	elem->ui->Name = "THDR";
	elem->Colour = COLPACK(0xFFFFA0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.0f;
	elem->Loss = 0.30f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.6f;
	elem->Diffusion = 0.62f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = 9000.0f		+273.15f;
	elem->HeatConduct = 1;
	elem->Latent = 0;
	elem->ui->Description = "Lightning! Very hot, inflicts damage upon most materials, and transfers current to metals.";

	elem->Properties = TYPE_ENERGY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &THDR_update;
	elem->Graphics = &THDR_graphics;
}


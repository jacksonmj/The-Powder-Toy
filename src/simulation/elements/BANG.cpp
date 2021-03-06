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

int BANG_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	if(parts[i].tmp==0)
	{
		if(parts[i].temp>=673.0f)
			parts[i].tmp = 1;
		else
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
						{
							int rt = parts[ri].type;
							if (rt==PT_FIRE || rt==PT_PLSM || rt==PT_SPRK || rt==PT_LIGH)
							{
								parts[i].tmp = 1;
							}
						}
					}
	
	}
	else if(parts[i].tmp==1)
	{
		int tempvalue = 2;
		flood_prop(x, y, parts[i].type, offsetof(particle, tmp), &tempvalue, 0);
		parts[i].tmp = 2;
	}
	else if(parts[i].tmp==2)
	{
		parts[i].tmp = 3;
	}
	else
	{
		float otemp = parts[i].temp-273.13f;
		//Explode!!
		sim->air.pv.add(SimPosI(x,y), 0.5f);
		parts[i].tmp = 0;
		if(sim->rng.chance<1,3>())
		{
			if(sim->rng.chance<1,2>())
			{
				sim->part_create(i, x, y, PT_FIRE);
			}
			else
			{
				sim->part_create(i, x, y, PT_SMKE);
				parts[i].life = sim->rng.randInt<500,549>();
			}
			sim->part_set_temp(parts[i], (MAX_TEMP/4)+otemp);
		}
		else
		{
			if(sim->rng.chance<1,15>())
			{
				sim->part_create(i, x, y, PT_EMBR);
				parts[i].tmp = 0;
				parts[i].life = 50;
				sim->part_set_temp(parts[i], (MAX_TEMP/3)+otemp);
				parts[i].vx = sim->rng.randInt<-10,10>();
				parts[i].vy = sim->rng.randInt<-10,10>();
			}
			else
			{
				sim->part_kill(i);
			}
		}
		return 1;
	}
	return 0;
}

void BANG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_BANG";
	elem->ui->Name = "TNT";
	elem->Colour = COLPACK(0xC05050);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
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
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 88;
	elem->Latent = 0;
	elem->ui->Description = "TNT, explodes all at once.";

	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BANG_update;
	elem->Graphics = NULL;
}


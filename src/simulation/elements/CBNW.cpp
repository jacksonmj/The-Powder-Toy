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

int CBNW_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	if (sim->air.pv.get(SimPosI(x,y))<=3)
    {
		if(sim->air.pv.get(SimPosI(x,y))<=-0.5 || sim->rng.chance<1,4000>())
    	{
			sim->part_change_type(i,x,y,PT_CO2);
			parts[i].ctype = 5;
			sim->air.pv.add(SimPosI(x,y), 0.5f);
		}
	}
	if(parts[i].tmp2!=20)
	{
		parts[i].tmp2 -= (parts[i].tmp2>20)?1:-1;
	}
	else if(sim->rng.chance<1,200>())
	{
		parts[i].tmp2 = sim->rng.randInt<0,39>();
	}
	if (parts[i].tmp>0)
	{
		//Explode
		if(parts[i].tmp==1 && sim->rng.chance<1,4>())
		{
			sim->part_change_type(i,x,y,PT_CO2);
			parts[i].ctype = 5;
			sim->air.pv.add(SimPosI(x,y), 0.2f);
		}
		parts[i].tmp--;
	}
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int rt = parts[ri].type;
					if ((sim->elements[rt].Properties&TYPE_PART) && parts[i].tmp == 0 && sim->rng.chance<1,50>())
					{
						//Start explode
						parts[i].tmp = sim->rng.randInt<0,24>();
					}
					else if ((sim->elements[rt].Properties&TYPE_SOLID) && rt!=PT_DMND && rt!=PT_GLAS && parts[i].tmp == 0 && sim->rng.chance(2-sim->air.pv.get(SimPosI(x,y)), 8000))
					{
						sim->part_change_type(i,x,y,PT_CO2);
						parts[i].ctype = 5;
						sim->air.pv.add(SimPosI(x,y), 0.2f);
					}
					if (rt==PT_CBNW)
					{
						if (!parts[i].tmp)
						{
							if (parts[ri].tmp)
							{
								parts[i].tmp = parts[ri].tmp;
								if(ri>i) //If the other particle hasn't been updated
									parts[i].tmp--;
							}
						}
						else if (!parts[ri].tmp)
						{
							parts[ri].tmp = parts[i].tmp;
							if(ri>i) //If the other particle hasn't been updated
								parts[ri].tmp++;
						}
					}
					else if (rt==PT_RBDM||rt==PT_LRBD)
					{
						if ((legacy_enable||parts[i].temp>(285.15f)) && sim->rng.chance<1,100>())
						{
							sim->part_change_type(i,x,y,PT_FIRE);
							parts[i].life = 4;
							parts[i].ctype = PT_WATR;
						}
					}
					else if (rt==PT_FIRE && parts[ri].ctype!=PT_WATR)
					{
						sim->part_kill(ri);
						if(sim->rng.chance<1,30>()){
							sim->part_kill(i);
							return 1;
						}
					}
				}
			}
	return 0;
}

int CBNW_graphics(GRAPHICS_FUNC_ARGS)
{
	int z = cpart->tmp2 - 20;//speckles!
	*colr += z * 1;
	*colg += z * 2;
	*colb += z * 8;
	return 0;
}

void CBNW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_CBNW";
	elem->ui->Name = "BUBW";
	elem->Colour = COLPACK(0x2030D0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 7500;
	elem->ui->Description = "Carbonated water. Slowly releases CO2.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 273.15f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 373.0f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &CBNW_update;
	elem->Graphics = &CBNW_graphics;
}


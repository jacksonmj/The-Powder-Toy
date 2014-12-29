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

int VIRS_update(UPDATE_FUNC_ARGS)
{
	//pavg[0] measures how many frames until it is cured (0 if still actively spreading and not being cured)
	//pavg[1] measures how many frames until it dies
	int r, rx, ry, rndstore = rand();
	if (parts[i].pavg[0])
	{
		parts[i].pavg[0] -= (rndstore&0x1) ? 0:1;
		//has been cured, so change back into the original element
		if (!parts[i].pavg[0])
		{
			sim->part_change_type(i,x,y,parts[i].tmp2);
			parts[i].tmp2 = 0;
			parts[i].pavg[0] = 0;
			parts[i].pavg[1] = 0;
		}
		return 0;
		//cured virus is never in below code
	}
	//decrease pavg[1] so it slowly dies
	if (parts[i].pavg[1] > 0)
	{
		if (!(rndstore & 0x7))
		{
			parts[i].pavg[1]--;
			//if pavg[1] is now 0, kill it
			if (parts[i].pavg[1]<=0)
			{
				sim->part_kill(i);
				return 1;
			}
		}
		rndstore >>= 3;
	}

	int rcount, ri, rnext, rt;
	for (rx=-1; rx<2; rx++)
	{
		//reset rndstore, one random can last through 3 locations and reduce rand() calling by up to 6x as much
		rndstore = rand();
		for (ry=-1; ry<2; ry++)
		{
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					//spread "being cured" state
					if ((rt == PT_VIRS || rt == PT_VRSS || rt == PT_VRSG) && parts[ri].pavg[0])
					{
						parts[i].pavg[0] = parts[ri].pavg[0] + ((rndstore & 0x3) ? 2:1);
						return 0;
					}
					//soap cures virus
					else if (rt == PT_SOAP)
					{
						parts[i].pavg[0] += 10;
						if (!(rndstore & 0x3))
							sim->part_kill(ri);
						return 0;
					}
					else if (rt == PT_PLSM)
					{
						if (surround_space && 10 + (int)(pv[(y+ry)/CELL][(x+rx)/CELL]) > (rand()%100))
						{
							sim->part_create(i, x, y, PT_PLSM);
							return 1;
						}
					}
					//transforms things into virus here
					else if (rt != PT_VIRS && rt != PT_VRSS && rt != PT_VRSG && rt != PT_DMND && !(sim->elements[rt].Properties&TYPE_ENERGY))
					{
						if (!(rndstore & 0x7))
						{
							rndstore >>= 3;
							parts[ri].tmp2 = rt;
							parts[ri].pavg[0] = 0;
							if (parts[i].pavg[1])
							{
								parts[ri].pavg[1] = parts[i].pavg[1] + ((rndstore % 3) ? 1 : 0);
								rndstore >>= 2;
							}
							else
								parts[ri].pavg[1] = 0;
							if (parts[ri].temp < 305.0f)
								sim->part_change_type(ri, x+rx, y+ry, PT_VRSS);
							else if (parts[ri].temp > 673.0f)
								sim->part_change_type(ri, x+rx, y+ry, PT_VRSG);
							else
								sim->part_change_type(ri, x+rx, y+ry, PT_VIRS);
						}
						else
							rndstore >>= 3;
					}
					//protons make VIRS last forever
					else if (rt == PT_PROT)
					{
						parts[i].pavg[1] = 0;
					}
				}
			}
		}
	}
	return 0;
}

int VIRS_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_BLUR;
	*pixel_mode |= NO_DECO;
	return 1;
}

void VIRS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_VIRS";
	elem->Name = "VIRS";
	elem->Colour = COLPACK(0xFE11F6);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
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

	elem->Weight = 31;

	elem->DefaultProperties.temp = 72.0f	+ 273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Virus. Turns everything it touches into virus.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 305.0f;
	elem->LowTemperatureTransitionElement = PT_VRSS;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_VRSG;

	elem->DefaultProperties.pavg[1] = 250;

	elem->Update = &VIRS_update;
	elem->Graphics = &VIRS_graphics;
}

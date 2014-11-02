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

int CAUS_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, trade, np;
	int rcount, ri, rnext;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int rt = parts[ri].type;
					if (rt!=PT_ACID && rt!=PT_CAUS)
					{
						if ((rt!=PT_CLNE && rt!=PT_PCLN && ptypes[rt].hardness>(rand()%1000))&&parts[i].life>=50)
						{
							if (!sim->check_middle_particle_type(i, ri, PT_GLAS))//GLAS protects stuff from acid
							{
								float newtemp = ((60.0f-(float)ptypes[rt].hardness))*7.0f;
								if(newtemp < 0){
									newtemp = 0;
								}
								parts[i].temp += newtemp;
								parts[i].life--;
								kill_part(ri);
							}
						}
						else if (parts[i].life<=50)
						{
							kill_part(i);
							return 1;
						}
					}
				}
			}
	return 0;
}

void CAUS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CAUS";
	elem->Name = "CAUS";
	elem->Colour = COLPACK(0x80FFA0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 2.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 1.50f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Caustic Gas, acts like Acid";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &CAUS_update;
	elem->Graphics = NULL;
}


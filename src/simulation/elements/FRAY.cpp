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

int FRAY_update(UPDATE_FUNC_ARGS)
{
	int nxx, nyy, len, nxi, nyi, rx, ry, nr, ry1, rx1;
	int rcount, ri, rnext;
	int ncount, ni, nnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type==PT_SPRK)
					{
						for (nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1, len = 0; ; nyy+=nyi, nxx+=nxi, len++) {
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0) || len>10) {
								break;
							}
							FOR_PMAP_POSITION(sim, x+nxi+nxx, y+nyi+nyy, ncount, ni, nnext)
							{
								if (!(ptypes[parts[ri].type].properties & TYPE_SOLID)){
									parts[ri].vx += nxi*((parts[i].temp-273.15)/10.0f);
									parts[ri].vy += nyi*((parts[i].temp-273.15)/10.0f);
								}
							}
						}
						break;//break out of pmap position loop, so that stacked sparks don't cause multiple activation
					}
				}
			}
	return 0;
}

void FRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FRAY";
	elem->Name = "FRAY";
	elem->Colour = COLPACK(0x00BBFF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
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

	elem->DefaultProperties.temp = 20.0f+0.0f +273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Force Emitter. Pushes or pulls objects based on its temp value, use like ARAY.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FRAY_update;
	elem->Graphics = NULL;
}


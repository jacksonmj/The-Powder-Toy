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

int STOR_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, np, rx1, ry1;
	if(parts[i].life && !parts[i].tmp)
		parts[i].life--;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if ((r>>8)>=NPART || !r)
					continue;
				if (!parts[i].tmp && !parts[i].life && (r&0xFF)!=PT_STOR && !(ptypes[(r&0xFF)].properties&TYPE_SOLID) && (!parts[i].ctype || (r&0xFF)==parts[i].ctype))
				{
					parts[i].tmp = parts[r>>8].type;
					parts[i].temp = parts[r>>8].temp;
					parts[i].tmp2 = parts[r>>8].life;
					parts[i].pavg[0] = parts[r>>8].tmp;
					parts[i].pavg[1] = parts[r>>8].ctype;
					kill_part(r>>8);
				}
				if(parts[i].tmp && (r&0xFF)==PT_SPRK && parts[r>>8].ctype==PT_PSCN && parts[r>>8].life>0 && parts[r>>8].life<4)
				{
					for(ry1 = 1; ry1 >= -1; ry1--){
						for(rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1){ // Oscilate the X starting at 0, 1, -1, 3, -5, etc (Though stop at -1)
							np = create_part(-1,x+rx1,y+ry1,parts[i].tmp);
							if (np!=-1)
							{
								parts[np].temp = parts[i].temp;
								parts[np].life = parts[i].tmp2;
								parts[np].tmp = parts[i].pavg[0];
								parts[np].ctype = parts[i].pavg[1];
								parts[i].tmp = 0;
								parts[i].life = 10;
								break;
							}
						}
					}
				}
			}
	return 0;
}

int STOR_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->tmp){
		*pixel_mode |= PMODE_GLOW;
		*colr = 0x50;
		*colg = 0xDF;
		*colb = 0xDF;
	} else {
		*colr = 0x20;
		*colg = 0xAF;
		*colb = 0xAF;
	}
	return 0;
}

void STOR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STOR";
	elem->Name = "STOR";
	elem->Colour = COLPACK(0x50DFDF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
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

	elem->CreationTemperature = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Solid. Stores a single particle, releases when charged with PSCN, also passes to PIPE";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &STOR_update;
	elem->Graphics = &STOR_graphics;
}


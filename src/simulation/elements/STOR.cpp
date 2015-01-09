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
#include "simulation/elements/SOAP.h"

int STOR_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt, np, rx1, ry1;
	int rcount, ri, rnext;
	bool doRelease = false;
	if(parts[i].life && !parts[i].tmp)
		parts[i].life--;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (!parts[i].tmp && !parts[i].life && rt!=PT_STOR && !(ptypes[rt].properties&TYPE_SOLID) && (!parts[i].ctype || rt==parts[i].ctype))
					{
						if (rt == PT_SOAP)
							SOAP_detach(sim, ri);
						parts[i].tmp = parts[ri].type;
						parts[i].temp = parts[ri].temp;
						parts[i].tmp2 = parts[ri].life;
						parts[i].pavg[0] = parts[ri].tmp;
						parts[i].pavg[1] = parts[ri].ctype;
						kill_part(ri);
					}
					if(parts[i].tmp && rt==PT_SPRK && parts[ri].ctype==PT_PSCN && parts[ri].life>0 && parts[ri].life<4)
					{
						doRelease = true;
					}
				}
			}

	if (doRelease)
	{
		for(ry1 = 1; ry1 >= -1; ry1--){
			for(rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1){ // Oscillate the X starting at 0, 1, -1, 3, -5, etc (Though stop at -1)
				np = sim->part_create(-1,x+rx1,y+ry1,parts[i].tmp);
				if (np!=-1)
				{
					parts[np].temp = parts[i].temp;
					parts[np].life = parts[i].tmp2;
					parts[np].tmp = parts[i].pavg[0];
					parts[np].ctype = parts[i].pavg[1];
					parts[i].tmp = 0;
					parts[i].life = 10;
					return 0;
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
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_STOR";
	elem->ui->Name = "STOR";
	elem->Colour = COLPACK(0x50DFDF);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWERED;
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
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Captures and stores a single particle. releases when charged with PSCN, also passes to PIPE.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID | PROP_NOCTYPEDRAW;

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


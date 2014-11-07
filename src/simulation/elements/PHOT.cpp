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

int PHOT_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry;
	int rcount, ri, rnext;
	float rr, rrr;
	parts[i].pavg[0] = x;
	parts[i].pavg[1] = y;
	if (!(parts[i].ctype&0x3FFFFFFF)) {
		kill_part(i);
		return 1;
	}
	if (parts[i].temp > 506)
		if (!(rand()%10)) update_PYRO(UPDATE_FUNC_SUBCALL_ARGS);

	bool isQuartz = false;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					// TODO: could these reactions go in PHOT movement?
					if (rt==PT_ISOZ || rt==PT_ISZS)
					{
						parts[i].vx *= 0.90;
						parts[i].vy *= 0.90;
						sim->part_create(ri, x+rx, y+ry, PT_PHOT);
						rrr = (rand()%360)*3.14159f/180.0f;
						if (rt==PT_ISOZ)
							rr = (rand()%128+128)/127.0f;
						else
							rr = (rand()%228+128)/127.0f;
						parts[ri].vx = rr*cosf(rrr);
						parts[ri].vy = rr*sinf(rrr);
						pv[y/CELL][x/CELL] -= 15.0f * CFDS;
					}
					if (rt==PT_QRTZ && !rx && !ry)
						isQuartz = true;
				}
			}
	if (isQuartz)
	{
		float a = (rand()%360)*3.14159f/180.0f;
		parts[i].vx = 3.0f*cosf(a);
		parts[i].vy = 3.0f*sinf(a);
		if(parts[i].ctype == 0x3FFFFFFF)
			parts[i].ctype = 0x1F<<(rand()%26);
		parts[i].life++; //Delay death
	}

	return 0;
}

int PHOT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x = 0;
	*colr = *colg = *colb = 0;
	for (x=0; x<12; x++) {
		*colr += (cpart->ctype >> (x+18)) & 1;
		*colb += (cpart->ctype >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (cpart->ctype >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	*colr *= x;
	*colg *= x;
	*colb *= x;

	*firea = 100;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD;
	return 0;
}

void PHOT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = (rand()%8) * 0.78540f;
	sim->parts[i].vx = 3.0f*cosf(a);
	sim->parts[i].vy = 3.0f*sinf(a);
}

void PHOT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PHOT";
	elem->Name = "PHOT";
	elem->Colour = COLPACK(0xFFFFFF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+900.0f+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Photons. Travel in straight lines.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 680;
	elem->DefaultProperties.ctype = 0x3FFFFFFF;

	elem->Update = &PHOT_update;
	elem->Graphics = &PHOT_graphics;
	elem->Func_Create = &PHOT_create;
}


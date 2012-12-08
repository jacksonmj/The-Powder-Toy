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

int PLNT_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, np;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)==PT_WATR && 1>(rand()%250))
				{
					np = sim->part_create(r>>8,x+rx,y+ry,PT_PLNT);
					if (np<0) continue;
					parts[np].life = 0;
				}
				else if ((r&0xFF)==PT_LAVA && 1>(rand()%250))
				{
					part_change_type(i,x,y,PT_FIRE);
					parts[i].life = 4;
				}
				else if (((r&0xFF)==PT_SMKE || (r&0xFF)==PT_CO2) && (1>rand()%250))
				{
					kill_part(r>>8);
					parts[i].life = rand()%60 + 60;
				}
				else if (surround_space && ((r&0xFF)==PT_WOOD) && (1>rand()%20) && (abs(rx+ry)<=2) && (VINE_MODE || parts[i].tmp==1) )				
				{
					int nnx = rand()%3 -1;
					int nny = rand()%3 -1;
					if (x+rx+nnx>=0 && y+ry+nny>0 && x+rx+nnx<XRES && y+ry+nny<YRES && (nnx || nny))
					{
						if (pmap[y+ry+nny][x+rx+nnx])
							continue;
						np = sim->part_create(-1,x+rx+nnx,y+ry+nny,PT_VINE);
						if (np<0) continue;
						parts[np].temp = parts[i].temp;
					}
				}
			}
	if (parts[i].life==2)
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						sim->part_create(-1,x+rx,y+ry,PT_O2);
				}
		parts[i].life = 0;
	}
	if (parts[i].temp > 350 && parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;
	return 0;
}

int PLNT_graphics(GRAPHICS_FUNC_ARGS)
{
	float maxtemp = fmax(cpart->tmp2,cpart->temp);
	if (maxtemp > 300)
	{
		*colr += (int)restrict_flt((maxtemp-300)/5,0,58);
		*colg -= (int)restrict_flt((maxtemp-300)/2,0,102);
		*colb += (int)restrict_flt((maxtemp-300)/5,0,70);
	}
	if (maxtemp < 273)
	{
		*colg += (int)restrict_flt((273-maxtemp)/4,0,255);
		*colb += (int)restrict_flt((273-maxtemp)/1.5,0,255);
	}
	return 0;
}

void PLNT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PLNT";
	elem->Name = "PLNT";
	elem->Colour = COLPACK(0x0CAC00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 10;
	elem->PhotonReflectWavelengths = 0x0007C000;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 65;
	elem->Latent = 0;
	elem->Description = "Plant, drinks water and grows.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_NEUTPENETRATE|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 573.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &PLNT_update;
	elem->Graphics = &PLNT_graphics;
}


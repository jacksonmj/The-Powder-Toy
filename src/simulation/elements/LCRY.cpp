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

int LCRY_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	if(parts[i].tmp==1 || parts[i].tmp==0)
	{
		if(parts[i].tmp==1)
		{
			if(parts[i].life<=0)
				parts[i].tmp = 0;
			else
			{
				parts[i].life-=2;
				if(parts[i].life < 0)
					parts[i].life = 0;
				parts[i].tmp2 = parts[i].life;
			}	
		}
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)==PT_LCRY && parts[r>>8].tmp == 3)
					{
						parts[r>>8].tmp = 1;
					}
				}
	}
	else if(parts[i].tmp==2 || parts[i].tmp==3)
	{
		if(parts[i].tmp==2)
		{
			if(parts[i].life>=10)
				parts[i].tmp = 3;
			else
			{
				parts[i].life+=2;
				if(parts[i].life > 10)
					parts[i].life = 10;
				parts[i].tmp2 = parts[i].life;
			}
		}
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)==PT_LCRY && parts[r>>8].tmp == 0)
					{
						parts[r>>8].tmp = 2;
					}
				}
	}
	return 0;
}

int LCRY_graphics(GRAPHICS_FUNC_ARGS)
{
	if(decorations_enable && cpart->dcolour && cpart->dcolour&0xFF000000)
	{
		*colr = (cpart->dcolour>>16)&0xFF;
		*colg = (cpart->dcolour>>8)&0xFF;
		*colb = (cpart->dcolour)&0xFF;

		if(cpart->tmp2<10){
			*colr /= 10-cpart->tmp2;
			*colg /= 10-cpart->tmp2;
			*colb /= 10-cpart->tmp2;
		}
		
	}
	else
	{
		*colr = *colg = *colb = 0x50+((cpart->tmp2>10?10:cpart->tmp2)*10);
	}
	*pixel_mode |= NO_DECO;
	return 0;
					
	/*int lifemod = ((cpart->tmp2>10?10:cpart->tmp2)*10);
	*colr += lifemod; 
	*colg += lifemod; 
	*colb += lifemod; 
	if(decorations_enable && cpart->dcolour && cpart->dcolour&0xFF000000)
	{
		lifemod *= 2.5f;
		if(lifemod < 40)
			lifemod = 40;
		*colr = (lifemod*((cpart->dcolour>>16)&0xFF) + (255-lifemod)**colr) >> 8;
		*colg = (lifemod*((cpart->dcolour>>8)&0xFF) + (255-lifemod)**colg) >> 8;
		*colb = (lifemod*((cpart->dcolour)&0xFF) + (255-lifemod)**colb) >> 8;
	}
	*pixel_mode |= NO_DECO;
	return 0;*/
}

void LCRY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LCRY";
	elem->Name = "LCRY";
	elem->Colour = COLPACK(0x505050);
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

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Liquid Crystal. Changes colour when charged. (PSCN Charges, NSCN Discharges)";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1273.0f;
	elem->HighTemperatureTransitionElement = PT_BGLA;

	elem->Update = &LCRY_update;
	elem->Graphics = &LCRY_graphics;
}


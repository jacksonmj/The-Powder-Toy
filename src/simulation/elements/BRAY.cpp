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
#include "simulation/elements-shared/Element_UI_ctypeWavelengths.h"
#include "simulation/elements-shared/Wavelengths.hpp"

int BRAY_graphics(GRAPHICS_FUNC_ARGS)
{
	int trans = 255;
	if(cpart->tmp==0)
	{
		trans = cpart->life * 7;
		if (trans>255) trans = 255;
		if (cpart->ctype&0x3FFFFFFF) {
			ElementsShared::Wavelengths::toRGB_noLimit(cpart->ctype, *colr, *colg, *colb);
		}
	}
	else if(cpart->tmp==1)
	{
		trans = cpart->life/4;
		if (trans>255) trans = 255;
		if (cpart->ctype&0x3FFFFFFF) {
			ElementsShared::Wavelengths::toRGB_noLimit(cpart->ctype, *colr, *colg, *colb);
		}
	}
	else if(cpart->tmp==2)
	{
		trans = cpart->life*100;
		if (trans>255) trans = 255;
		*colr = 255;
		*colg = 150;
		*colb = 50;
	}
	*cola = trans;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND | PMODE_GLOW;
	return 0;
}

void BRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI_ctypeWavelengths>();

	elem->Identifier = "DEFAULT_PT_BRAY";
	elem->ui->Name = "BRAY";
	elem->Colour = COLPACK(0xFFFFFF);
	elem->ui->MenuVisible = 0;
	elem->ui->MenuSection = SC_ELEC;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Ray Point. Rays create points when they collide.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 30;

	elem->Update = NULL;
	elem->Graphics = &BRAY_graphics;
}


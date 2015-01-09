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

int WOOD_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 450 && parts[i].temp > parts[i].tmp)
		parts[i].tmp = (int)parts[i].temp;
	return 0;
}

int WOOD_graphics(GRAPHICS_FUNC_ARGS)
{
	float maxtemp = fmax(cpart->tmp,cpart->temp);
	if (maxtemp > 400)
	{
		*colr -= (int)restrict_flt((maxtemp-400)/3,0,172);
		*colg -= (int)restrict_flt((maxtemp-400)/4,0,140);
		*colb -= (int)restrict_flt((maxtemp-400)/20,0,44);
	}
	if (maxtemp < 273)
	{
		*colr -= (int)restrict_flt((273-maxtemp)/5,0,40);
		*colg += (int)restrict_flt((273-maxtemp)/4,0,40);
		*colb += (int)restrict_flt((273-maxtemp)/1.5,0,150);
	}
	return 0;
}

void WOOD_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_WOOD";
	elem->ui->Name = "WOOD";
	elem->Colour = COLPACK(0xC0A040);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
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

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 15;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 164;
	elem->Latent = 0;
	elem->ui->Description = "Wood, flammable.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 873.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &WOOD_update;
	elem->Graphics = &WOOD_graphics;
}


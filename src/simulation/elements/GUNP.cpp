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

void GUNP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_GUNP";
	elem->ui->Name = "GUN";
	elem->Colour = COLPACK(0xC0C0D0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.02f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.80f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 600;
	elem->Explosive = 1;
	elem->Meltable = 0;
	elem->Hardness = 10;

	elem->Weight = 85;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 97;
	elem->Latent = 0;
	elem->ui->Description = "Gunpowder. Light dust, explodes on contact with fire or spark.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 673.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = NULL;
	elem->Graphics = NULL;
}


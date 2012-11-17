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

void EMBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_EMBR";
	elem->Name = "EMBR";
	elem->Colour = COLPACK(0xFFF288);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.001f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.90f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.07f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->CreationTemperature = 500.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Sparks. Formed by explosions.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &update_EMBR;
	elem->Graphics = &graphics_EMBR;
}


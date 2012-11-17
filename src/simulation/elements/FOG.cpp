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

void FOG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FOG";
	elem->Name = "FOG";
	elem->Colour = COLPACK(0xAAAAAA);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_CRACKER2;
	elem->Enabled = 1;

	elem->Advection = 0.8f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.4f;
	elem->Loss = 0.70f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.99f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 1;

	elem->CreationTemperature = 243.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->Description = "Not quite Steam";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 373.15f;
	elem->HighTemperatureTransitionElement = PT_WTRV;

	elem->Update = &update_FOG;
	elem->Graphics = NULL;
}


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

void STKM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_STKM";
	elem->Name = "STKM";
	elem->Colour = PIXPACK(0xFFE0A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->CreationTemperature = R_TEMP+14.6f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Stickman. Don't kill him!";

	elem->State = ST_NONE;
	elem->Properties = 0;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &update_STKM;
	elem->Graphics = &graphics_STKM;
}


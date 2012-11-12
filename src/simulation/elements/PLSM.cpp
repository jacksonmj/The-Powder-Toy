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

void PLSM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PLSM";
	elem->Name = "PLSM";
	elem->Colour = PIXPACK(0xBB99FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 0.9f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.20f;
	elem->Collision = 0.0f;
	elem->Gravity = -0.1f;
	elem->Diffusion = 0.30f;
	elem->PressureAdd_NoAmbHeat = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 1;

	elem->CreationTemperature = 10000.0f	+273.15f;
	elem->HeatConduct = 5;
	elem->Latent = 0;
	elem->Description = "Plasma, extremely hot.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &update_PYRO;
	elem->Graphics = &graphics_PLSM;
}


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

int GLAS_update(UPDATE_FUNC_ARGS)
{
	parts[i].pavg[0] = parts[i].pavg[1];
	parts[i].pavg[1] = pv[y/CELL][x/CELL];
	if (parts[i].pavg[1]-parts[i].pavg[0] > 0.25f || parts[i].pavg[1]-parts[i].pavg[0] < -0.25f)
	{
		part_change_type(i,x,y,PT_BGLA);
	}
	return 0;
}

void GLAS_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].pavg[1] = pv[y/CELL][x/CELL];
}

void GLAS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GLAS";
	elem->Name = "GLAS";
	elem->Colour = COLPACK(0x404040);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
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
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 150;
	elem->Latent = 0;
	elem->Description = "Solid. Meltable. Shatters under pressure";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID | PROP_NEUTPASS | PROP_HOT_GLOW | PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 1973.0f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &GLAS_update;
	elem->Graphics = NULL;
	elem->Func_Create = &GLAS_create;
}


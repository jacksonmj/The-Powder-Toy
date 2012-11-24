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

int GRAV_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = 20;
	*colg = 20;
	*colb = 20;
	if (cpart->vx>0)
	{
		*colr += (cpart->vx)*GRAV_R;
		*colg += (cpart->vx)*GRAV_G;
		*colb += (cpart->vx)*GRAV_B;
	}
	if (cpart->vy>0)
	{
		*colr += (cpart->vy)*GRAV_G;
		*colg += (cpart->vy)*GRAV_B;
		*colb += (cpart->vy)*GRAV_R;

	}
	if (cpart->vx<0)
	{
		*colr -= (cpart->vx)*GRAV_B;
		*colg -= (cpart->vx)*GRAV_R;
		*colb -= (cpart->vx)*GRAV_G;

	}
	if (cpart->vy<0)
	{
		*colr -= (cpart->vy)*GRAV_R2;
		*colg -= (cpart->vy)*GRAV_G2;
		*colb -= (cpart->vy)*GRAV_B2;
	}
	return 0;
}

void GRAV_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_GRAV";
	elem->Name = "GRAV";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 10;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 85;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->Description = "Very light dust. Changes colour based on velocity.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_PART;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &update_MISC;
	elem->Graphics = &GRAV_graphics;
}


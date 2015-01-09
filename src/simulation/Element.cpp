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

#include "simulation/Element.h"
#include "simulation/Simulation.h"

#include "powdergraphics.h"


Element::Element() :
	Identifier(""),
	Name(""),
	Colour(COLPACK(0xFFFFFF)),
	MenuVisible(0),
	MenuSection(0),
	Enabled(0),
	Advection(0.0f),
	AirDrag(0.0f),
	AirLoss(1.00f),
	Loss(0.00f),
	Collision(0.0f),
	Gravity(0.0f),
	Diffusion(0.00f),
	PressureAdd_NoAmbHeat(0.00f),
	Falldown(0),
	Flammable(0),
	Explosive(0),
	Meltable(0),
	Hardness(0),
	PhotonReflectWavelengths(0x3FFFFFFF),
	Weight(100),
	HeatConduct(255),
	Latent(0),
	Description(""),
	State(ST_NONE),
	Properties(0),
	LowPressureTransitionThreshold(IPL),
	LowPressureTransitionElement(NT),
	HighPressureTransitionThreshold(IPH),
	HighPressureTransitionElement(NT),
	LowTemperatureTransitionThreshold(ITL),
	LowTemperatureTransitionElement(NT),
	HighTemperatureTransitionThreshold(ITH),
	HighTemperatureTransitionElement(NT),
	Update(NULL),
	Graphics(&Element::Graphics_default),
	Func_Create(NULL),
	Func_Create_Override(NULL),
	Func_Create_Allowed(NULL),
	Func_ChangeType(NULL),
	DefaultProperties({})
{
	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.pmap_prev = -1;
	DefaultProperties.pmap_next = -1;
}


int Element::Graphics_default(GRAPHICS_FUNC_ARGS)
{
	int t = cpart->type;
	//Property based defaults
	if (sim->elements[t].Properties & PROP_RADIOACTIVE) *pixel_mode |= PMODE_GLOW;
	if(sim->elements[t].Properties & TYPE_LIQUID)
	{
		*pixel_mode |= PMODE_BLUR;
	}
	if(sim->elements[t].Properties & TYPE_GAS)
	{
		*pixel_mode &= ~PMODE;
		*pixel_mode |= FIRE_BLEND;
		*firer = *colr/2;
		*fireg = *colg/2;
		*fireb = *colb/2;
		*firea = 125;
		*pixel_mode |= DECO_FIRE;
	}
	return 1;
}

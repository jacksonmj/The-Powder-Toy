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

#include "powder.h"

Element::Element() :
	Identifier(""),
	Name(""),
	Colour(PIXPACK(0xFFFFFF)),
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
	Graphics(NULL),
	Func_Create(NULL),
	Func_Create_Override(NULL),
	DefaultProperties({})
{
	DefaultProperties.temp = R_TEMP + 273.15f;
}

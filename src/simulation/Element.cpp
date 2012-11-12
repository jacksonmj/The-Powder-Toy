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

Element::Element() :
	Identifier(""),
	Name(""),
	Colour(PIXPACK(0xFFFFFF)),
	MenuVisible(0),
	MenuSection(0),
	Enabled(0),
	Advection(0.0f),
	AirDrag(0.0f) /*;
	float AirLoss;
	float Loss;
	float Collision;
	float Gravity;
	float Diffusion;
	float PressureAdd_NoAmbHeat;
	int Falldown;

	int Flammable;
	int Explosive;
	int Meltable;
	int Hardness;

	int Weight;

	float CreationTemperature;
	unsigned char HeatConduct;
	unsigned int Latent;
	char *Description;

	char State;
	unsigned int Properties;

	float LowPressureTransitionThreshold;
	int LowPressureTransitionElement;
	float HighPressureTransitionThreshold;
	int HighPressureTransitionElement;
	float LowTemperatureTransitionThreshold;
	int LowTemperatureTransitionElement;
	float HighTemperatureTransitionThreshold;
	int HighTemperatureTransitionElement;

	int (*Update) (UPDATE_FUNC_ARGS);
	int (*Graphics) (GRAPHICS_FUNC_ARGS);*/
{
	;
}

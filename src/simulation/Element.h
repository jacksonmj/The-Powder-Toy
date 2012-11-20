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

#ifndef ELEMENT_H
#define ELEMENT_H

#include "graphics/ARGBColour.h"

// TODO: remove these undefs, once the defines have been removed from powder.h
#undef UPDATE_FUNC_ARGS
#undef UPDATE_FUNC_SUBCALL_ARGS
#undef GRAPHICS_FUNC_ARGS
#undef GRAPHICS_FUNC_SUBCALL_ARGS

class Simulation;
struct particle;

#define UPDATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int surround_space, int nt
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt
#define GRAPHICS_FUNC_ARGS Simulation *sim, particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb

class Element
{
public:
	char *Identifier;
	char *Name;
	ARGBColour Colour;
	int MenuVisible;
	int MenuSection;
	int Enabled;

	float Advection;
	float AirDrag;
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

	/* Weight Help
	 * 1   = Gas   ||
	 * 2   = Light || Liquids  0-49
	 * 98  = Heavy || Powder  50-99
	 * 100 = Solid ||
	 * -1 is Neutrons and Photons
	 */
	int Weight;

	float CreationTemperature;
	unsigned char HeatConduct;
	// Latent value is in TPT imaginary units - 750/226*enthalpy value of the material
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
	int (*Graphics) (GRAPHICS_FUNC_ARGS);

	Element();
	virtual ~Element() {}

};

#endif

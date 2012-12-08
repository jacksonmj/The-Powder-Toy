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
#include "simulation/Particle.h"

// TODO: remove these undefs, once the defines have been removed from powder.h
#undef UPDATE_FUNC_ARGS
#undef UPDATE_FUNC_SUBCALL_ARGS
#undef GRAPHICS_FUNC_ARGS
#undef GRAPHICS_FUNC_SUBCALL_ARGS

class Simulation;

#define UPDATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int surround_space, int nt
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt
#define GRAPHICS_FUNC_ARGS Simulation *sim, particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb
#define ELEMENT_CREATE_FUNC_ARGS Simulation *sim, int i, int x, int y
#define ELEMENT_CREATE_OVERRIDE_FUNC_ARGS Simulation *sim, int p, int x, int y, int t


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
	// Photon wavelengths are ANDed with this value when a photon hits an element, meaning that only wavelengths present in both this value and the original photon will remain in the reflected photon
	unsigned int PhotonReflectWavelengths;

	/* Weight Help
	 * 1   = Gas   ||
	 * 2   = Light || Liquids  0-49
	 * 98  = Heavy || Powder  50-99
	 * 100 = Solid ||
	 * -1 is Neutrons and Photons
	 */
	int Weight;

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
	// Func_Create can be used to set initial properties that are not constant (e.g. a random life value)
	// It cannot be used to block creation, to do that use Func_Create_Override and return -1 to block or -4 to allow
	// Particle type should not be changed in this function
	void (*Func_Create)(ELEMENT_CREATE_FUNC_ARGS);

	// Func_Create_Override can be used to completely override part_create
	// Coordinates and particle type are checked before calling this.
	// The meaning of the return value is identical to part_create, except that returning -4 means continue with part_create as though there was no override function
	int (*Func_Create_Override)(ELEMENT_CREATE_OVERRIDE_FUNC_ARGS);

	particle DefaultProperties;

	Element();
	virtual ~Element() {}

};

#endif

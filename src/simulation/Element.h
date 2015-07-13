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

#ifndef Simulation_Element_H
#define Simulation_Element_H

#include "graphics/ARGBColour.h"
#include "simulation/Particle.h"
#include "simulation/Element_UI.h"
#include <string>

class Simulation;


#define ST_NONE 0
#define ST_SOLID 1
#define ST_LIQUID 2
#define ST_GAS 3
/*
   TODO: We should start to implement these.
*/
#define TYPE_PART			0x00001 //1 Powders
#define TYPE_LIQUID			0x00002 //2 Liquids
#define TYPE_SOLID			0x00004 //4 Solids
#define TYPE_GAS			0x00008 //8 Gases (Includes plasma)
#define TYPE_ENERGY			0x00010 //16 Energy (Thunder, Light, Neutrons etc.)
#define PROP_CONDUCTS		0x00020 //32 Conducts electricity
#define PROP_BLACK			0x00040 //64 Absorbs Photons (not currently implemented or used, a photwl attribute might be better)
#define PROP_NEUTPENETRATE	0x00080 //128 Penetrated by neutrons
#define PROP_NEUTABSORB		0x00100 //256 Absorbs neutrons, reflect is default
#define PROP_NEUTPASS		0x00200 //512 Neutrons pass through, such as with glass
#define PROP_DEADLY			0x00400 //1024 Is deadly for stickman
#define PROP_HOT_GLOW		0x00800 //2048 Hot Metal Glow
#define PROP_LIFE			0x01000 //4096 Is a GoL type
#define PROP_RADIOACTIVE	0x02000 //8192 Radioactive
#define PROP_LIFE_DEC		0x04000 //2^14 Life decreases by one every frame if > zero
#define PROP_LIFE_KILL		0x08000 //2^15 Kill when life value is <= zero
#define PROP_LIFE_KILL_DEC	0x10000 //2^16 Kill when life value is decremented to<= zero
#define PROP_SPARKSETTLE	0x20000	//2^17 Allow Sparks/Embers to settle
#define PROP_NOAMBHEAT      0x40000 //2^18 Don't transfer or receive heat from ambient heat.
#define PROP_DRAWONCTYPE       0x80000  //2^19 Set its ctype to another element if the element is drawn upon it (like what CLNE does)
#define PROP_NOCTYPEDRAW       0x100000 // 2^20 When this element is drawn upon with, do not set ctype (like BCLN for CLNE)



#define UPDATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int surround_space, int nt, particle *parts
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt, parts
#define GRAPHICS_FUNC_ARGS Simulation *sim, particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb
#define ELEMENT_CREATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int t
#define ELEMENT_CREATE_OVERRIDE_FUNC_ARGS Simulation *sim, int p, int x, int y, int t
#define ELEMENT_CREATE_ALLOWED_FUNC_ARGS Simulation *sim, int i, int x, int y, int t
#define ELEMENT_CHANGETYPE_FUNC_ARGS Simulation *sim, int i, int x, int y, int from, int to
#define ELEMENT_SIMINIT_FUNC_ARGS Simulation *sim, int t

class Element
{
public:
	Element_UI *ui;

	std::string Identifier;
	ARGBColour Colour;
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

	// Latent value is in TPT imaginary units - 750/226*enthalpy value of the material
	unsigned int Latent;

	char State;
	// NB: if (Properties&TYPE_ENERGY) is changed, pmap needs rebuilding (sim->pmap_reset()) since it is divided according to this property
	unsigned int Properties;

	float LowPressureTransitionThreshold;
	int LowPressureTransitionElement;
	float HighPressureTransitionThreshold;
	int HighPressureTransitionElement;
	float LowTemperatureTransitionThreshold;
	int LowTemperatureTransitionElement;
	float HighTemperatureTransitionThreshold;
	int HighTemperatureTransitionElement;

	float HeatCapacity;// Joules per Kelvin per particle
	unsigned char HeatConduct;// 0-250, 250 is most conductive
	float LowTemperatureTransitionEnergy;// Joules per particle
	float HighTemperatureTransitionEnergy;// Joules per particle

	int (*Update) (UPDATE_FUNC_ARGS);
	int (*Graphics) (GRAPHICS_FUNC_ARGS);
	// Func_Create can be used to set initial properties that are not constant (e.g. a random life value)
	// It cannot be used to block creation, to do that use Func_Create_Allowed
	// Particle type should not be changed in this function
	void (*Func_Create)(ELEMENT_CREATE_FUNC_ARGS);

	// Func_Create_Override can be used to completely override Simulation::part_create
	// Coordinates and particle type are checked before calling this.
	// The meaning of the return value is identical to part_create, except that returning -4 means continue with part_create as though there was no override function
	int (*Func_Create_Override)(ELEMENT_CREATE_OVERRIDE_FUNC_ARGS);

	// Func_Create_Allowed is used to check whether a particle can be created, by both Simulation::part_create and Simulation::part_change_type
	// Arguments are the same as Simulation::part_create or Simulation::part_change_type
	// This function should not modify the particle
	// Before calling this, coordinates and particle type are checked, but not part_canMove()
	bool (*Func_Create_Allowed)(ELEMENT_CREATE_ALLOWED_FUNC_ARGS);

	// Func_ChangeType is called by Simulation::part_create, Simulation::part_change_type, and Simulation::part_kill
	// It should be used for things such as setting STKM legs and allocating/freeing a fighters[] slot
	// For part_create and part_change_type, it is called at the end of the function, after the pmap and all the properties and element counts are set
	// For part_kill, it is called at the start of the function, before modifying particle properties or removing it from the pmap
	void (*Func_ChangeType)(ELEMENT_CHANGETYPE_FUNC_ARGS);

	// Func_SimInit is called when a simulation is created, normally just used to create ElemDataSim objects for the new simulation
	void (*Func_SimInit)(ELEMENT_SIMINIT_FUNC_ARGS);

	particle DefaultProperties;

	template<class ElemUI_Class_T, typename... Args>
	void ui_create(Args&&... args) {
		delete ui;
		ui = new ElemUI_Class_T(this, args...);
	}
	Element();
	virtual ~Element() {
		delete ui;
	}
	static int Graphics_default(GRAPHICS_FUNC_ARGS);
};

#endif

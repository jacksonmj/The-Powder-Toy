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

#include "powder.h"
#include "misc.h"
#include "simulation/Element.h"
#include "simulation/Simulation.h"

// Declare the element initialisation functions
#define ElementNumbers_Include_Decl
#define DEFINE_ELEMENT(name, id) void name ## _init_element(ELEMENT_INIT_FUNC_ARGS);
#include "simulation/ElementNumbers.h"


Simulation *globalSim = NULL; // TODO: remove this global variable

Simulation::Simulation() :
	pfree(-1)
{
}

void Simulation::InitElements()
{
	#define DEFINE_ELEMENT(name, id) if (id>=0 && id<PT_NUM) { name ## _init_element(&elements[id], id); };
	#define ElementNumbers_Include_Call
	#include "simulation/ElementNumbers.h"

	Simulation_Compat_CopyData(this);
}




void Simulation_Compat_CopyData(Simulation* sim)
{
	// TODO: this can be removed once all the code uses Simulation instead of global variables
	parts = sim->parts;


	for (int t=0; t<PT_NUM; t++)
	{
		ptypes[t].name = mystrdup(sim->elements[t].Name);
		ptypes[t].pcolors = PIXRGB(COLR(sim->elements[t].Colour), COLG(sim->elements[t].Colour), COLB(sim->elements[t].Colour));
		ptypes[t].menu = sim->elements[t].MenuVisible;
		ptypes[t].menusection = sim->elements[t].MenuSection;
		ptypes[t].enabled = sim->elements[t].Enabled;
		ptypes[t].advection = sim->elements[t].Advection;
		ptypes[t].airdrag = sim->elements[t].AirDrag;
		ptypes[t].airloss = sim->elements[t].AirLoss;
		ptypes[t].loss = sim->elements[t].Loss;
		ptypes[t].collision = sim->elements[t].Collision;
		ptypes[t].gravity = sim->elements[t].Gravity;
		ptypes[t].diffusion = sim->elements[t].Diffusion;
		ptypes[t].hotair = sim->elements[t].PressureAdd_NoAmbHeat;
		ptypes[t].falldown = sim->elements[t].Falldown;
		ptypes[t].flammable = sim->elements[t].Flammable;
		ptypes[t].explosive = sim->elements[t].Explosive;
		ptypes[t].meltable = sim->elements[t].Meltable;
		ptypes[t].hardness = sim->elements[t].Hardness;
		ptypes[t].weight = sim->elements[t].Weight;
		ptypes[t].heat = sim->elements[t].CreationTemperature;
		ptypes[t].hconduct = sim->elements[t].HeatConduct;
		ptypes[t].descs = mystrdup(sim->elements[t].Description);
		ptypes[t].state = sim->elements[t].State;
		ptypes[t].properties = sim->elements[t].Properties;
		ptypes[t].update_func = sim->elements[t].Update;
		ptypes[t].graphics_func = sim->elements[t].Graphics;

		ptransitions[t].plt = sim->elements[t].LowPressureTransitionElement;
		ptransitions[t].plv = sim->elements[t].LowPressureTransitionThreshold;
		ptransitions[t].pht = sim->elements[t].HighPressureTransitionElement;
		ptransitions[t].phv = sim->elements[t].HighPressureTransitionThreshold;
		ptransitions[t].tlt = sim->elements[t].LowTemperatureTransitionElement;
		ptransitions[t].tlv = sim->elements[t].LowTemperatureTransitionThreshold;
		ptransitions[t].tht = sim->elements[t].HighTemperatureTransitionElement;
		ptransitions[t].thv = sim->elements[t].HighTemperatureTransitionThreshold;

		platent[t] = sim->elements[t].Latent;
	}
}

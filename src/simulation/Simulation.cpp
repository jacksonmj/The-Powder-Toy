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


Simulation::Simulation()
{
}

void Simulation::InitElements()
{
	#define DEFINE_ELEMENT(name, id) if (id>=0 && id<PT_NUM) { name ## _init_element(&elements[id], id); };
	#define ElementNumbers_Include_Call
	#include "simulation/ElementNumbers.h"

	Compat_CopyElementProperties();
}

void Simulation::Compat_CopyElementProperties()
{
	// TODO: this can be removed once all the code uses Simulation::elements instead of ptypes
	for (int t=0; t<PT_NUM; t++)
	{
		ptypes[t].name = mystrdup(elements[t].Name);
		ptypes[t].pcolors = PIXRGB(COLR(elements[t].Colour), COLG(elements[t].Colour), COLB(elements[t].Colour));
		ptypes[t].menu = elements[t].MenuVisible;
		ptypes[t].menusection = elements[t].MenuSection;
		ptypes[t].enabled = elements[t].Enabled;
		ptypes[t].advection = elements[t].Advection;
		ptypes[t].airdrag = elements[t].AirDrag;
		ptypes[t].airloss = elements[t].AirLoss;
		ptypes[t].loss = elements[t].Loss;
		ptypes[t].collision = elements[t].Collision;
		ptypes[t].gravity = elements[t].Gravity;
		ptypes[t].diffusion = elements[t].Diffusion;
		ptypes[t].hotair = elements[t].PressureAdd_NoAmbHeat;
		ptypes[t].falldown = elements[t].Falldown;
		ptypes[t].flammable = elements[t].Flammable;
		ptypes[t].explosive = elements[t].Explosive;
		ptypes[t].meltable = elements[t].Meltable;
		ptypes[t].hardness = elements[t].Hardness;
		ptypes[t].weight = elements[t].Weight;
		ptypes[t].heat = elements[t].CreationTemperature;
		ptypes[t].hconduct = elements[t].HeatConduct;
		ptypes[t].descs = mystrdup(elements[t].Description);
		ptypes[t].state = elements[t].State;
		ptypes[t].properties = elements[t].Properties;
		ptypes[t].update_func = elements[t].Update;
		ptypes[t].graphics_func = elements[t].Graphics;

		ptransitions[t].plt = elements[t].LowPressureTransitionElement;
		ptransitions[t].plv = elements[t].LowPressureTransitionThreshold;
		ptransitions[t].pht = elements[t].HighPressureTransitionElement;
		ptransitions[t].phv = elements[t].HighPressureTransitionThreshold;
		ptransitions[t].tlt = elements[t].LowTemperatureTransitionElement;
		ptransitions[t].tlv = elements[t].LowTemperatureTransitionThreshold;
		ptransitions[t].tht = elements[t].HighTemperatureTransitionElement;
		ptransitions[t].thv = elements[t].HighTemperatureTransitionThreshold;

		platent[t] = elements[t].Latent;
	}
}

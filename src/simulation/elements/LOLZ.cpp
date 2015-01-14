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
#include "simulation/elements-shared/GridBasedPattern.h"

class LOLZ_pattern : public GridBasedPattern_Pattern<9,9>
{
public:
	LOLZ_pattern(SimulationSharedData *sd, int t) :
			GridBasedPattern_Pattern(sd,t)
	{
		bool pattern[9][9] = {
			{0,0,0,0,0,0,0,0,0},
			{1,0,0,0,0,0,1,0,0},
			{1,0,0,0,0,0,1,0,0},
			{1,0,0,1,1,0,0,1,0},
			{1,0,1,0,0,1,0,1,0},
			{1,0,1,0,0,1,0,1,0},
			{0,1,0,1,1,0,0,1,0},
			{0,1,0,0,0,0,0,1,0},
			{0,1,0,0,0,0,0,1,0},
		};
		setPattern(pattern);
	}
};

void LOLZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_LOLZ";
	elem->ui->Name = "LOLZ";
	elem->Colour = COLPACK(0x569212);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_CRACKER2;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 373.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->ui->Description = "Lolz";

	elem->State = ST_GAS;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Graphics = NULL;
	simSD->elemData_create<LOLZ_pattern>(t);
	elem->Func_SimInit = &SimInit_createElemData< GridBasedPattern_ElemDataSim<9,9, 9,CELL,9,CELL> >;
}


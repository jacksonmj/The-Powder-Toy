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
#include "simulation/elements/FILT.h"
#include "simulation/elements-shared/Element_UI_ctypeWavelengths.h"
#include <sstream>

class Element_UI_FILT : public Element_UI_ctypeWavelengths
{
public:
	Element_UI_FILT(Element *el)
		: Element_UI_ctypeWavelengths(el)
	{}

	std::string getModeName(int tmpMode)
	{
		const char* filtModes[] = {"set colour", "AND", "OR", "subtract colour", "red shift", "blue shift", "no effect", "XOR", "NOT", "old QRTZ scattering"};
		if (tmpMode>=0 && tmpMode<=9)
			return filtModes[tmpMode];
		else
			return "unknown mode";
	}

	std::string getHUDText(Simulation *sim, int i, bool debugMode)
	{
		std::stringstream ss;
		ss << Name << " (" << getModeName(sim->parts[i].tmp);
		if (sim->parts[i].ctype)
			ss << ", " << sim->parts[i].ctype;
		ss << ")";
		return ss.str();
	}
};

int FILT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x, ctype = Element_FILT::getWavelengths(cpart);
	*colg = 0;
	*colb = 0;
	*colr = 0;
	for (x=0; x<12; x++) {
		*colr += (ctype >> (x+18)) & 1;
		*colb += (ctype >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (ctype >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	if (cpart->life>0 && cpart->life<=4)
		*cola = 127+cpart->life*30;
	else
		*cola = 127;
	*colr *= x;
	*colg *= x;
	*colb *= x;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND;
	return 0;
}

void FILT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI_FILT>();

	elem->Identifier = "DEFAULT_PT_FILT";
	elem->ui->Name = "FILT";
	elem->Colour = COLPACK(0x000056);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Filter for photons, changes the color.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID | PROP_NOAMBHEAT | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &FILT_graphics;
}


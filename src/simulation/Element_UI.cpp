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

#include "simulation/Element_UI.h"
#include "simulation/Element.h"
#include "simulation/Simulation.h"

#include "powdergraphics.h"

#include <algorithm>

Element_UI::Element_UI(Element *el) :
	elem(el),
	Name(""),
	Description(""),
	MenuVisible(0),
	MenuSection(0)
{}

std::string Element_UI::getHUDText(Simulation *sim, int i, bool debugMode)
{
	if (debugMode)
	{
		int ctype = sim->parts[i].ctype;
		if (sim->IsValidElement(ctype))
			return Name + " (" + sim->elements[ctype].ui->Name + ")";
		else
			return Name + " ()";
	}
	else
	{
		return Name;
	}
}

std::string Element_UI::getLowercaseName()
{
	std::string lowername = Name;
	std::transform(lowername.begin(), lowername.end(), lowername.begin(), ::tolower);
	return lowername;
}

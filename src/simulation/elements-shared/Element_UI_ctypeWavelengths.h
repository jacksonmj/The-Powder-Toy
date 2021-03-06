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

#ifndef Simulation_ElementUI_ctypeWavelengths_h
#define Simulation_ElementUI_ctypeWavelengths_h

#include "simulation/Element_UI.h"

class Element_UI_ctypeWavelengths : public Element_UI
{
public:
	Element_UI_ctypeWavelengths(Element *el) : Element_UI(el) {}
	std::string getHUDText(Simulation *sim, int i, bool debugMode);
};

#endif

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

#include "simulation/elements-shared/Element_UI_ctypeWavelengths.h"
#include "simulation/Simulation.h"
#include <sstream>

std::string Element_UI_ctypeWavelengths::getHUDText(Simulation *sim, int i, bool debugMode)
{
	if (debugMode)
	{
		std::stringstream ss;
		ss << Name << " (" << (sim->parts[i].ctype&0x3FFFFFFF) << ")";
		return ss.str();
	}
	return Element_UI::getHUDText(sim, i, debugMode);
}

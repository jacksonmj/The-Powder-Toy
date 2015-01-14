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

// This header is included in every single element file

#ifndef ELEMENTS_COMMON_H
#define ELEMENTS_COMMON_H

#include <cmath>
#include "common/tptmath.h"
#include "simulation/ElementNumbers.h"
#include "simulation/Element.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationSharedData.h"

template<class ElemDataClass_T>
void SimInit_createElemData(ELEMENT_SIMINIT_FUNC_ARGS)
{
	sim->elemData_create<ElemDataClass_T>(t);
}

#include "powder.h"
#include "gravity.h"
#include "powdergraphics.h"

#endif

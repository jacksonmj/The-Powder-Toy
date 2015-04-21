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

#include "simulation/SimulationSharedData.h"
#include "simulation/Simulation.h"
#include <exception>
#include <algorithm>

// Declare the element initialisation functions
#define ElementNumbers_Include_Decl
#define DEFINE_ELEMENT(name, id) void name ## _init_element(ELEMENT_INIT_FUNC_ARGS);
#include "simulation/ElementNumbers.h"


SimulationSharedData::SimulationSharedData()
{
	std::fill_n(elemDataShared_, PT_NUM, nullptr);
	if (pthread_rwlock_init(&data_rwlock, NULL))
		throw std::runtime_error("Initialising SimulationSharedData.data_rwlock failed");
	InitElements();
}

SimulationSharedData::~SimulationSharedData()
{
	pthread_rwlock_destroy(&data_rwlock);
	for (size_t t=0; t<PT_NUM; t++)
	{
		delete elemDataShared_[t];
	}
}

void SimulationSharedData::InitElements()
{
	#define DEFINE_ELEMENT(name, id) if (id>=0 && id<PT_NUM) { name ## _init_element(this, &elements[id], id); };
	#define ElementNumbers_Include_Call
	#include "simulation/ElementNumbers.h"
}

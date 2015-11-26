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

#ifndef simulation_elements_LIFE_H
#define simulation_elements_LIFE_H

#include "../ElemDataShared.h"
#include "../ElemDataSim.h"
#include "../Simulation.h"
#include "LIFE_Rule.hpp"

#include "common/Observer.h"

#include <vector>

namespace Element_LIFE
{

void setType(SimulationSharedData *simSD, particle &p, int type);
void setType(Simulation *sim, particle &p, int type);

bool isValidType(SimulationSharedData *simSD, int type);
bool isValidType(Simulation *sim, int type);

}

extern std::vector<int> const oldgolTypes;

class LIFE_ElemDataShared : public ElemDataShared
{
public:
	std::vector<LIFE_Rule> rules;
	LIFE_ElemDataShared(SimulationSharedData *s, int t);
	void initDefaultRules();
};

class LIFE_ElemDataSim : public ElemDataSim
{
protected:
	Observer_ClassMember<LIFE_ElemDataSim> obs_simCleared;
	Observer_ClassMember<LIFE_ElemDataSim> obs_simBeforeUpdate;
	unsigned char ruleMap[YRES][XRES];
	unsigned char neighbourTotalMap[YRES][XRES];
	unsigned short neighbourMap[YRES][XRES][6];
	unsigned int speedCounter;
	void readLife();
	void updateLife();
public:
	unsigned int speed;
	unsigned int generation;
	void Simulation_Cleared();
	void Simulation_BeforeUpdate();
	LIFE_ElemDataSim(Simulation *s, int t);
};

#endif

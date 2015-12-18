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

#ifndef simulation_elements_EMP_h
#define simulation_elements_EMP_h

#include "../ElemDataSim.h"
#include "../Simulation.h"
#include "common/Observer.h"

class EMP_ElemDataSim : public ElemDataSim
{
protected:
	Observer_ClassMember<EMP_ElemDataSim> obs_simCleared;
	Observer_ClassMember<EMP_ElemDataSim> obs_simAfterUpdate;
	int flashStrength;
public:
	int triggerCount;
	int getFlashStrength();
	void Simulation_Cleared();
	void Simulation_AfterUpdate();
	EMP_ElemDataSim(Simulation *s, int t);
};

#endif

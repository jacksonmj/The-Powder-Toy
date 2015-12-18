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

#ifndef simulation_elements_ETRD_h
#define simulation_elements_ETRD_h

#include "../ElemDataShared.h"
#include "../ElemDataSim.h"
#include "../Simulation.h"
#include "common/Observer.h"
#include <vector>

class ETRD_deltaWithLength
{
public:
	SimPosDI d;
	int length;
};

class ETRD_ElemDataShared : public ElemDataShared
{
public:
	std::vector<ETRD_deltaWithLength> deltaPos;
	int maxLength;
	ETRD_ElemDataShared(SimulationSharedData *s, int t);
	void initDeltaPos(int maxLength_);
};

class ETRD_ElemDataSim : public ElemDataSim
{
protected:
	Observer_ClassMember<ETRD_ElemDataSim> obs_simCleared;
	Observer_ClassMember<ETRD_ElemDataSim> obs_simBeforeUpdate;
	Observer_ClassMember<ETRD_ElemDataSim> obs_simAfterUpdate;
public:
	bool isValid;
	int countLife0;
	void invalidate();
	ETRD_ElemDataSim(Simulation *s, int t);
};

namespace Element_ETRD
{
int nearestSparkablePart(Simulation *sim, int targetId);
}

#endif

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

#ifndef SimulationSharedData_h
#define SimulationSharedData_h

#include "Config.h"
#include <pthread.h>
#include <vector>
#include <memory>
#include "simulation/Element.h"
#include "simulation/ElementNumbers.h"
#include "simulation/ElemDataShared.h"

class SimulationSharedData
{
public:
	ElemDataShared *elemDataShared_[PT_NUM];
	pthread_rwlock_t data_rwlock;
	Element elements[PT_NUM];

	void InitElements();
	SimulationSharedData();
	virtual ~SimulationSharedData();

	template <class ElemDataClass_T>
	ElemDataClass_T * elemData(int elementId) {
		return static_cast<ElemDataClass_T*>(elemDataShared_[elementId]);
	}

	ElemDataShared * elemData(int elementId) { return elemDataShared_[elementId]; }

	template<class ElemDataClass_T, typename... Args>
	void elemData_create(int elementId, Args&&... args) {
		delete elemDataShared_[elementId];
		elemDataShared_[elementId] = new ElemDataClass_T(this, elementId, args...);
	}

	class DataReadLock
	{
	protected:
		std::shared_ptr<SimulationSharedData> simSD;
	public:
		DataReadLock(std::shared_ptr<SimulationSharedData> sd) : simSD(sd) {
			pthread_rwlock_rdlock(&simSD->data_rwlock);
		}
		~DataReadLock() {
			pthread_rwlock_unlock(&simSD->data_rwlock);
		}
	};
	class DataWriteLock
	{
	protected:
		std::shared_ptr<SimulationSharedData> simSD;
	public:
		DataWriteLock(std::shared_ptr<SimulationSharedData> sd) : simSD(sd) {
			pthread_rwlock_wrlock(&simSD->data_rwlock);
		}
		~DataWriteLock() {
			pthread_rwlock_unlock(&simSD->data_rwlock);
		}
	};
};

#endif

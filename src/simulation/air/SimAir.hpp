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

#ifndef Simulation_Air_SimAir_h
#define Simulation_Air_SimAir_h

#include "simulation/CellsData.hpp"
#include "simulation/air/AirManipulator.hpp"
#include "simulation/air/AirSimulator.hpp"
#include "simulation/air/AirData.hpp"
#include "simulation/walls/WallsData.hpp"

class Simulation;

class SimAir : public AirManipulators<AirManipulator_immediate>
{
protected:
	CellsUChar data_blockair, data_blockairh;

	// previous data (copy of output from last air sim step, before any changes by particles)
	AirData prevData;
	// Before particle sim, this is the same as prevData. After particle sim, this is prevData with changes applied from particles.
	AirData currentData;
	// Destination for AirSimulator output
	AirData simOutput1, simOutput2;

	WallsData wallsData_copy;

	Simulation *sim;
	std::unique_ptr<AirSimulator_async> airSimulator;
	std::shared_future<void> airSimFuture;

	bool airSimNeeded;
public:
	void discardSimResults();
	void getSimResults();
	void startSimIfNeeded();
	void markAsDirty();
	void applyChanges();
	void simulate_sync();
	unsigned char block(SimCellCoord c) const {
		return data_blockair[c.y][c.x];
	}
	void block(SimCellCoord c, bool newState) {
		data_blockair[c.y][c.x] = newState;
	}
	unsigned char blockh(SimCellCoord c) const {
		return data_blockairh[c.y][c.x];
	}
	void blockh(SimCellCoord c, bool newState) {
		data_blockairh[c.y][c.x] = (newState?8:0);
	}
	void blockh_inc(SimCellCoord c) {
		if (data_blockairh[c.y][c.x]<8)
			data_blockairh[c.y][c.x]++;
	}
	// Set to all zeros
	void block_clear();
	void blockh_clear();
	// Calculate initial values for block and blockh based on sim walls
	void initBlockingData();

	SimAir(Simulation *sim_);
	virtual ~SimAir();

	void getData(AirDataP destination);
	void setData(const_AirDataP newData);
	void clear();
	void clearVelocity();
	void clearPressure();
	void clearHeat();
	void invert();

};

#endif

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

#ifndef Simulation_Air_AirSimulator_h
#define Simulation_Air_AirSimulator_h

#include "simulation/CellsData.hpp"
#include "simulation/air/AirData.hpp"

#include <future>
#include <vector>
#include <memory>

// Classes which contain the data pointers and settings for air simulation
// Note: inputData, outputData, and prevData must point to different (and not overlapping) data locations in memory
class AirSimulator_params_base
{
public:
	int airMode;
	int gravityMode;
	bool ambientHeatEnabled;
	float ambientTemp;

	const_CellsFloatP fvx, fvy;
	const_CellsUCharP bmap;
	const_CellsUCharP blockair;
	const_CellsUCharP blockairh;

	const_AirDataP prevData;// output data from previous air sim run
};

// Use this class if inputData must not be modified
class AirSimulator_params_roInput : public AirSimulator_params_base
{
public:
	const_AirDataP inputData;
	AirDataP outputData;
	std::vector<AirDataP> outputData_copies;
};

// Use this class if inputData is allowed to be modified
class AirSimulator_params_rwInput : public AirSimulator_params_base
{
public:
	AirDataP inputData;
	AirDataP outputData;
	std::vector<AirDataP> outputData_copies;
};



// AirSimulator base classes

// AirSimulator_sync: synchronous simulator, simulate() will return when air simulation has finished
// AirSimulator_async: asynchronous simulator (wraps a synchronous simulator), simulate() returns a shared_future which will be made valid when air simulation has finished


// NB: in the current simulators, AirSimulator_sync::simulate() is not thread safe. It should not be called simultaneously from different threads. To use it from different threads, wrap the AirSimulator_sync inside AirSimulator_async (which will serialise simulation runs, they will be performed one at a time).
class AirSimulator_sync
{
protected:
	float kernel[9];
	static void initKernel(float *kernel);

	// air simulation code should go in these functions in derived classes (must implement sim_impl_rwInput, others are optional)
	virtual void sim_impl_roInput(const_AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);
	virtual void sim_impl_rwInput(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params) = 0;
	void sim_RoToRwInput(const_AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);

	AirSimulator_sync();
public:
	virtual ~AirSimulator_sync();
	// Factory method, returns a new air simulator of the specified version
	static std::unique_ptr<AirSimulator_sync> create(int airSimVersion);
	// Run air sim
	virtual void simulate(AirSimulator_params_roInput params);
	virtual void simulate(AirSimulator_params_rwInput params);
};

class WorkerThreads;

class AirSimulator_async
{
protected:
	std::unique_ptr<AirSimulator_sync> internalSim;
	std::unique_ptr<WorkerThreads> worker;

	AirSimulator_async(int airSimVersion);
	AirSimulator_async(std::unique_ptr<AirSimulator_sync> airSim);
public:
	virtual ~AirSimulator_async();
	// Factory methods, returns a new air simulator of the specified version, or wraps an existing synchronous air simulator
	static std::unique_ptr<AirSimulator_async> create(int airSimVersion);
	static std::unique_ptr<AirSimulator_async> create(std::unique_ptr<AirSimulator_sync> airSim);
	// Run air sim
	// The returned future will be made valid when air simulation has finished
	virtual std::shared_future<void> simulate(AirSimulator_params_roInput params);
	virtual std::shared_future<void> simulate(AirSimulator_params_rwInput params);
};


#endif

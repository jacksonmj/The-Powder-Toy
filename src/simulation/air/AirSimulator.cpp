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


#include "simulation/CellsData.hpp"
#include "simulation/air/AirSimulator.hpp"
#include "common/tptmath.h"
#include <cmath>
#include <algorithm>
#include <utility>
#include "common/Threading.hpp"

#include "simulation/air/AirSimulator_v0.hpp"
#include "simulation/air/AirSimulator_v1.hpp"


std::unique_ptr<AirSimulator_sync> AirSimulator_sync::create(int airSimVersion)
{
	switch (airSimVersion)
	{
	case 1:
	default:
		return std::unique_ptr<AirSimulator_sync>(new AirSimulator_v1());
	case 0:
		return std::unique_ptr<AirSimulator_sync>(new AirSimulator_v0());
	}
}

AirSimulator_sync::AirSimulator_sync()
{
	initKernel(kernel);
}
AirSimulator_sync::~AirSimulator_sync()
{}

void AirSimulator_sync::simulate(AirSimulator_params_roInput params)
{
	sim_impl_roInput(params.inputData, params.outputData, params);
	for (auto it=params.outputData_copies.begin(); it!=params.outputData_copies.end(); ++it)
		AirData_copy(params.outputData, *it);
}

void AirSimulator_sync::simulate(AirSimulator_params_rwInput params)
{
	sim_impl_rwInput(params.inputData, params.outputData, params);
	for (auto it=params.outputData_copies.begin(); it!=params.outputData_copies.end(); ++it)
		AirData_copy(params.outputData, *it);
}

void AirSimulator_sync::initKernel(float *kernel)
{
	// Gaussian kernel, normalised to give total = 1
	int i, j;
	float s = 0.0f;
	for (j=-1; j<2; j++)
		for (i=-1; i<2; i++)
		{
			kernel[(i+1)+3*(j+1)] = expf(-2.0f*(i*i+j*j));
			s += kernel[(i+1)+3*(j+1)];
		}
	s = 1.0f / s;
	for (j=-1; j<2; j++)
		for (i=-1; i<2; i++)
			kernel[(i+1)+3*(j+1)] *= s;
}

void AirSimulator_sync::sim_impl_roInput(const_AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	sim_RoToRwInput(inputData, outputData, params);
}

void AirSimulator_sync::sim_RoToRwInput(const_AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	AirData tmpInput;
	AirData_copy(inputData, tmpInput);
	sim_impl_rwInput(tmpInput, outputData, params);
}


AirSimulator_async::AirSimulator_async(int airSimVersion) :
	AirSimulator_async(AirSimulator_sync::create(airSimVersion))
{}

AirSimulator_async::AirSimulator_async(std::unique_ptr<AirSimulator_sync> airSim) :
	internalSim(std::move(airSim)), worker(new WorkerThreads(1))
{}

std::unique_ptr<AirSimulator_async> AirSimulator_async::create(int airSimVersion) {
	return std::unique_ptr<AirSimulator_async>(new AirSimulator_async(airSimVersion));
}

std::unique_ptr<AirSimulator_async> AirSimulator_async::create(std::unique_ptr<AirSimulator_sync> airSim) {
	return std::unique_ptr<AirSimulator_async>(new AirSimulator_async(std::move(airSim)));
}

AirSimulator_async::~AirSimulator_async()
{}

std::shared_future<void> AirSimulator_async::simulate(AirSimulator_params_roInput params)
{
	return worker->asyncTask([this](AirSimulator_params_roInput params){
		internalSim->simulate(params);
	}, params);
}

std::shared_future<void> AirSimulator_async::simulate(AirSimulator_params_rwInput params)
{
	return worker->asyncTask([this](AirSimulator_params_rwInput params){
		internalSim->simulate(params);
	}, params);
}


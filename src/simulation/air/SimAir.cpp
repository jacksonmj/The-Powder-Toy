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
#include "simulation/air/SimAir.hpp"
#include "simulation/Simulation.h"
#include "common/tptmath.h"
#include <cmath>
#include <algorithm>
#include <utility>

#include "powder.h" //global vars emap, bmap

SimAir::SimAir(Simulation *sim_) : sim(sim_), airSimNeeded(false)
{
	airSimulator = AirSimulator_async::create(1);
	setManipData(currentData);
	block_clear();
	blockh_clear();
}

SimAir::~SimAir() {}

void SimAir::discardSimResults()
{
	if (airSimFuture.valid())
	{
		airSimFuture.get();
		airSimFuture = std::shared_future<void>();
		airSimNeeded = true;
	}
}

void SimAir::getSimResults()
{
	if (airSimNeeded)
		startSimIfNeeded();
	if (airSimFuture.valid())
	{
		airSimFuture.get();
		swap(prevData, simOutput1);
		swap(currentData, simOutput2);
		setManipData(currentData);
	}
}

void SimAir::markAsDirty()
{
	airSimNeeded = true;
}

void SimAir::applyChanges()
{
	pv.getNewData_maybeSwap(currentData.pv);
	vx.getNewData_maybeSwap(currentData.vx);
	vy.getNewData_maybeSwap(currentData.vy);
	hv.getNewData_maybeSwap(currentData.hv);
	setManipData(currentData);
	pv.clearBuffer();
	vx.clearBuffer();
	vy.clearBuffer();
	hv.clearBuffer();
	airSimNeeded = true;
}

void SimAir::startSimIfNeeded()
{
	if (!airSimNeeded)
		return;
	discardSimResults();
	AirSimulator_params_roInput params;
	params.airMode = sim->airMode;
	params.ambientHeatEnabled = sim->ambientHeatEnabled;
	params.ambientTemp = sim->ambientTemp;
	params.blockair = data_blockair;
	params.blockairh = data_blockairh;
	params.bmap = bmap;
	params.fvx = fvx;
	params.fvy = fvy;
	params.gravityMode = gravityMode;
	params.prevData = prevData;
	params.inputData = currentData;
	params.outputData = simOutput1;
	params.outputData_copies.push_back(simOutput2);
	airSimFuture = airSimulator->simulate(params);
	airSimNeeded = false;
}

void SimAir::simulate_sync()
{
	airSimNeeded = true;
	startSimIfNeeded();
	getSimResults();
}

void SimAir::block_clear()
{
	CellsData_fill<unsigned char>(data_blockair, 0);
}

void SimAir::blockh_clear()
{
	CellsData_fill<unsigned char>(data_blockairh, 0);
}

void SimAir::initBlockingData()
{
	for (int y=0; y<YRES/CELL; y++)
	{
		for (int x=0; x<XRES/CELL; x++)
		{
			data_blockair[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_BLOCKAIR || (bmap[y][x]==WL_EWALL && !emap[y][x]));
			data_blockairh[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_BLOCKAIR || (bmap[y][x]==WL_EWALL && !emap[y][x]) || bmap[y][x]==WL_GRAV) ? 0x8:0;
		}
	}
}

void SimAir::getData(AirDataP destination)
{
	AirData_copy(currentData, destination);
}
void SimAir::setData(const_AirDataP newData)
{
	discardSimResults();
	AirData_copy(newData, prevData);
	AirData_copy(newData, currentData);
}

void SimAir::invert()
{
	discardSimResults();
	CellsData_invert<float>(prevData.vx);
	CellsData_invert<float>(prevData.vy);
	CellsData_invert<float>(prevData.pv);
	CellsData_invert<float>(currentData.vx);
	CellsData_invert<float>(currentData.vy);
	CellsData_invert<float>(currentData.pv);
	setManipData(currentData);
}

void SimAir::clear()
{
	clearVelocity();
	clearPressure();
	clearHeat();
}

void SimAir::clearVelocity()
{
	discardSimResults();
	CellsData_fill<float>(prevData.vx, 0.0f);
	CellsData_fill<float>(prevData.vy, 0.0f);
	CellsData_fill<float>(currentData.vx, 0.0f);
	CellsData_fill<float>(currentData.vy, 0.0f);
	vx.clearBuffer();
	vy.clearBuffer();
	vx.setData(currentData.vx);
	vy.setData(currentData.vy);
}
void SimAir::clearPressure()
{
	discardSimResults();
	CellsData_fill<float>(prevData.pv, 0.0f);
	CellsData_fill<float>(currentData.pv, 0.0f);
	pv.clearBuffer();
	pv.setData(currentData.pv);
}

void SimAir::clearHeat()
{
	discardSimResults();
	CellsData_fill<float>(prevData.hv, sim->ambientTemp);
	CellsData_fill<float>(currentData.hv, sim->ambientTemp);
	hv.clearBuffer();
	hv.setData(currentData.hv);
}




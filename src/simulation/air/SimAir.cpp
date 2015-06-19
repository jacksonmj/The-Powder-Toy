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
#include "common/Intrinsics.hpp"
#include "algorithm/count_if.hpp"
#include "algorithm/for_each_i.hpp"
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
	WallsData_copy(sim->walls.getDataPtr(), wallsData_copy);
	params.wallsData = wallsData_copy;
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


// Check for walls which block air and set blockair and blockairh accordingly
class Kernel_initBlockingData : public tptalgo::Kernel_base
{
public:
	TPTALGO_KERN_I_COMMON(Kernel_initBlockingData)

	const uint8_t * const bmap, * const emap;
	unsigned char * const blockair, * const blockairh;

	bool isAligned(size_t alignAmount, size_t i=0) const
	{
		return (
			getAlignOffset_prev(bmap+i, alignAmount)==0 &&
			getAlignOffset_prev(emap+i, alignAmount)==0 &&
			getAlignOffset_prev(blockair+i, alignAmount)==0 &&
			getAlignOffset_prev(blockairh+i, alignAmount)==0
		);
	}
	bool canAlign(size_t alignAmount) const
	{
		return (
			getAlignOffset_diff(bmap, emap, alignAmount)==0 &&
			getAlignOffset_diff(bmap, blockair, alignAmount)==0 &&
			getAlignOffset_diff(bmap, blockairh, alignAmount)==0
		);
	}

	Kernel_initBlockingData(const_WallsDataP wallsData, CellsUCharRP blockair_, CellsUCharRP blockairh_) :
		bmap(reinterpret_cast<const unsigned char *>(wallsData.wallType)),
		emap(reinterpret_cast<const unsigned char *>(wallsData.electricity)),
		blockair(reinterpret_cast<unsigned char *>(blockair_)),
		blockairh(reinterpret_cast<unsigned char *>(blockairh_))
	{}

	// Basic implementation, checking one cell at a time
	TPTALGO_KERN_I_IMPL(1, 1)
	{
	public:
		i_impl(...) {}
		void i_op(const Kernel &k, size_t i, bool isAligned) const
		{
			bool blockAll = k.bmap[i] && (
				k.bmap[i]==WL_WALL ||
				k.bmap[i]==WL_WALLELEC ||
				k.bmap[i]==WL_BLOCKAIR ||
				(k.bmap[i]==WL_EWALL && !k.emap[i])
			);
			k.blockair[i] = blockAll;
			k.blockairh[i] = (blockAll || (k.bmap[i]==WL_GRAV)) ? 0x8:0;
		}
	};

	// Much faster when not many walls are present, checks many cells at a time against 0 and skips full comparison if they are all empty.
	TPTALGO_KERN_I_IMPL(sizeof(uint_fast32_t)/sizeof(unsigned char), 2)
	{
	public:
		using T = uint_fast32_t;
		const typename Kernel::template i_impl<1> subImpl;
		i_impl(const Kernel &kernel) : subImpl(kernel) {}
		void i_op(const Kernel &k, size_t i, bool isAligned) const
		{
			T bmapCells = *reinterpret_cast<const T*>(k.bmap+i);
			*reinterpret_cast<T*>(k.blockair+i) = 0;
			*reinterpret_cast<T*>(k.blockairh+i) = 0;
			if (bmapCells!=0)
			{
				for (size_t j=0; j<this->step_items; j++)
					subImpl.i_op(k, i+j, false);
			}
		}

		bool isAligned(const Kernel &kernel, size_t i) const { return kernel.isAligned(sizeof(uint_fast32_t), i); }
		bool canAlign(const Kernel &kernel) const { return kernel.canAlign(sizeof(uint_fast32_t)); }
	};

#if HAVE_LIBSIMDPP
	// SIMD implementation
	// May be slower than uint_fast32_t when no walls are present, but runs in constant time (so will be faster when enough walls are placed).
	TPTALGO_KERN_I_IMPL(simdpp::uint8v::length, 3)
	{
	public:
		using T = simdpp::uint8v;
		const T vec_WL_WALL, vec_WL_WALLELEC, vec_WL_BLOCKAIR, vec_WL_EWALL, vec_WL_GRAV, vec_1;

		i_impl(const Kernel &kernel) : 
			vec_WL_WALL		(simdpp::splat(WL_WALL)),
			vec_WL_WALLELEC	(simdpp::splat(WL_WALLELEC)),
			vec_WL_BLOCKAIR	(simdpp::splat(WL_BLOCKAIR)),
			vec_WL_EWALL	(simdpp::splat(WL_EWALL)),
			vec_WL_GRAV		(simdpp::splat(WL_GRAV)),
			vec_1			(simdpp::splat(1))
		{}

		void i_op(const Kernel &k, size_t i, bool isAligned) const
		{
			// Load cell data
			T bmapCells, emapCells;
			if (isAligned && 0)
			{
				bmapCells = simdpp::load(k.bmap+i);
				emapCells = simdpp::load(k.emap+i);
			}
			else
			{
				bmapCells = simdpp::load_u(k.bmap+i);
				emapCells = simdpp::load_u(k.emap+i);
			}

			// Do comparison
			T blockAll = T(
				(bmapCells==vec_WL_WALL) |
				(bmapCells==vec_WL_WALLELEC) |
				(bmapCells==vec_WL_BLOCKAIR) |
				((bmapCells==vec_WL_EWALL) & (emapCells!=T::zero()))
			);
			// simd true is 0xff in each byte, turn it into 0x1 for blockair and 0x8 for blockairh
			T blockAirVec = blockAll & vec_1;
			T blockAirHVec = (blockAll | (bmapCells==vec_WL_GRAV)) & simdpp::shift_l<3>(vec_1);

			// Store blockair data
			if (isAligned)
			{
				simdpp::store(k.blockair+i, blockAirVec);
				simdpp::store(k.blockairh+i, blockAirHVec);
			}
			else
			{
				simdpp::store_u(k.blockair+i, blockAirVec);
				simdpp::store_u(k.blockairh+i, blockAirHVec);
			}
		}

		bool isAligned(const Kernel &kernel, size_t i) const { return kernel.isAligned(T::length_bytes, i); }
		bool canAlign(const Kernel &kernel) const { return kernel.canAlign(T::length_bytes); }
	};
#endif
};

void SimAir::initBlockingData()
{
	Kernel_initBlockingData kernel(sim->walls.getDataPtr(), data_blockair, data_blockairh);
	tptalgo::for_each_i(kernel, (XRES/CELL)*(YRES/CELL));
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




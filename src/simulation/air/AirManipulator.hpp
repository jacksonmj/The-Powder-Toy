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

#ifndef Simulation_Air_AirManipulator_h
#define Simulation_Air_AirManipulator_h

/*
Classes which provide an interface to an air data array, with methods for retrieving values and modifying them with certain operations.
  AirManipulator_immediate - all changes are visible immediately.
  AirManipulator_buffered - changes are buffered and do not take effect until getNewData() is called to retrieve a copy of the data with changes applied.


Example:

CellFloats pvdata;
AirManipulator_immediate pv(pvdata);

pv.add(SimPosI(x,y), 10.0f); // adds 10 pressure to cell (x/CELL,y/CELL)
pv.add(SimPosF(x,y), 10.0f); // adds 10 pressure to cell (x/CELL,y/CELL), x and y are floats
pv.add(SimPosCell(x,y), 10.0f); // adds 10 pressure to cell (x,y)

*/

#include "defines.h"

#include "simulation/CellsData.hpp"
#include "simulation/air/AirData.hpp"
#include <cmath>

class AirManipulator
{
public:
	AirManipulator();
	virtual ~AirManipulator();
	virtual void clearBuffer() = 0;
	virtual const_CellsFloatP getDataPtr() = 0;
	virtual void getNewData_maybeSwap(CellsFloat &dest) = 0;
	virtual void getNewData(CellsFloatP dest) = 0;
	virtual void setData(const_CellsFloatP origData) = 0;
};

class AirManipulator_immediate : public AirManipulator
{
private:
	const_CellsFloatP origData;
	CellsFloat newData;
public:
	const_CellsFloatP getDataPtr() { return newData; }
	void clearBuffer();
	void getNewData_maybeSwap(CellsFloat &dest);
	void getNewData(CellsFloatP dest);

	void setData(const_CellsFloatP origData_);
	AirManipulator_immediate(const_CellsFloatP origData=nullptr);
	virtual ~AirManipulator_immediate();

	float get(SimPosCell c) const {
		return newData[c.y][c.x];
	}
	float operator() (SimPosCell c) const {
		return newData[c.y][c.x];
	}

	void add(SimPosCell c, float amount) {
		newData[c.y][c.x] += amount;
	}

	void multiply(SimPosCell c, float multiplier) {
		newData[c.y][c.x] *= multiplier;
	}

	void blend(SimPosCell c, float value, float weight) {
		// weight = 0.0f: no change in value
		// weight -> infinity: equivalent to setting value
		newData[c.y][c.x] = (newData[c.y][c.x] + weight*value)/(1.0f + weight);
	}
	void blend_legacy(SimPosCell c, float value, float weight) {
		// "legacy" because in AirManipulator_buffered it requires additional calculation
		// weight = 0.0f: no change in value
		// weight = 1.0f: equivalent to setting value
		// weight should not be greater than 1.0
		// Used for compatibility with TPT++ (many of the air interactions are of exactly this form)
		newData[c.y][c.x] += weight*(value-newData[c.y][c.x]);
	}

	void set(SimPosCell c, float value) {
		newData[c.y][c.x] = value;
	}
};

class AirManipulator_buffered : public AirManipulator
{
private:
	const_CellsFloatP origData;
	CellsFloat buf_add;
	CellsFloat buf_multiply;
	CellsFloat buf_blendWeight;
	CellsFloat buf_blendValue;

	static void applyBuffer_1d(const float *srcData, float *destData, float *buf_add, float *buf_multiply, float *buf_blendWeight, float *buf_blendValue);

public:
	const_CellsFloatP getDataPtr() { return origData; }
	void clearBuffer();
	void getNewData_maybeSwap(CellsFloat &dest);
	void getNewData(CellsFloatP dest);

	// TODO: void mergeBufferFrom(AirManipulator_buffered &src) - function to combine buffered changes, so that changes can be made simultaneously by different threads then combined and applied.

	void setData(const_CellsFloatP data_) {
		origData = data_;
	}
	AirManipulator_buffered(const_CellsFloatP data_=nullptr) {
		setData(data_);
	}
	virtual ~AirManipulator_buffered() {}

	float get(SimPosCell c) const {
		return origData[c.y][c.x];
	}
	float operator() (SimPosCell c) const {
		return origData[c.y][c.x];
	}
	void add(SimPosCell c, float amount) {
		buf_add[c.y][c.x] += amount;
	}
	void multiply(SimPosCell c, float multiplier) {
		buf_multiply[c.y][c.x] *= multiplier;
	}
	void blend(SimPosCell c, float value, float weight) {
		buf_blendWeight[c.y][c.x] += weight;
		buf_blendValue[c.y][c.x] += weight*value;
	}
	void blend_legacy(SimPosCell c, float value, float weight) {
		float newWeight = (std::abs(1.0f-weight)<1e-3) ? 1e5 : weight/(1-weight);
		blend(c, value, newWeight);
	}
	void set(SimPosCell c, float value) {
		blend(c, value, 1e5);
	}
};

template<class AirManipT>
class AirManipulators
{
public:
	AirManipT vx, vy, pv;
	AirManipulator_immediate hv;
	SimPosDF vel(SimPosCell c)
	{
		return SimPosDF(vx(c), vy(c));
	}
	void setManipData(AirDataP data)
	{
		vx.setData(data.vx);
		vy.setData(data.vy);
		pv.setData(data.pv);
		hv.setData(data.hv);
	}
	AirManipulators() {}
	AirManipulators(CellsFloatP data_vx, CellsFloatP data_vy, CellsFloatP data_pv, CellsFloatP data_hv) : vx(data_vx), vy(data_vy), pv(data_pv), hv(data_hv) {}
	AirManipulators(AirDataP data)
	{
		setManipData(data);
	}
	virtual ~AirManipulators() {}
};


#endif

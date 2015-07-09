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

#ifndef Simulation_CellsData_h
#define Simulation_CellsData_h

// Note: if adding a new data type array, lines will probably need adding here ("CELLSDATA_TYPEDEFS" and at the bottom "typedef CellsData<...> Cells...;") and in CellsData.cpp (at the bottom, "CELLSDATA_INSTANTIATE")

#include "common/Compat.hpp"
#include "simulation/Position.hpp"
#include "common/AlignedAlloc.hpp"
#include "common/tpt-stdint.h"
#include <utility>
#include <cstddef>
using std::swap;

#define CELLSDATA_TYPEDEFS(DataType, BaseName) \
typedef DataType (* BaseName##P)[XRES/CELL];\
typedef DataType (* TPT_RESTRICT BaseName##RP)[XRES/CELL];\
typedef const DataType (* const_##BaseName##P)[XRES/CELL];\
typedef const DataType (* TPT_RESTRICT const_##BaseName##RP)[XRES/CELL];

CELLSDATA_TYPEDEFS(float, CellsFloat)
CELLSDATA_TYPEDEFS(uint8_t, CellsUChar)
CELLSDATA_TYPEDEFS(uint8_t, CellsUint8)


void CellsData_limit(CellsFloatP data, float minVal, float maxVal);
void CellsData_limit(const_CellsFloatRP src, CellsFloatRP dest, float minVal, float maxVal);
size_t CellsData_count_and8(const_CellsUint8P src);// Count the number of cells where (x&8)
size_t CellsData_count_1(const_CellsUint8P src);// Count the number of cells where x==1, x can only be 0 or 1.
void CellsData_subtract_sat(const_CellsUint8P src, CellsUint8P dest, uint8_t value);// Saturating subtraction (if result is less than 0, result is set to 0) of a value from all bytes

template<typename DataType>
void CellsData_fill(DataType (*dest)[XRES/CELL], DataType value);

template<typename DataType>
void CellsData_copy(const DataType (* TPT_RESTRICT src)[XRES/CELL], DataType (* TPT_RESTRICT dest)[XRES/CELL]);

template<typename DataType>
void CellsData_invert(const DataType (*src)[XRES/CELL], DataType (*dest)[XRES/CELL]);
template<typename DataType>
void CellsData_invert(DataType (*data)[XRES/CELL]) {
	CellsData_invert(data, data);
}

template<typename DataType>
class CellsData;

template<typename DataType>
void swap(CellsData<DataType> &a, CellsData<DataType> &b);

template<typename DataType>
class CellsData
{
protected:
	DataType (*data)[XRES/CELL];
public:
	using DataType_2d = DataType (*)[XRES/CELL];
	using const_DataType_2d = const DataType (*)[XRES/CELL];

	CellsData() {
		data = reinterpret_cast<DataType_2d>(tpt_aligned_alloc(16, sizeof(DataType)*(YRES/CELL)*(XRES/CELL)));
	}
	~CellsData() {
		tpt_aligned_free(data);
	}

	DataType_2d ptr2d() { return data; }
	const_DataType_2d ptr2d() const { return data; }
	operator DataType_2d() { return ptr2d(); }
	operator const_DataType_2d() const { return ptr2d(); }

	DataType* ptr1d() { return reinterpret_cast<DataType*>(data); }
	const DataType* ptr1d() const { return reinterpret_cast<const DataType*>(data); }
	explicit operator DataType*() { return ptr1d(); }
	explicit operator const DataType*() const { return ptr1d(); }

	DataType& operator ()(int x, int y) { return data[y][x]; }
	const DataType& operator ()(int x, int y) const { return data[y][x]; }
	DataType& operator ()(SimPosCell c) { return data[c.y][c.x]; }
	const DataType& operator ()(SimPosCell c) const { return data[c.y][c.x]; }

	void copyTo(DataType_2d dest) const
	{
		CellsData_copy(ptr2d(), dest);
	}
	CellsData(const DataType (*src)[XRES/CELL]) : CellsData()
	{
		CellsData_copy(src, ptr2d());
	}

	friend void swap<DataType>(CellsData<DataType>& a, CellsData<DataType>& b);
};

template<typename DataType>
void swap(CellsData<DataType> &a, CellsData<DataType> &b) {
	swap(a.data, b.data);
}

using CellsFloat = CellsData<float>;
using CellsUChar = CellsData<uint8_t>;
using CellsUint8 = CellsData<uint8_t>;

#endif

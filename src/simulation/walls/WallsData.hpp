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

#ifndef simulation_walls_WallsData_h
#define simulation_walls_WallsData_h

#include "simulation/CellsData.hpp"

template<class TI, class TF>
class WallsData_items
{
public:
	TI wallType;
	TI electricity;
	TF fanVX, fanVY;
protected:
	void setNull(...) {}
	template <class TI2 = TI, class TF2 = TF> typename std::enable_if<std::is_pointer<TI2>::value && std::is_pointer<TF2>::value, void>::type setNull()
	{
		wallType = electricity = nullptr;
		fanVX = fanVY = nullptr;
	}
public:
	WallsData_items()
	{
		if (std::is_pointer<TI>::value && std::is_pointer<TF>::value)
			setNull();
	}
	template <class SrcT>
	WallsData_items(SrcT&& src)
	{
		wallType = src.wallType;
		electricity = src.electricity;
		fanVX = src.fanVX;
		fanVY = src.fanVY;
	}
};

// pointers / const pointers to wall data
using WallsDataP = WallsData_items<CellsUint8P, CellsFloatP>;
using const_WallsDataP = WallsData_items<const_CellsUint8P, const_CellsFloatP>;

// wall data (allocates memory to store the data)
class WallsData : public WallsData_items<CellsUint8, CellsFloat>
{
public:
	WallsData();
	WallsData(const_WallsDataP src);
	WallsData& operator=(const_WallsDataP src);
};

void WallsData_copy(const_WallsDataP src, WallsDataP dest);
void swap(WallsData& a, WallsData& b);

#endif

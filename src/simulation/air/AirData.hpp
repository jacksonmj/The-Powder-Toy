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

#ifndef simulation_air_AirData_h
#define simulation_air_AirData_h

#include "simulation/CellsData.hpp"
#include <type_traits>

template<class T>
class AirData_items
{
public:
	T vx,vy;
	T pv;
	T hv;
protected:
	void setNull(...) {}
	template <class T2 = T> typename std::enable_if<std::is_pointer<T2>::value, void>::type setNull()
	{
		vx = vy = pv = hv = nullptr;
	}
public:
	AirData_items()
	{
		if (std::is_pointer<T>::value)
			setNull();
	}
	template <class SrcT>
	AirData_items(SrcT&& src)
	{
		vx = src.vx;
		vy = src.vy;
		pv = src.pv;
		hv = src.hv;
	}
};

// pointers / const pointers to air data
using AirDataP = AirData_items<CellsFloatP>;
using const_AirDataP = AirData_items<const_CellsFloatP>;

// air data (allocates memory to store the data)
class AirData : public AirData_items<CellsFloat>
{
public:
	AirData();
	AirData(const_AirDataP src);
	AirData& operator=(const_AirDataP src);
};

void AirData_copy(const_AirDataP src, AirDataP dest);
void swap(AirData& a, AirData& b);

#endif

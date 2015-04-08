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


#include "simulation/air/AirData.hpp"
#include "common/tptmath.h"
#include <cmath>
#include <algorithm>
#include <utility>

#include "common/Threading.hpp"

#define WL_FAN 123
//include "powder.h"

using std::swap;

AirData::AirData() {}

AirData::AirData(const_AirDataP src)
{
	AirData_copy(src, *this);
}

AirData& AirData::operator=(const_AirDataP src)
{
	AirData_copy(src, *this);
	return *this;
}

AirData::operator AirDataP()
{
	AirDataP tmp;
	tmp.vx = vx;
	tmp.vy = vy;
	tmp.pv = pv;
	tmp.hv = hv;
	return tmp;
}

AirData::operator const_AirDataP() const
{
	const_AirDataP tmp;
	tmp.vx = vx;
	tmp.vy = vy;
	tmp.pv = pv;
	tmp.hv = hv;
	return tmp;
}

void swap(AirData& a, AirData& b)
{
	swap(a.vx, b.vx);
	swap(a.vy, b.vy);
	swap(a.pv, b.pv);
	swap(a.hv, b.hv);
}

void AirData_copy(const_AirDataP src, AirDataP dest)
{
	CellsData_copy<float>(src.vx, dest.vx);
	CellsData_copy<float>(src.vy, dest.vy);
	CellsData_copy<float>(src.pv, dest.pv);
	CellsData_copy<float>(src.hv, dest.hv);
}

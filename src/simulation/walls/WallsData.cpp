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


#include "simulation/walls/WallsData.hpp"
#include <algorithm>
#include <utility>

using std::swap;

WallsData::WallsData() {}

WallsData::WallsData(const_WallsDataP src)
{
	WallsData_copy(src, *this);
}

WallsData& WallsData::operator=(const_WallsDataP src)
{
	WallsData_copy(src, *this);
	return *this;
}

WallsData::operator WallsDataP()
{
	WallsDataP tmp;
	tmp.wallType = wallType;
	tmp.electricity = electricity;
	tmp.fanVX = fanVX;
	tmp.fanVY = fanVY;
	return tmp;
}

WallsData::operator const_WallsDataP() const
{
	const_WallsDataP tmp;
	tmp.wallType = wallType;
	tmp.electricity = electricity;
	tmp.fanVX = fanVX;
	tmp.fanVY = fanVY;
	return tmp;
}

void swap(WallsData& a, WallsData& b)
{
	swap(a.wallType, b.wallType);
	swap(a.electricity, b.electricity);
	swap(a.fanVX, b.fanVX);
	swap(a.fanVY, b.fanVY);
}

void WallsData_copy(const_WallsDataP src, WallsDataP dest)
{
	CellsData_copy<uint8_t>(src.wallType, dest.wallType);
	CellsData_copy<uint8_t>(src.electricity, dest.electricity);
	CellsData_copy<float>(src.fanVX, dest.fanVX);
	CellsData_copy<float>(src.fanVY, dest.fanVY);	
}


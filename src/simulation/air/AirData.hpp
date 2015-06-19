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

#ifndef Simulation_Air_AirData_h
#define Simulation_Air_AirData_h

#include "simulation/CellsData.hpp"

// pointers to air data
class AirDataP
{
public:
	CellsFloatP vx,vy;
	CellsFloatP pv;
	CellsFloatP hv;
	AirDataP() : vx(nullptr), vy(nullptr), pv(nullptr), hv(nullptr) {}
};

// pointers to constant air data
class const_AirDataP
{
public:
	const_CellsFloatP vx,vy;
	const_CellsFloatP pv;
	const_CellsFloatP hv;
	const_AirDataP() : vx(nullptr), vy(nullptr), pv(nullptr), hv(nullptr) {}
	const_AirDataP(const AirDataP &p) : vx(p.vx), vy(p.vy), pv(p.pv), hv(p.hv) {}
};

// air data (memory to store the data is allocated by this class)
class AirData
{
public:
	CellsFloat vx,vy;
	CellsFloat pv;
	CellsFloat hv;
	AirData();
	AirData(const_AirDataP src);
	AirData& operator=(const_AirDataP src);
	operator AirDataP();
	operator const_AirDataP() const;
};
void swap(AirData& a, AirData& b);

// copy air data
void AirData_copy(const_AirDataP src, AirDataP dest);

#endif

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

#include "simulation/air/AirManipulator.hpp"
#include "common/tptmath.h"
#include <utility>

using std::swap;


AirManipulator::AirManipulator()
{}
AirManipulator::~AirManipulator()
{}

void AirManipulator_immediate::getNewData_maybeSwap(CellsFloat &dest)
{
	swap(newData, dest);
}

void AirManipulator_immediate::getNewData(CellsFloatP dest)
{
	CellsData_copy<float>(newData, dest);
}

void AirManipulator_immediate::clearBuffer()
{
	if (origData!=nullptr)
		CellsData_copy<float>(origData, newData);
	else
		CellsData_fill<float>(newData, 0.0f);
}

void AirManipulator_immediate::setData(const_CellsFloatP origData_)
{
	origData = origData_;
	clearBuffer();
}

AirManipulator_immediate::AirManipulator_immediate(const_CellsFloatP origData)
{
	setData(origData);
}

AirManipulator_immediate::~AirManipulator_immediate()
{}



void AirManipulator_buffered::getNewData(CellsFloatP dest)
{
	applyBuffer_1d(reinterpret_cast<const float*>(origData), reinterpret_cast<float*>(dest), buf_add.ptr1d(), buf_multiply.ptr1d(), buf_blendWeight.ptr1d(), buf_blendValue.ptr1d());
}

void AirManipulator_buffered::getNewData_maybeSwap(CellsFloat &dest)
{
	getNewData(dest);
}

void AirManipulator_buffered::applyBuffer_1d(const float *srcData, float *destData, float *buf_add, float *buf_multiply, float *buf_blendWeight, float *buf_blendValue)
{
	for (int i=0; i<(XRES/CELL)*(YRES/CELL); i++)
	{
		destData[i] = (srcData[i]*buf_multiply[i]+buf_add[i] + buf_blendValue[i]) / buf_blendWeight[i];
	}
}

void AirManipulator_buffered::clearBuffer()
{
	CellsData_fill<float>(buf_add, 0.0f);
	CellsData_fill<float>(buf_multiply, 1.0f);
	CellsData_fill<float>(buf_blendWeight, 1.0f);
	CellsData_fill<float>(buf_blendValue, 0.0f);
}


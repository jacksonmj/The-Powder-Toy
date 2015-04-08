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
#include <algorithm>
#include "catch.hpp"

TEST_CASE("simulation/CellsData count_and8", "[simulation]")
{
	CellsUChar data;
	unsigned char * p = data.ptr1d();
	size_t cellsCount = (XRES/CELL)*(YRES/CELL);

	SECTION("all are 8")
	{
		CellsData_fill<unsigned char>(data, 8);
		CHECK( CellsData_count_and8(data) == cellsCount );
	}
	SECTION("all lower than 8")
	{
		CellsData_fill<unsigned char>(data, 7);
		CHECK( CellsData_count_and8(data) == 0 );
	}
	SECTION("unaligned data")
	{
		unsigned char *tmp = new unsigned char[cellsCount+16];
		std::fill_n(tmp, cellsCount+16, 8);
		for (int i=0; i<16; i++)
		{
			CHECK( CellsData_count_and8(reinterpret_cast<CellsUCharP>(tmp+i)) == cellsCount );
		}
		delete[] tmp;
	}
	SECTION("with a zero in middle at different positions % 32")
	{
		CellsData_fill<unsigned char>(data, 8);
		for (int i=0; i<32; i++)
		{
			data.ptr1d()[32*10+i] = 0;
			CHECK( CellsData_count_and8(data) == cellsCount-1 );
			data.ptr1d()[32*10+i] = 8;
		}
	}

}

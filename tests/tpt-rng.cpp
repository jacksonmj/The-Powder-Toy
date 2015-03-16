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

#include "common/tpt-rng.h"
#include "catch.hpp"
#include <functional>


TEST_CASE("CountBitsOccupied dynamic", "[tptrng]")
{
	struct CountBitsOccupied_testData { uint_fast32_t x; int bits; };
	std::vector<struct CountBitsOccupied_testData> testData;
	testData.push_back({0,0});
	testData.push_back({1,1});
	testData.push_back({2,2});
	testData.push_back({3,2});
	testData.push_back({4,3});
	testData.push_back({0xFF,8});
	testData.push_back({0x100,9});
	testData.push_back({0xFFFFFFFF,32});

	SECTION("compiler builtin")
	{
		for (auto it=testData.begin(); it<testData.end(); ++it)
			CHECK( RandomStore_auto::countBitsOccupied_dynamic(it->x) == it->bits);
	}
	SECTION("fallback")
	{
		for (auto it=testData.begin(); it<testData.end(); ++it)
			CHECK( RandomStore_auto::countBitsOccupied_dynamic_fallback(it->x) == it->bits);
	}

}

TEST_CASE("CountBitsOccupied template", "[tptrng]")
{
	CHECK( CountBitsOccupied<0>::bits == 0);
	CHECK( CountBitsOccupied<1>::bits == 1);
	CHECK( CountBitsOccupied<2>::bits == 2);
	CHECK( CountBitsOccupied<3>::bits == 2);
	CHECK( CountBitsOccupied<4>::bits == 3);
	CHECK( CountBitsOccupied<0xFF>::bits == 8);
	CHECK( CountBitsOccupied<0x100>::bits == 9);
	CHECK( CountBitsOccupied<0xFFFFFFFF>::bits == 32);
}

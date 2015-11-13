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

#include <vector>
#include "common/tpt-rng.h"
#include "catch.hpp"

#include "simulation/elements-shared/Wavelengths.hpp"


TEST_CASE("Wavelengths popcount", "")
{
	// max number of bits per colour
	const int bitCount = 12;

	class Wavelengths_popcount_test : public ElementsShared::Wavelengths
	{
		unsigned int correctPopcount(unsigned int x)
		{
		#if defined(__POPCNT__) && !defined(_MSC_VER)
			return _mm_popcnt_u32(x); //builtin assumed to be correct...
		#else
			int count = 0;
			for (int i=0; i<bitCount; i++) {
				if ((x>>i)&1)
					count++;
			}
			return count;
		#endif
		}
public:
		void check(int x)
		{
			CHECK( popcount(x) == correctPopcount(x) );
		}
	};
	Wavelengths_popcount_test t;

	for (unsigned int x=0; x<(1<<bitCount)-1; x++)
		t.check(x);
}

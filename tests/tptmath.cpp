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

#include "common/tptmath.h"
#include "catch.hpp"

TEST_CASE("remainder_p", "[tptmath]")
{
	CHECK( tptmath::remainder_p(5,3) == 2);
	CHECK( tptmath::remainder_p(-5,3) == 1);
}

TEST_CASE("binomial_gte1", "[tptmath]")
{
	CHECK( tptmath::binomial_gte1(0, 0.2) == Approx(0));
	// octave: 1-binopdf(0,10,0.2)
	CHECK( tptmath::binomial_gte1(10, 0.2) == Approx(0.8926252));
}

TEST_CASE("SmallKBinomialGenerator", "[tptmath]")
{
	class SmallKBinomialGeneratorTest : public tptmath::SmallKBinomialGenerator
	{
	public:
		SmallKBinomialGeneratorTest(unsigned int n, float p, unsigned int maxK_)
			: tptmath::SmallKBinomialGenerator(n, p, maxK_)
		{}
		float getCdf(unsigned int k)
		{
			if (k<maxK)
				return cdf[k];
			return 1.0f;
		}
	};

	SmallKBinomialGeneratorTest g(10, 0.2, 4);
	// Following values generated with octave: binocdf(0:3,10,0.2)
	CHECK( g.getCdf(0) == Approx(0.10737418) );
	CHECK( g.getCdf(1) == Approx(0.37580964) );
	CHECK( g.getCdf(2) == Approx(0.67779953) );
	CHECK( g.getCdf(3) == Approx(0.87912612) );
}


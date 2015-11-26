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

#include "simulation/elements/LIFE_Rule.hpp"
#include "catch.hpp"

static void checkSTAR(LIFE_Rule &rule)
{
	CHECK(rule.born(0) == 0);
	CHECK(rule.born(1) == 0);
	CHECK(rule.born(2) == 1);
	CHECK(rule.born(3) == 0);
	CHECK(rule.born(4) == 0);
	CHECK(rule.born(5) == 0);
	CHECK(rule.born(6) == 0);
	CHECK(rule.born(7) == 1);
	CHECK(rule.born(8) == 1);
	CHECK(rule.survive(0) == 0);
	CHECK(rule.survive(1) == 0);
	CHECK(rule.survive(2) == 0);
	CHECK(rule.survive(3) == 1);
	CHECK(rule.survive(4) == 1);
	CHECK(rule.survive(5) == 1);
	CHECK(rule.survive(6) == 1);
	CHECK(rule.survive(7) == 0);
	CHECK(rule.survive(8) == 0);
	CHECK(rule.states() == 6);
}

TEST_CASE("LIFE_rule", "[elements][PT_LIFE]")
{
	SECTION("separated rulestring")
	{
		LIFE_Rule rule("278", "3456", 6);
		checkSTAR(rule);
	}
	SECTION("composite rulestring")
	{
		LIFE_Rule rule("S3456/B278/6");
		checkSTAR(rule);
	}
	SECTION("messy composite rulestring")
	{
		LIFE_Rule rule("S  3456	 / b2 78 / 6 ");
		checkSTAR(rule);
	}
}

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

#include "catch.hpp"
#include "common/Compat.hpp"
#include "simulation/Position.hpp"
#include <type_traits>

template <class C>
bool SimPosApproxEq(const C& a, const C& b)
{
	return (a.x==Approx(b.x)) && (a.y==Approx(b.y));
}

static_assert(CELL>=2, "tests for SimPos currently assume that CELL>=2");

TEST_CASE("simulation/Position comparison", "[simulation][simpos]")
{
	SECTION("comparison ==")
	{
		CHECK( SimPosI(10,20) == SimPosI(10,20) );
		CHECK_FALSE( SimPosI(10,20) == SimPosI(10,21) );
		CHECK_FALSE( SimPosI(10,20) == SimPosI(11,20) );
		CHECK_FALSE( SimPosI(10,20) == SimPosI(11,21) );
	}
	SECTION("comparison !=")
	{
		CHECK_FALSE( SimPosI(10,20) != SimPosI(10,20) );
		CHECK( SimPosI(10,20) != SimPosI(10,21) );
		CHECK( SimPosI(10,20) != SimPosI(11,20) );
		CHECK( SimPosI(10,20) != SimPosI(11,21) );
	}
}

TEST_CASE("simulation/Position conversion", "[simulation][simpos]")
{
	SECTION("SimPos float->int: round to nearest")
	{
		CHECK( SimPosI(SimPosF(10.3f,20.3f)) == SimPosI(10,20) );
		CHECK( SimPosI(SimPosF(10.7f,20.7f)) == SimPosI(11,21) );

		CHECK( SimPosI(SimPosF(-10.3f,-20.3f)) == SimPosI(-10,-20) );
		CHECK( SimPosI(SimPosF(-10.7f,-20.7f)) == SimPosI(-11,-21) );
	}
	SECTION("SimPos int->float")
	{
		CHECK(SimPosApproxEq( SimPosF(SimPosI(10,20)) , SimPosF(10.0f,20.0f) ));
	}
	SECTION("SimPos float->cell: floored div by CELL")
	{
		CHECK( SimPosCell(SimPosF(10.3f*CELL,20.3f*CELL)) == SimPosCell(10,20) );
		CHECK( SimPosCell(SimPosF(10.7f*CELL,20.7f*CELL)) == SimPosCell(10,20) );
		// NB: default conversions to SimPosCell do not work correctly for negative positions, for speed
	}
	SECTION("SimPos float->cell (cell_safe): floored div by CELL")
	{
		CHECK( SimPosF(10.3f*CELL,20.3f*CELL).cell_safe() == SimPosCell(10,20) );
		CHECK( SimPosF(10.7f*CELL,20.7f*CELL).cell_safe() == SimPosCell(10,20) );
		CHECK( SimPosF(-10.3f*CELL,-20.3f*CELL).cell_safe() == SimPosCell(-11,-21) );
		CHECK( SimPosF(-10.7f*CELL,-20.7f*CELL).cell_safe() == SimPosCell(-11,-21) );
	}
	SECTION("SimPos int->cell: floored div by CELL")
	{
		CHECK( SimPosCell(SimPosI(10*CELL+1,20*CELL+1)) == SimPosCell(10,20) );
		CHECK( SimPosCell(SimPosI(10*CELL+CELL-1,20*CELL+CELL-1)) == SimPosCell(10,20) );
		// NB: default conversions to SimPosCell do not work correctly for negative positions, for speed
	}
	SECTION("SimPos int->cell (cell_safe): floored div by CELL")
	{
		CHECK( SimPosF(10*CELL+1,20*CELL+1).cell_safe() == SimPosCell(10,20) );
		CHECK( SimPosF(10*CELL+CELL-1,20*CELL+CELL-1).cell_safe() == SimPosCell(10,20) );
		CHECK( SimPosF(-10*CELL+1,-20*CELL+1).cell_safe() == SimPosCell(-10,-20) );
		CHECK( SimPosF(-10*CELL+CELL-1,-20*CELL+CELL-1).cell_safe() == SimPosCell(-10,-20) );
	}
	SECTION("SimPos cell->int")
	{
		SECTION("topLeft")
		{
			CHECK( SimPosI(SimPosCell(10,20).topLeft()) == SimPosI(10*CELL,20*CELL) );
		}
		SECTION("centre")
		{
			CHECK( SimPosI(SimPosCell(10,20).centre()) == SimPosI(std::lround(10*CELL + 0.5f*CELL), std::lround(20*CELL + 0.5f*CELL)) );
		}
		SECTION("bottom right")
		{
			CHECK( SimPosI(SimPosCell(10,20).bottomRight()) == SimPosI(10*CELL+CELL-1,20*CELL+CELL-1) );
		}
	}

	
	SECTION("SimPosD float->int: round to nearest")
	{
		CHECK( SimPosDI(SimPosDF(10.3f,20.3f)) == SimPosDI(10,20) );
		CHECK( SimPosDI(SimPosDF(10.7f,20.7f)) == SimPosDI(11,21) );

		CHECK( SimPosDI(SimPosDF(-10.3f,-20.3f)) == SimPosDI(-10,-20) );
		CHECK( SimPosDI(SimPosDF(-10.7f,-20.7f)) == SimPosDI(-11,-21) );
	}
	SECTION("SimPosD int->float")
	{
		CHECK(SimPosApproxEq( SimPosDF(SimPosDI(10,20)) , SimPosDF(10.0f,20.0f) ));
		CHECK(SimPosApproxEq( SimPosDF(SimPosDI(-10,-20)) , SimPosDF(-10.0f,-20.0f) ));
	}
	SECTION("SimPosD float->cell: round to nearest")
	{
		CHECK( SimPosDCell(SimPosDF(10.3f*CELL,20.3f*CELL)) == SimPosDCell(10,20) );
		CHECK( SimPosDCell(SimPosDF(10.7f*CELL,20.7f*CELL)) == SimPosDCell(11,21) );
		CHECK( SimPosDCell(SimPosDF(-10.3f*CELL,-20.3f*CELL)) == SimPosDCell(-10,-20) );
		CHECK( SimPosDCell(SimPosDF(-10.7f*CELL,-20.7f*CELL)) == SimPosDCell(-11,-21) );		
	}
	SECTION("SimPosD int->cell: round to nearest")
	{
		CHECK( SimPosDCell(SimPosDI(10*CELL+1,20*CELL+1)) == SimPosDCell(10,20) );
		CHECK( SimPosDCell(SimPosDI(10*CELL+CELL-1,20*CELL+CELL-1)) == SimPosDCell(11,21) );
		CHECK( SimPosDCell(SimPosDI(-10*CELL+1,-20*CELL+1)) == SimPosDCell(-10,-20) );
		CHECK( SimPosDCell(SimPosDI(-10*CELL+CELL-1,-20*CELL+CELL-1)) == SimPosDCell(-9,-19) );
	}
	SECTION("SimPosD cell->int")
	{
		CHECK( SimPosDI(SimPosDCell(10,20)) == SimPosDI(10*CELL,20*CELL) );
	}
}

TEST_CASE("simulation/Position maths return types", "[simulation][simpos]")
{
	SimPosCell c;
	SimPosI i;
	SimPosF f;
	SimPosDCell dc;
	SimPosDI di;
	SimPosDF df;
	int si;
	float sf;

	SECTION("add")
	{
		CHECK( (std::is_same<decltype(c+dc) , decltype(c)>::value) );
		CHECK( (std::is_same<decltype(c+di) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(c+df) , decltype(f)>::value) );

		CHECK( (std::is_same<decltype(i+dc) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(i+di) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(i+df) , decltype(f)>::value) );

		CHECK( (std::is_same<decltype(f+dc) , decltype(f)>::value) );
		CHECK( (std::is_same<decltype(f+di) , decltype(f)>::value) );
		CHECK( (std::is_same<decltype(f+df) , decltype(f)>::value) );


		CHECK( (std::is_same<decltype(dc+dc) , decltype(dc)>::value) );
		CHECK( (std::is_same<decltype(dc+di) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(dc+df) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(di+dc) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(di+di) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(di+df) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(df+dc) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df+di) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df+df) , decltype(df)>::value) );
	}

	SECTION("sub")
	{
		CHECK( (std::is_same<decltype(c-dc) , decltype(c)>::value) );
		CHECK( (std::is_same<decltype(c-di) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(c-df) , decltype(f)>::value) );

		CHECK( (std::is_same<decltype(i-dc) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(i-di) , decltype(i)>::value) );
		CHECK( (std::is_same<decltype(i-df) , decltype(f)>::value) );

		CHECK( (std::is_same<decltype(f-dc) , decltype(f)>::value) );
		CHECK( (std::is_same<decltype(f-di) , decltype(f)>::value) );
		CHECK( (std::is_same<decltype(f-df) , decltype(f)>::value) );


		CHECK( (std::is_same<decltype(dc-dc) , decltype(dc)>::value) );
		CHECK( (std::is_same<decltype(dc-di) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(dc-df) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(di-dc) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(di-di) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(di-df) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(df-dc) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df-di) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df-df) , decltype(df)>::value) );
	}

	SECTION("mul")
	{
		CHECK( (std::is_same<decltype(dc*si) , decltype(dc)>::value) );
		CHECK( (std::is_same<decltype(di*si) , decltype(di)>::value) );
		CHECK( (std::is_same<decltype(df*si) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(dc*sf) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(di*sf) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df*sf) , decltype(df)>::value) );
	}

	SECTION("div")
	{
		CHECK( (std::is_same<decltype(dc/si) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(di/si) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df/si) , decltype(df)>::value) );

		CHECK( (std::is_same<decltype(dc/sf) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(di/sf) , decltype(df)>::value) );
		CHECK( (std::is_same<decltype(df/sf) , decltype(df)>::value) );
	}
}

TEST_CASE("simulation/Position maths values", "[simulation][simpos]")
{
	SECTION("add")
	{
		CHECK( (SimPosI(10,20) + SimPosDI(1,2)) == SimPosI(11,22) );
		CHECK( (SimPosI(10,20) + SimPosDCell(1,2)) == SimPosI(10+1*CELL,20+2*CELL) );

		{
			SimPosI x(10,20);
			x += SimPosDI(1,2);
			CHECK( x == SimPosI(11,22) );
		}
		{
			SimPosI x(10,20);
			x += SimPosDCell(1,2);
			CHECK( x == SimPosI(10+1*CELL,20+2*CELL) );
		}

		CHECK( (SimPosDI(10,20) + SimPosDI(1,2)) == SimPosDI(11,22) );
		CHECK( (SimPosDI(10,20) + SimPosDCell(1,2)) == SimPosDI(10+1*CELL,20+2*CELL) );

		{
			SimPosDI x(10,20);
			x += SimPosDI(1,2);
			CHECK( x == SimPosDI(11,22) );
		}
		{
			SimPosDI x(10,20);
			x += SimPosDCell(1,2);
			CHECK( x == SimPosDI(10+1*CELL,20+2*CELL) );
		}
	}

	SECTION("sub")
	{
		CHECK( (SimPosI(10,20) - SimPosDI(1,2)) == SimPosI(9,18) );
		CHECK( (SimPosI(10,20) - SimPosDCell(1,2)) == SimPosI(10-1*CELL,20-2*CELL) );
		CHECK( (SimPosI(10,20) - SimPosI(9,18)) == SimPosDI(1,2) );
		
		{
			SimPosI x(10,20);
			x -= SimPosDI(1,2);
			CHECK( x == SimPosI(9,18) );
		}
		{
			SimPosI x(10,20);
			x -= SimPosDCell(1,2);
			CHECK( x == SimPosI(10-1*CELL,20-2*CELL) );
		}

		CHECK( (SimPosDI(10,20) - SimPosDI(1,2)) == SimPosDI(9,18) );
		CHECK( (SimPosDI(10,20) - SimPosDCell(1,2)) == SimPosDI(10-1*CELL,20-2*CELL) );

		{
			SimPosDI x(10,20);
			x -= SimPosDI(1,2);
			CHECK( x == SimPosDI(9,18) );
		}
		{
			SimPosDI x(10,20);
			x -= SimPosDCell(1,2);
			CHECK( x == SimPosDI(10-1*CELL,20-2*CELL) );
		}
	}

	SECTION("mul")
	{
		CHECK( (SimPosDI(10,20) * 3) == SimPosDI(30,60) );
		CHECK(SimPosApproxEq( SimPosDI(10,20) * 2.5f , SimPosDF(25,50) ));

		SimPosDI xd(10,20);
		xd *= 3;
		CHECK( xd == SimPosDI(30, 60) );
	}

	SECTION("div")
	{
		CHECK(SimPosApproxEq( SimPosDI(20,40) / 4.0f , SimPosDF(5,10) ));

		SimPosDI xd(20,40);
		xd /= 4;
		CHECK( xd == SimPosDI(5, 10) );
	}
}

TEST_CASE("simulation/PositionD funcs", "[simulation][simpos]")
{
	SECTION("length")
	{
		CHECK( SimPosDF(1,2).length() == Approx(std::sqrt(5)) );
		CHECK( SimPosDI(3,4).length() == 5 );
		CHECK( SimPosDCell(3,4).length() == 5*CELL );
	}

	SECTION("length_1")
	{
		CHECK( SimPosDF(3,5).length_1() == 8 );
		CHECK( SimPosDF(5,3).length_1() == 8 );
		CHECK( SimPosDI(3,5).length_1() == 8 );
		CHECK( SimPosDI(5,3).length_1() == 8 );
		CHECK( SimPosDCell(3,5).length_1() == 8*CELL );
		CHECK( SimPosDCell(5,3).length_1() == 8*CELL );
	}

	SECTION("maxComponent")
	{
		CHECK( SimPosDF(3,5).maxComponent() == 5 );
		CHECK( SimPosDF(5,3).maxComponent() == 5 );
		CHECK( SimPosDI(3,5).maxComponent() == 5 );
		CHECK( SimPosDI(5,3).maxComponent() == 5 );
		CHECK( SimPosDCell(3,5).maxComponent() == 5*CELL );
		CHECK( SimPosDCell(5,3).maxComponent() == 5*CELL );
	}

	SECTION("unit")
	{
		CHECK(SimPosApproxEq( SimPosDI(10,20).unit_1() , SimPosDF(0.5f,1.0f) ));
		CHECK(SimPosApproxEq( SimPosDI(30,40).unit() , SimPosDF(0.6f,0.8f) ));
	}
}



#if (defined(__x86_64__) || defined(__i386)) && defined(__GNUC__) && !defined(__clang__)
[[gnu::target("fpmath=387")]] TPT_NOINLINE void SimPos_float_testF_forceFPU(SimPosF &dest_f, SimPosI &dest_i, int bits);
[[gnu::target("fpmath=387")]] TPT_NOINLINE void SimPos_float_testFT_forceFPU(SimPosF &dest_f, SimPosI &dest_i, int bits);

void SimPos_float_testF_forceFPU(SimPosF &dest_f, SimPosI &dest_i, int bits)
{
	dest_f = SimPosF(1.5f-std::pow(2.0f,-bits), 1.5f);
	dest_i = dest_f;
}
void SimPos_float_testFT_forceFPU(SimPosF &dest_f, SimPosI &dest_i, int bits)
{
	dest_f = SimPosFT(1.5f-std::pow(2.0f,-bits), 1.5f);
	dest_i = dest_f;
}
#endif

void SimPos_float_testF_current(SimPosF &dest_f, SimPosI &dest_i, int bits)
{
	dest_f = SimPosF(1.5f-std::pow(2.0f,-bits), 1.5f);
	dest_i = dest_f;
}
void SimPos_float_testFT_current(SimPosF &dest_f, SimPosI &dest_i, int bits)
{
	dest_f = SimPosFT(1.5f-std::pow(2.0f,-bits), 1.5f);
	dest_i = dest_f;
}

TEST_CASE("simulation/Position float rounding", "[simulation][simpos][float-rounding]")
{
	// f is the floating point number after being stored to memory
	// i1 is f converted to an integer inside the function which calculated f, so probably before being stored to memory (so the floating point value converted may have had more precision)
	// i2 is f converted to an integer after being stored to memory
	// Ideally, i1 and i2 will match.

	SimPosF f;
	SimPosI i1, i2;
	int bits = 40;// how far down the significand the difference from 1.5 is

	SECTION("default FPU, SimPosFT")
	{
		// With SimPosFT used to force store to memory. This should always give i1==i2.
		SimPos_float_testFT_current(f, i1, bits);
		i2 = f;
		CHECK( i1.x == i2.x );
	}
/*
	SECTION("default FPU, SimPosF")
	{
		// Without SimPosFT. If i1==i2 for this, then it's probably safe to remove the forced store to memory and turn SimPosFT into a no-op for this arch/FPU.
		SimPos_float_testF_current(f, i1, bits);
		i2 = f;
		CHECK( i1.x == i2.x );
	}

#if (defined(__x86_64__) || defined(__i386)) && defined(__GNUC__) && !defined(__clang__)
	// Versions which tell the compiler to use the 387 FPU. SimPosFT one matches, SimPosF one breaks.
	SECTION("387 FPU, SimPosFT")
	{
		SimPos_float_testFT_forceFPU(f, i1, bits);
		i2 = f;
		CHECK( i1.x == i2.x );
	}
	SECTION("387 FPU, SimPosF")
	{
		SimPos_float_testF_forceFPU(f, i1, bits);
		i2 = f;
		CHECK( i1.x == i2.x );
	}
#endif
*/
}


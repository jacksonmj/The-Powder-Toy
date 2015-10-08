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

#ifndef Simulation_ElementsShared_wavelengths_h
#define Simulation_ElementsShared_wavelengths_h

#include "common/tpt-stdint.h"
#include "common/Intrinsics.hpp"
#include "graphics/ARGBColour.h"

namespace ElementsShared
{

class Wavelengths
{
protected:
	// Count the number of 1-bits in x (valid for x length of 14 bits)
	static unsigned int popcount(unsigned int x)
	{
#if defined(__POPCNT__)
		return _popcnt32(x);
#elif uint_fast32_t == uint_64_t
		// From http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSet64
		// Valid for 14 bit numbers.
		return ((x * 0x200040008001ULL) & 0x111111111111111ULL) % 0xf;
#else
		// From http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel , modified to only support 16 bit numbers.
		unsigned int c;
		x = x - ((x >> 1) & 0x5555);
		x = (x & 0x3333) + ((x >> 2) & 0x3333);
		c = ((x * 0x1111) >> 12) & 0xF;
		return c;
#endif
	}

public:
	// Functions to convert a wavelengths value (wl) into a colour
	static void toRGB_noLimit(unsigned int wl, int& dest_r, int& dest_g, int& dest_b)
	{
		unsigned int r=0, g=0, b=0;
		r = popcount((wl>>18)&0xFFF);
		g = popcount((wl>>9)&0xFFF);
		b = popcount((wl)&0xFFF);
		unsigned int x = (r+g+b+1);
		const unsigned int mult = 624;
		dest_r = r * mult/x;
		dest_g = g * mult/x;
		dest_b = b * mult/x;
	}
	static void toRGB(int wl, int& dest_r, int& dest_g, int& dest_b)
	{
		int r,g,b;
		toRGB_noLimit(wl, r, g, b);
		dest_r = (r>255) ? 255 : r;
		dest_g = (g>255) ? 255 : g;
		dest_b = (b>255) ? 255 : b;
	}
	static ARGBColour toARGB(int wl)
	{
		int r,g,b;
		toRGB(wl, r, g, b);
		return COLRGB(r, g, b);
	}
};

}

#endif

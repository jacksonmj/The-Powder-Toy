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

#ifndef tptmath_h
#define tptmath_h

class tptmath
{
public:
	template<typename T>
	static constexpr int isign(T i)
	{
		return (i>0) ? 1 : ((i<0) ? -1 : 0);
	}

	static unsigned scale_to_clamped_byte(float f, float min, float max)
	{
		if (f<min)
			return 0;
		if (f>max)
			return 255;
		return (unsigned)(255.0f*(f-min)/(max-min));
	}

	static constexpr float clamp_flt(float x, float min, float max)
	{
		// optimised by gcc into two SSE instructions (maxss, minss)
		return (x>max) ? max : ((x<min) ? min : x);
	}
	static constexpr int clamp_int(int x, int min, int max)
	{
		return (x>max) ? max : ((x<min) ? min : x);
	}
};

struct Vec2f
{
	float x, y;
};
struct Vec2i
{
	int x, y;
};
struct Vec2sc
{
	signed char x, y;
};

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#endif

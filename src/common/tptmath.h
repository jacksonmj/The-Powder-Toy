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

#include <cmath>

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
	// Find the positive remainder of x/y, with y assumed to be positive
	// (x%y but with more helpful handling of negative x)
	//  e.g. remainder_p(5,3)=2; remainder_p(-5,3) = 1
	static constexpr int remainder_p(int x, int y)
	{
		return (x % y) + (x>=0 ? 0 : y);
	}
	static constexpr float remainder_p(float x, float y)
	{
		return std::fmod(x, y) + (x>=0 ? 0 : y);
	}

	// X ~ binomial(n,p), returns P(X>=1)
	// e.g. If a reaction has n chances of occurring, each time with probability p, this returns the probability that it occurs at least once.
	static float binomial_gte1(int n, float p)
	{
		return 1.0f - std::pow(1.0f-p, n);
	}

	class SmallKBinomialGenerator
	{
	protected:
		float *cdf;
		unsigned int maxK;
	public:
		// Class to generate numbers from a binomial distribution, up to a maximum value (maxK).
		// Results which would have been above maxK return maxK.
		// Note: maxK must be small, otherwise the method used in this class is inefficient. (n and p can be any valid value)
		SmallKBinomialGenerator(unsigned int n, float p, unsigned int maxK_);
		~SmallKBinomialGenerator();
		unsigned int calc(float randFloat);
	};
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

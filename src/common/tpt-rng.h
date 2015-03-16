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

#ifndef tptrng_h
#define tptrng_h

#include <random>

/*
 * Classes to help with using random numbers in TPT
 *
 * tpt_rng: wrapper around a C++11 random number generator
 * RandomStore and RandomStore_auto: classes to help split a random number into several smaller random numbers, in an attempt to reduce the amount of time spent generating random numbers.
 *
 *
 * Examples:
 *
 *  tpt_rng rng;
 *  foo = rng.randInt<200,250>(); // random integer between 200 and 250 inclusive
 *  if (rng.chance(1,10)) // 1/10 chance
 *		dostuff();
 *  if (rng.chance<1,10>()) // 1/10 chance
 *		dostuff();
 *  if (rng.chancef(0.1f)) // 1/10 chance
 *		dostuff();
 *  angle = rng.randFloat(-M_PI,M_PI);
 *  x = tpt_rng.randUint32();
 *
 *  RandomStore_auto has mostly the same functions as tpt_rng. However, note that the denominator for RandomStore_auto::chance() should be no larger than necessary (larger denominator = more bits used to calculate result, which defeats the whole point of using RandomStore_auto instead of tpt_rng).
 *
 *
 * Two versions of chance() and randInt() are provided in tpt_rng and RandomStore_auto - template and no template.
 *  rng.chance<1,10>();				rng.chance(1,10);
 *  rng.randInt<200,250>();	rng.randInt(200,250);
 * The template versions of these functions should be used if min and max are constant (known at compile time).
 * This is because some of the no template versions may be slower, depending on the compiler (g++ seems to be clever enough to optimise no template versions with constant arguments into something similar to the template version, but this is not guaranteed to happen). The template versions also have some compile-time checks to warn if the arguments seem wrong (such as a chance always being false).
 * randFloat and chancef do not have template versions
 *
 * For chance(numerator,denominator), denominator must be > 0 and <= 2^32-1.
 * RandomStore_auto has a lower upper limit, denominator <= 2^31-1.
 * If numerator>=denominator, chance always returns true.
 * If numerator<=0, chance always returns false.
 */

class tpt_rng
{
protected:
	// TODO: test effects of different generators on Simulation (speed, and are numbers still random enough with different generators).
	std::mt19937 gen;

	static constexpr uint_fast32_t getChanceThreshold_frac(uint_fast32_t numerator, uint_fast32_t denominator)
	{
		return double(numerator)/denominator*4294967296.0;
	}
	static constexpr uint_fast32_t getChanceThreshold_float(float p)
	{
		return p*4294967296.0;
	}
	static constexpr uint_fast32_t getChanceThreshold_double(double p)
	{
		return p*4294967296.0;
	}
	static constexpr float getScaleForFloat(float minVal, float maxVal)
	{
		return (maxVal-minVal)/4294967296.0f;
	}

public:
	tpt_rng();
	void seed(uint_fast32_t val)
	{
		gen.seed(val);
	}

	// These should always return the specified number of bits of random data.
	// (So if the generator is changed from mt19937 to something else, these functions should be modified if necessary, possibly using std::independent_bits_engine)
	uint_fast32_t randUint32();
	uint_fast64_t randUint64();

	/* Functions which return true with approximately the specified chance.
	 * If the arguments in the calling functions are constants, then any decent optimising compiler should be able to calculate getChanceThreshold at compile time. (This has been tested - in g++ with optimisations on, calling any of these chance functions produces instructions which just generate a random number, then compare it to a constant, no calculation is performed at runtime to get the constant).
	 */
	bool chance(uint_fast32_t numerator, uint_fast32_t denominator) // Chance of returning true is numerator/denominator
	{
		return (randUint32() < getChanceThreshold_frac(numerator, denominator));
	}
	bool chancef(float p) // Chance of returning true is p (should be 0 < p < 1)
	{
		return (randUint32() < getChanceThreshold_float(p));
	}
	bool chancef(double p) // Chance of returning true is p (should be 0 < p < 1)
	{
		return (randUint32() < getChanceThreshold_double(p));
	}

	// Get a random integer in the range minVal to maxVal inclusive
	int randInt(int minVal, int maxVal)
	{
		// This is the same method as the downscaling part of std::uniform_int_distribution::operator() in libstdc++
		// Reproduced here to ensure the same results on different platforms
		const uint_fast32_t resultRange = (maxVal-minVal+1);
		const uint_fast32_t scaling = uint_fast32_t(uint_fast32_t(0xFFFFFFFF) / resultRange);
		const uint_fast32_t regenThreshold = resultRange * scaling;
		uint_fast32_t ret;
		do
			ret = randUint32();
		while (ret>=regenThreshold);
		return ret/scaling + minVal;
	}

public:
	float randFloat(float minVal, float maxVal)
	{
		return randUint32()*getScaleForFloat(minVal,maxVal) + minVal;
	}

	// For tpt_rng, template versions are just wrappers with some static_asserts
	template <int minVal, int maxVal>
	bool randInt()
	{
		static_assert(minVal<maxVal, "minVal must be less then maxVal");
		return randInt(minVal, maxVal);
	}
	template <uint_fast32_t numerator, uint_fast32_t denominator>
	bool chance()
	{
		static_assert(denominator>0, "denominator must be greater than 0");
		static_assert(numerator>0, "chance with constant numerator<=0 is equivalent to false");
		static_assert(numerator<denominator, "chance with constant numerator>=denominator is equivalent to true");
		return chance(numerator, denominator);
	}

};

class RandomStore
{
protected:
	uint_fast32_t randStore;
public:
	RandomStore(uint_fast32_t randStore_=0) : randStore(randStore_) {}
	// Store a random integer to be sliced up
	void store(uint_fast32_t randStore_)
	{
		randStore = randStore_;
	}
	// Retrieve a random integer that is bitsNeeded bits long
	uint_fast32_t getRandomBits(int bitsNeeded)
	{
		int ret = randStore & (uint_fast32_t(1<<bitsNeeded) - 1);
		randStore >>= bitsNeeded;
		return ret;
	}
	// Return a random integer in the given range.
	// NB: If 2^bitsToUse == maxVal-minVal+1 (the number of distinct values that can be generated), then using this function is fine. Otherwise, returned values will be biased: if this is a problem, then either use tpt_rng.randInt or RandomStore_auto.randInt instead (which do not suffer from this problem), or ensure 2^bitsToUse is much larger than maxVal-minVal+1 (which reduces the amount of bias).
	int randInt(int minVal, int maxVal, int bitsToUse)
	{
		return (getRandomBits(bitsToUse) % (maxVal-minVal+1))+minVal;
	}
	// Functions which automatically calculate the number of bits required for a chance() or randInt() are not defined here. The programmer is responsible for ensuring that no more than the stored number of bits are retrieved, and forcing the number of bits retrieved to be stated in the calling code helps make it clear whether this limit is being respected.
	// Use something like: 3>getRandomBits(4) for 3/16 chance
};




// Template to calculate index of most significant bit set (index of LSB=1, MSB=32)
template <uint_fast32_t val>
struct CountBitsOccupied
{
	enum { bits = 1+CountBitsOccupied<(val>>1)>::bits };
};
template <>
struct CountBitsOccupied<0>
{
	enum { bits = 0 };
};

// Alternative to RandomStore which obtains new random data when necessary
// Easier to use, but the overhead of tracking when new data is needed makes it a bit slower
class RandomStore_auto : public RandomStore
{
protected:
	static const int rngBitsProvided = 32;
	tpt_rng *rng;
	int bitsUsed;
public:
	RandomStore_auto(tpt_rng *rng_) : RandomStore(0), rng(rng_), bitsUsed(rngBitsProvided)
	{}
	void clearStore()
	{
		randStore = 0;
		bitsUsed = rngBitsProvided;
	}
	void store(uint_fast32_t randStore_)
	{
		randStore = randStore_;
		bitsUsed = 0;
	}
	uint_fast32_t getRandomBits(int bitsNeeded)
	{
		if (bitsUsed > rngBitsProvided-bitsNeeded)
		{
			randStore = rng->randUint32();
			bitsUsed = bitsNeeded;
		}
		else
		{
			bitsUsed += bitsNeeded;
		}
		int ret = randStore & (uint_fast32_t(1<<bitsNeeded) - 1);
		randStore >>= bitsNeeded;
		return ret;
	}

	// Calculate index of most significant bit set (index of LSB=1, MSB=32)
	static int countBitsOccupied_dynamic(uint_fast32_t x)
	{
		// Version using compiler builtins
		// TODO: add case for MSVC (_BitScanReverse)?
#if defined(__GNUC__)
		return (x==0) ? 0 : 32-__builtin_clz(x);
#else
		return countBitsOccupied_dynamic_fallback(x);
#endif
	}
	static int countBitsOccupied_dynamic_fallback(uint_fast32_t x)
	{
		// Version to use if no compiler builtins are available
		int bits = 0;
		if (x&0xFFFF0000)
			x >>= 16, bits += 16;
		if (x&0xFF00)
			x >>= 8, bits += 8;
		if (x&0xF0)
			x >>= 4, bits += 4;
		if (x&0xC)
			x >>= 2, bits += 2;
		if (x&0x2)
			x >>=1, bits += 1;
		if (x&0x1)
			bits += 1;
		return bits;
	}

	static constexpr int decideBitsToUse(int minBits, int minVal, int maxVal)
	{
		// If there is a high chance of rejecting the random number, use an extra bit
		// e.g. maxVal-minVal+1 = 33 -> minBits = 6
		//  6 bits used -> 31/64 chance of rejecting and needing to use another 6 bits
		//  7 bits used -> 29/128 chance of rejecting and needing to use another 7 bits.
		// Using an extra bit reduces the chance of rejecting, so on average fewer bits should be used even though each call uses an extra bit.
		return (minBits>2 && (1<<minBits)!=(maxVal-minVal+1) && ((maxVal-minVal+1)-(1<<(minBits-1)) < (1<<(minBits-2)))) ? minBits+1 : minBits;
	}

	// Return a random integer in the given range
	// 2^bitsToUse should be >= maxVal-minVal+1, and bitsToUse should be < 32
	// (Unlike RandomStore.randInt, this function is never biased towards certain numbers)
	// This is almost the same method as the downscaling part of std::uniform_int_distribution::operator() in libstdc++
	int randInt(int minVal, int maxVal, int bitsToUse)
	{
		if (bitsToUse>=16)//if lots of bits are needed, just use a whole random number
			return rng->randInt(minVal,maxVal);
		const uint_fast32_t resultRange = (maxVal-minVal+1);
		// if bitsToUse is no bigger than necessary, scaling=1, and all uses of scaling should get optimised out (assuming minVal and maxVal are known at compile time).
		const uint_fast32_t scaling = uint_fast32_t((1<<bitsToUse) / resultRange);
		const uint_fast32_t regenThreshold = resultRange * scaling;
		uint_fast32_t ret;
		do
			ret = getRandomBits(bitsToUse);
		while (ret>=regenThreshold);
		return ret/scaling + minVal;
	}
	int randInt(int minVal, int maxVal)
	{
		// could use decideBitsToUse() here, but the condition checking work in decideBitsToUse might exceed the savings from fewer bits used. TODO: check whether it does
		return randInt(minVal, maxVal, countBitsOccupied_dynamic(maxVal-minVal));
	}
	bool chance(uint_fast32_t numerator, uint_fast32_t denominator)
	{
		return (uint_fast32_t(randInt(0,denominator-1)) < numerator);
	}
	// template versions
	template <int minVal, int maxVal>
	int randInt()
	{
		static_assert(minVal<maxVal, "minVal must be less then maxVal");
		return randInt(minVal, maxVal, decideBitsToUse(CountBitsOccupied<maxVal-minVal>::bits, minVal, maxVal));
	}
	template <uint_fast32_t numerator, uint_fast32_t denominator>
	bool chance()
	{
		static_assert(denominator>0, "denominator must be greater than 0");
		static_assert(numerator>0, "constant chance with numerator<=0 is equivalent to false");
		static_assert(numerator<denominator, "constant chance with numerator>=denominator is equivalent to true");
		// TODO: static_assert check gcd==1
		return (uint_fast32_t(randInt<0,denominator-1>()) < numerator);
	}

	// Can't handle these using a limited number of bits, so call tpt_rng functions instead:
	float randFloat(float minVal, float maxVal) { return rng->randFloat(minVal, maxVal); }
	uint_fast32_t randUint32() { return rng->randUint32(); }
};

#endif

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
#include "simulation/CellsData.hpp"

#include <numeric>
#include <algorithm>

#include "common/Intrinsics.hpp"

void CellsData_limit(CellsFloatP dest, float minVal, float maxVal)
{
	float *p = reinterpret_cast<float*>(dest);
	for (int i=0; i<(XRES/CELL)*(YRES/CELL); i++)
		p[i] = tptmath::clamp_flt(p[i], minVal, maxVal);
}

void CellsData_limit(const_CellsFloatRP src, CellsFloatRP dest, float minVal, float maxVal)
{
	const float *psrc = reinterpret_cast<const float*>(src);
	float *pdest = reinterpret_cast<float*>(dest);
	for (int i=0; i<(XRES/CELL)*(YRES/CELL); i++)
		pdest[i] = tptmath::clamp_flt(psrc[i], minVal, maxVal);
}

template<typename DataType>
void CellsData_fill(DataType (*dest)[XRES/CELL], DataType value)
{
	std::fill_n(reinterpret_cast<DataType*>(dest), (XRES/CELL)*(YRES/CELL), value);
}

template<typename DataType>
void CellsData_copy(const DataType (* TPT_RESTRICT src)[XRES/CELL], DataType (* TPT_RESTRICT dest)[XRES/CELL])
{
	std::copy_n(reinterpret_cast<const DataType*>(src), (XRES/CELL)*(YRES/CELL), reinterpret_cast<DataType*>(dest));
}

template<typename DataType>
void CellsData_invert(const DataType (* TPT_RESTRICT src)[XRES/CELL], DataType (* TPT_RESTRICT dest)[XRES/CELL])
{
	const DataType *psrc = reinterpret_cast<const DataType*>(src);
	DataType *pdest = reinterpret_cast<DataType*>(dest);
	for (int i=0; i<(XRES/CELL)*(YRES/CELL); i++)
		pdest[i] = -psrc[i];
}

template <bool (*singleFunc)(unsigned char)>
size_t CellsData_count_if_single(const unsigned char (*src)[XRES/CELL])
{
	const unsigned char *p = reinterpret_cast<const unsigned char*>(src);
	size_t count = 0;
	for (size_t i=0; i<(XRES/CELL)*(YRES/CELL); i++)
	{
		if (singleFunc(p[i]))
			count++;
	}
	return count;
}

#ifdef __SSE2__
#define CELLSDATA_SIMD128
// singleFunc should return true if the passed byte matches the condition.
// simdFunc is passed 16 bytes in a __m128i. If n of these match the condition, the return value should contain n bytes=1, and all other bytes=0 (order is unimportant, but the value of each byte should not be greater than 1).
template <bool (*singleFunc)(unsigned char), __m128i (*simdFunc)(__m128i)>
size_t CellsData_count_if_simd128(const unsigned char (*src)[XRES/CELL])
{
	const unsigned char *p = reinterpret_cast<const unsigned char*>(src);

	size_t count = 0;//number of bytes meeting the condition
	size_t i = 0;//overall array position index

	// Ensure 16 byte alignment
	for (; (uintptr_t(p+i) % 16)!=0 && i<(XRES/CELL)*(YRES/CELL); i++)
	{
		if (singleFunc(p[i]))
			count++;
	}

	const __m128i zero = _mm_setzero_si128();
	while (i<(XRES/CELL)*(YRES/CELL)-32)
	{
		// SIMD version of: count += (p[i]&8)>>3

		__m128i accum_8_1 = zero;
		__m128i accum_8_2 = zero;
		size_t maxLoops = ((XRES/CELL)*(YRES/CELL) - i)/32;// 16 bytes per __m128i, and loop is unrolled to do 2 per iteration = 32 bytes
		if (maxLoops>255)// make sure bytes in accumulators don't overflow (adding 0 or 1 to each byte on each iteration)
			maxLoops = 255;
		const __m128i* p128 = reinterpret_cast<const __m128i*>(p+i);
		const __m128i* p128max = p128+maxLoops*2;
		i += maxLoops * 32;//set overall position index to value it will have at the end of the following loop
		while (p128<p128max)
		{
			// load 16 bytes from array into each
			__m128i x1 = _mm_load_si128(p128++);
			__m128i x2 = _mm_load_si128(p128++);
			x1 = simdFunc(x1);
			x2 = simdFunc(x2);
			// byte-wise addition
			accum_8_1 = _mm_add_epi8(accum_8_1, x1);
			accum_8_2 = _mm_add_epi8(accum_8_2, x2);
		}
		// add bytes 0-7 to 8-15, results in 8 * 16-bit ints (each <= 2*255)
		accum_8_1 = _mm_add_epi16(_mm_unpacklo_epi8(accum_8_1, zero), _mm_unpackhi_epi8(accum_8_1, zero));
		accum_8_2 = _mm_add_epi16(_mm_unpacklo_epi8(accum_8_2, zero), _mm_unpackhi_epi8(accum_8_2, zero));
		// add the accumulators together, gives 8 * 16-bit ints (each <= 4*255)
		__m128i accum = _mm_add_epi16(accum_8_1, accum_8_2);
		// to 4*32-bit, 2*64-bit
		accum = _mm_add_epi32(_mm_unpacklo_epi16(accum, zero), _mm_unpackhi_epi16(accum, zero));
		accum = _mm_add_epi64(_mm_unpacklo_epi32(accum, zero), _mm_unpackhi_epi32(accum, zero));
		// add resulting 64-bit numbers to count
		uint64_t *results = reinterpret_cast<uint64_t*>(&accum);
		count += results[0]+results[1];
	}

	for (; i<(XRES/CELL)*(YRES/CELL); i++)
	{
		if (singleFunc(p[i]))
			count++;
	}
	return count;
}
#endif


static bool CellsData_count_and8_single(unsigned char x)
{
	return (x&8);
}
#if defined(CELLSDATA_SIMD128) && defined(__SSE2__)
static __m128i CellsData_count_and8_simd128(__m128i x)
{
	const __m128i mask = _mm_set1_epi8(8);
	// each byte &= 8
	x = _mm_and_si128(x, mask);
	// shift right, so in each byte 8 -> 1. Doesn't matter what size split, since all possibilities contain a whole number of bytes, and previous x&=mask means there will be no transfer between adjacent bytes.
	x = _mm_srli_epi64(x, 3);
	return x;
}
size_t CellsData_count_and8(const unsigned char (*src)[XRES/CELL])
{
	return CellsData_count_if_simd128<CellsData_count_and8_single,CellsData_count_and8_simd128>(src);
}
#else
size_t CellsData_count_and8(const unsigned char (*src)[XRES/CELL])
{
	return CellsData_count_if_single<CellsData_count_and8_single>(src);
}
#endif


static bool CellsData_count_1_single(unsigned char x)
{
	return x;
}
#if defined(CELLSDATA_SIMD128) && defined(__SSE2__)
static __m128i CellsData_count_1_simd128(__m128i x)
{
	return x;
}
size_t CellsData_count_1(const unsigned char (*src)[XRES/CELL])
{
	return CellsData_count_if_simd128<CellsData_count_1_single,CellsData_count_1_simd128>(src);
}
#else
size_t CellsData_count_1(const unsigned char (*src)[XRES/CELL])
{
	return CellsData_count_if_single<CellsData_count_1_single>(src);
}
#endif



#define CELLSDATA_INSTANTIATE(DataType) \
template class CellsData<DataType>;\
template void CellsData_fill<DataType>(DataType (*dest)[XRES/CELL], DataType value);\
template void CellsData_copy<DataType>(const DataType (*src)[XRES/CELL], DataType (*dest)[XRES/CELL]);

#define CELLSDATA_INSTANTIATE_NUMERIC(DataType) \
CELLSDATA_INSTANTIATE(DataType)\
template void CellsData_invert<DataType>(const DataType (*src)[XRES/CELL], DataType (*dest)[XRES/CELL]);


CELLSDATA_INSTANTIATE_NUMERIC(float)
CELLSDATA_INSTANTIATE_NUMERIC(unsigned char)



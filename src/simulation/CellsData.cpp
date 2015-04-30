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
#include "algorithm/count_if.hpp"
#include "algorithm/transform.hpp"

class Kernel_limit_float : public tptalgo::Kernel_base
{
public:
	TPTALGO_KERN_COMMON(Kernel_limit_float)
	const float minVal, maxVal;
	Kernel_limit_float(float minVal_, float maxVal_) : minVal(minVal_), maxVal(maxVal_) {}
	
	TPTALGO_KERN_IMPL(float)
	{
	public:
		impl(...) {}
		float op(const Kernel &kernel, float x) const
		{
			return tptmath::clamp_flt(x, kernel.minVal, kernel.maxVal);
		}
	};

#if HAVE_LIBSIMDPP
	TPTALGO_KERN_IMPL_SIMDPP(simdpp::float32, N)
	{
	public:
		using T = simdpp::float32<N>;
		const T minVec, maxVec;
		impl(const Kernel &kernel) : minVec(simdpp::splat(kernel.minVal)), maxVec(simdpp::splat(kernel.maxVal)) {}
		T op(const Kernel &kernel, T x) const
		{
			return simdpp::min(simdpp::max(x,minVec),maxVec);
		}
		static constexpr unsigned unrollCount = 4;
	};
#endif
};

void CellsData_limit(CellsFloatP dest, float minVal, float maxVal)
{
	const Kernel_limit_float kernel(minVal, maxVal);
	tptalgo::transform(kernel, reinterpret_cast<const float*>(dest), reinterpret_cast<float*>(dest), (XRES/CELL)*(YRES/CELL));
}

void CellsData_limit(const_CellsFloatRP src, CellsFloatRP dest, float minVal, float maxVal)
{
	const Kernel_limit_float kernel(minVal, maxVal);
	tptalgo::transform(kernel, reinterpret_cast<const float*>(src), reinterpret_cast<float*>(dest), (XRES/CELL)*(YRES/CELL));
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


// Count the number of cells==1. NB: only for use if all values are guaranteed to be either 0 or 1.

class Kernel_count_uint8_1 : public tptalgo::Kernel_base
{
public:
	TPTALGO_KERN_COMMON(Kernel_count_uint8_1)
	
	TPTALGO_KERN_IMPL(uint8_t)
	{
	public:
		impl(...) {}
		bool op(const Kernel &kernel, uint8_t x) const
		{
			return x;
		}
	};

#if HAVE_LIBSIMDPP
	TPTALGO_KERN_IMPL_SIMDPP(simdpp::uint8, N)
	{
	public:
		using T = simdpp::uint8<N>;
		impl(...) {}
		T op(const Kernel &kernel, T x) const
		{
			return x;
		}
		static constexpr uint64_t max_count = 1;
	};
#endif
};

size_t CellsData_count_1(const unsigned char (*src)[XRES/CELL])
{
	return tptalgo::count_if(Kernel_count_uint8_1(), reinterpret_cast<const unsigned char*>(src), (XRES/CELL)*(YRES/CELL));
}


// Count the number of cells >= a specific value

class Kernel_count_uint8_and8 : public tptalgo::Kernel_base
{
public:
	TPTALGO_KERN_COMMON(Kernel_count_uint8_and8)
	
	TPTALGO_KERN_IMPL(uint8_t)
	{
	public:
		impl(...) {}
		bool op(const Kernel &kernel, uint8_t x) const
		{
			return (x&8);
		}
	};

#if HAVE_LIBSIMDPP
	TPTALGO_KERN_IMPL_SIMDPP(simdpp::uint8, N)
	{
	public:
		using T = simdpp::uint8<N>;
		const T vec_8;
		impl(const Kernel &kernel) : vec_8(simdpp::splat(8)) {}
		T op(const Kernel &kernel, T x) const
		{
			// Shifting uint32 is more efficient than shifting uint8 (uint8 needs multiple instructions), and gives the same answer because of the &vec_8 mask.
			return T(simdpp::shift_r<3>(simdpp::uint32<N/4>(x & vec_8)));
		}
		static constexpr uint64_t max_count = 1;
	};
#endif
};

size_t CellsData_count_and8(const unsigned char (*src)[XRES/CELL])
{
	return tptalgo::count_if(Kernel_count_uint8_and8(), reinterpret_cast<const unsigned char*>(src), (XRES/CELL)*(YRES/CELL));
}



class Kernel_subtract_sat : public tptalgo::Kernel_base
{
public:
	TPTALGO_KERN_COMMON(Kernel_subtract_sat)
	unsigned char amount;
	Kernel_subtract_sat(unsigned char amount_) : amount(amount_) {}

	TPTALGO_KERN_IMPL(unsigned char)
	{
	public:
		impl(...) {}
		unsigned char op(const Kernel &kernel, unsigned char x) const
		{
			return ((x>=kernel.amount) ? x-kernel.amount : 0);
		}
	};

#if HAVE_LIBSIMDPP
	TPTALGO_KERN_IMPL_SIMDPP(simdpp::uint8, N)
	{
	public:
		using T = simdpp::uint8<N>;
		const T amountVec;
		impl(const Kernel &kernel) : amountVec(simdpp::splat(kernel.amount)) {}
		T op(const Kernel &kernel, T x) const
		{
			return simdpp::sub_sat(x, amountVec);
		}
		static constexpr unsigned unrollCount = 4;
	};
#endif
};

void CellsData_subtract_sat(const unsigned char (*src)[XRES/CELL], unsigned char (*dest)[XRES/CELL], unsigned char value)
{
	Kernel_subtract_sat kernel(value);
	tptalgo::transform(kernel, reinterpret_cast<const unsigned char*>(src), reinterpret_cast<unsigned char*>(dest), (XRES/CELL)*(YRES/CELL));
}


#define CELLSDATA_INSTANTIATE(DataType) \
template class CellsData<DataType>;\
template void CellsData_fill<DataType>(DataType (*dest)[XRES/CELL], DataType value);\
template void CellsData_copy<DataType>(const DataType (*src)[XRES/CELL], DataType (*dest)[XRES/CELL]);

#define CELLSDATA_INSTANTIATE_NUMERIC(DataType) \
CELLSDATA_INSTANTIATE(DataType)\
template void CellsData_invert<DataType>(const DataType (*src)[XRES/CELL], DataType (*dest)[XRES/CELL]);


CELLSDATA_INSTANTIATE_NUMERIC(float)
CELLSDATA_INSTANTIATE_NUMERIC(unsigned char)



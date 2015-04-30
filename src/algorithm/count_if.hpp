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

#ifndef algorithm_count_if_h
#define algorithm_count_if_h

#include "algorithm/common.hpp"
#include <limits>

namespace tptalgo
{

namespace detail
{

class count_if
{
public:
	// bool impl::op(DataType x) - should return true if x matches the condition.
	// int impl::op(DataType x) - should return the number of matches in x.
	template<class DataType, class Kernel>
	static size_t single(const Kernel &kernel, const DataType * data, size_t data_length)
	{
		using KernelImpl = typename Kernel::template impl<DataType>;
		const KernelImpl kernelImpl(kernel);
		size_t count = 0;
		for (size_t i=0; i<data_length; i+=KernelImpl::step_items)
		{
			count += kernelImpl.op(kernel, data[i]);
		}
		return count;
	}
	template<class DataType, class Kernel>
	static size_t single(...) { throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel"); }

	template<class DataType, class Kernel, class opInputT>
	static bool alignData(size_t &count, size_t &i, const Kernel &kernel, const DataType * data, size_t data_length)
	{
		using KernelImpl = typename Kernel::template impl<opInputT>;
		// Check alignment of data
		const int minAlignment = KernelImpl::minAlignment;
		bool isAligned = (getAlignOffset_prev(data,minAlignment)==0);

		if ((KernelImpl::step_bytes % KernelImpl::minAlignment) != 0)
		{
			// alignment changes on each simd step, so always use unaligned loads
			return false;
		}
		else
		{
			int alignOffset = getAlignOffset_next(data,minAlignment);
			if (alignOffset!=0 && alignOffset % KernelImpl::step_bytes == 0)
			{
				// data is currently unaligned, but this can be resolved by taking single steps (for which alignment is assumed to not be required) until it is aligned
				size_t alignIterCount = alignOffset/KernelImpl::step_bytes;
				if (alignIterCount>data_length)
					alignIterCount = data_length;
				count += single(kernel, data, alignIterCount);
				i += alignIterCount*KernelImpl::step_items;
				return true;
			}
		}
		return isAligned;
	}

	template<class KernelImpl>
	class get_max_count
	{
		template<class KI> static std::integral_constant<uint64_t,KI::max_count> test(int unused);
		template<class KI> static std::integral_constant<uint64_t,0> test(...);
	public:
		static const size_t value = decltype(test<KernelImpl>(0))::value;
	};

#if HAVE_LIBSIMDPP
	
	template<class opInputT, class DataType, class Kernel, class KernelImpl=typename Kernel::template impl<opInputT>, class opOutputT=decltype(std::declval<KernelImpl>().op(std::declval<Kernel>(),std::declval<opInputT>()))>
	static opOutputT accumulate(const DataType *data, const int loopCount, const Kernel &kernel, const KernelImpl &kernelImpl, const bool isAligned)
	{
		const char *pdata = reinterpret_cast<const char*>(data);
		const char *pdatamax = pdata + loopCount*KernelImpl::step_bytes;
		opOutputT accum = opOutputT::zero();
		while (pdata<pdatamax)
		{
			opInputT x;
			if (isAligned)
				x = KernelImpl::load(pdata++);
			else
				x = KernelImpl::load_u(pdata++);
			accum = accum+kernelImpl.op(kernel, x);
		}
		return accum;
	}

	template<class opInputT, class DataType, class Kernel, class KernelImpl=typename Kernel::template impl<opInputT>, class opOutputT=decltype(std::declval<KernelImpl>().op(std::declval<Kernel>(),std::declval<opInputT>()))>
	static void accumulate_2(opOutputT &dest_accum_1, opOutputT &dest_accum_2, const DataType *data, const int loopCount, const Kernel &kernel, const KernelImpl &kernelImpl, const bool isAligned)
	{
		constexpr int unrollCount = 2;

		const char *pdata = reinterpret_cast<const char*>(data);
		const char *pdatamax = pdata + loopCount*unrollCount*KernelImpl::step_bytes;
		opOutputT accum_1 = opOutputT::zero();
		opOutputT accum_2 = opOutputT::zero();
		while (pdata<pdatamax)
		{
			opInputT x1, x2;
			if (isAligned)
			{
				x1 = KernelImpl::load(pdata);
				x2 = KernelImpl::load(pdata+KernelImpl::step_bytes);
			}
			else
			{
				x1 = KernelImpl::load_u(pdata);
				x2 = KernelImpl::load_u(pdata+KernelImpl::step_bytes);
			}
			pdata += KernelImpl::step_bytes*unrollCount;
			opOutputT c1 = kernelImpl.op(kernel, x1);
			opOutputT c2 = kernelImpl.op(kernel, x2);
			accum_1 = accum_1+c1;
			accum_2 = accum_2+c2;
		}
		dest_accum_1 = accum_1;
		dest_accum_2 = accum_2;
	}

	template<class opOutputT>
	static void add_accum(size_t &dest_count, opOutputT accum)
	{
		typename decltype(accum.eval())::element_type data[opOutputT::length];
		simdpp::store_u(data, accum);
		size_t count = 0;
		for (size_t i=0; i<opOutputT::length; i++)
		{
			count += data[i];
		}
		dest_count += count;
	}

	template<class opOutputT>
	static void add_accum_2(size_t &dest_count, opOutputT accum_1, opOutputT accum_2)
	{
		add_accum(dest_count, accum_1);
		add_accum(dest_count, accum_2);		
	}

#if defined(__SSE2__)
	template<unsigned N, class E>
	static typename std::enable_if<N%2==0,void>::type add_accum(size_t &dest_count, simdpp::uint32<N,E> accum)
	{
		using T = simdpp::uint32<N>;
		using BigT = simdpp::uint64<N/2>;
		add_accum(dest_count, BigT(simdpp::zip4_lo(accum, T::zero())) + BigT(simdpp::zip4_hi(accum, T::zero())));
	}
	template<unsigned N, class E>
	static typename std::enable_if<N%2==0,void>::type add_accum(size_t &dest_count, simdpp::uint16<N,E> accum)
	{
		using T = simdpp::uint16<N>;
		using BigT = simdpp::uint32<N/2>;
		add_accum(dest_count, BigT(simdpp::zip8_lo(accum, T::zero())) + BigT(simdpp::zip8_hi(accum, T::zero())));
	}
	template<unsigned N, class E>
	static typename std::enable_if<N%2==0,void>::type add_accum(size_t &dest_count, simdpp::uint8<N,E> accum)
	{
		using T = simdpp::uint8<N>;
		using BigT = simdpp::uint16<N/2>;
		add_accum(dest_count, BigT(simdpp::zip16_lo(accum, T::zero())) + BigT(simdpp::zip16_hi(accum, T::zero())));
	}

	template<unsigned N>
	static typename std::enable_if<N%2==0,void>::type add_accum_2(size_t &dest_count, simdpp::uint8<N> accum_1, simdpp::uint8<N> accum_2)
	{
		using T = simdpp::uint8<N>;
		using BigT = simdpp::uint16<N/2>;
		BigT big_accum_1 = BigT(simdpp::zip16_lo(accum_1, T::zero())) + BigT(simdpp::zip16_hi(accum_1, T::zero()));
		BigT big_accum_2 = BigT(simdpp::zip16_lo(accum_2, T::zero())) + BigT(simdpp::zip16_hi(accum_2, T::zero()));

		add_accum(dest_count, big_accum_1+big_accum_2);
	}
#endif
	
	
	// simdpp::uint*<n> impl::op(DataType x) - should return the number of matches in x. simdpp::uint*<n> = CountT
	template<class DataType, class Kernel, class opInputT>
	static size_t simdpp(const Kernel &kernel, const DataType *data, size_t data_length)
	{
		using KernelImpl = typename Kernel::template impl<opInputT>;
		using opOutputT = decltype(std::declval<KernelImpl>().op(std::declval<Kernel>(),std::declval<opInputT>()));

		constexpr typename opOutputT::element_type maxOutput = get_max_count<KernelImpl>::value;
		constexpr size_t maxAccumLoops = (maxOutput==0 ? 1 : std::numeric_limits<typename opOutputT::element_type>::max() / maxOutput);
		static_assert(maxAccumLoops!=0, "If defined, impl::max_count is the maximum value that can be returned in each element of the impl::op return value. It cannot be larger then the maximum value that can be stored in the element type.");
		
		const KernelImpl kernelImpl(kernel);
		size_t count = 0;//number of items meeting the condition
		size_t i = 0;//overall array position index

		// If possible, single step so that future loads will be aligned
		bool isAligned = alignData<DataType,Kernel,opInputT>(count, i, kernel, data, data_length);

		// Unrolled SIMD, adding results from each call to KernelImpl::op into accumulators until they are full. Then add accumulators to count.
		{
			constexpr int unrollCount = 2;
			constexpr size_t loop_steps = unrollCount*maxAccumLoops;// number of times KernelImpl::op will be called per loop iteration
			const size_t loops = (data_length-i)/(loop_steps*KernelImpl::step_items);// number of iterations to do
			if (loops)
			{
				const char *pdata = reinterpret_cast<const char*>(data+i);
				const char *pdatamax = pdata + loops*loop_steps*KernelImpl::step_bytes;
				while (pdata<pdatamax)
				{
					opOutputT accum_1, accum_2;
					accumulate_2<opInputT>(accum_1, accum_2, pdata, maxAccumLoops, kernel, kernelImpl, isAligned);
					add_accum_2(count, accum_1, accum_2);
					pdata += loop_steps*KernelImpl::step_bytes;
				}
				i += loops*loop_steps*KernelImpl::step_items;
			}
		}

		// Unrolled SIMD, adding results from each call to KernelImpl::op into accumulators which will never be full (since there are not enough data items left to fill them). Then add accumulators to count.
		{
			constexpr int unrollCount = 2;
			const size_t accumLoops = (data_length-i)/(unrollCount*KernelImpl::step_items);
			if (accumLoops)
			{
				opOutputT accum_1, accum_2;
				accumulate_2<opInputT>(accum_1, accum_2, data+i, accumLoops, kernel, kernelImpl, isAligned);
				add_accum_2(count, accum_1, accum_2);
				i += accumLoops*unrollCount*KernelImpl::step_items;
			}
		}

		// Non-unrolled SIMD
		{
			const size_t accumLoops = (data_length-i)/KernelImpl::step_items;
			if (accumLoops)
			{
				opOutputT accum = accumulate<opInputT>(data+i, accumLoops, kernel, kernelImpl, isAligned);
				add_accum(count, accum);
				i += accumLoops*KernelImpl::step_items;
			}
		}

		// Elements at end, not enough left for SIMD
		if (i<data_length)
		{
			count += single(kernel, data+i, data_length-i);
		}
		return count;
	}
#endif
	template<class DataType, class Kernel, class opInputT>
	static size_t simdpp(...) { throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel"); return 0; }
};

}
}


namespace tptalgo
{
template<class DataType, class Kernel>
size_t count_if(const Kernel &kernel, const DataType *data, size_t data_length)
{
	using opInputT = typename detail::select_impl<DataType, Kernel>::type;
	static_assert(!std::is_same<void,opInputT>::value, "No valid implementation could be found in the supplied kernel");
	if (HAVE_LIBSIMDPP)
	{
		return detail::count_if::simdpp<DataType, Kernel, opInputT>(kernel, data, data_length);
	}
	else
	{
		return detail::count_if::single<DataType, Kernel>(kernel, data, data_length);
	}
}
}


#endif

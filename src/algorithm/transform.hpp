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

#ifndef algorithm_transform_h
#define algorithm_transform_h

#include "algorithm/common.hpp"

namespace tptalgo
{
namespace detail
{

class transform
{
public:
	// DataType impl::op(DataType x) - should return transformed value of x
	template<class DataType, class Kernel>
	static void single(const Kernel &kernel, const DataType * src, DataType * dest, size_t data_length)
	{
		using KernelImpl = typename Kernel::template impl<DataType>;
		const KernelImpl kernelImpl(kernel);
		for (size_t i=0; i<data_length; i+=KernelImpl::step_items)
		{
			dest[i] = kernelImpl.op(kernel, src[i]);
		}
	}
	template<class DataType, class Kernel>
	static size_t single(...) { throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel"); }


#if HAVE_LIBSIMDPP
	// simd_unroll_helper: a recursive template that will execute a load/op/store unrollCount times
	/* This method of unrolling seems to give the best assembly output.
	 * Alternatives:
	 *  SimdT x[unrollCount] and loop - poor code generation in clang. It stores and loads x values to the stack instead of keeping them in registers.
	 *  for (int j=0; j<unrollCount; j++) - poor instruction scheduling. Ideally loads, ops, stores should each be grouped together.
	 *  Manual unrolling - unroll count not configurable by Kernel.
	 */
	template<int unrollCount, class DataType, class Kernel, class SimdT>
	class simd_unroll_helper
	{
	public:
		using KernelImpl = typename Kernel::template impl<SimdT>;
		static const int ptrOffset = (unrollCount-1)*KernelImpl::step_bytes;
		simd_unroll_helper<unrollCount-1, DataType, Kernel, SimdT> prevIter;
		SimdT x;
		void load(const char *psrc)
		{
			prevIter.load(psrc);
			x = KernelImpl::load(psrc+ptrOffset);
		}
		void load_u(const char *psrc)
		{
			prevIter.load_u(psrc);
			x = KernelImpl::load_u(psrc+ptrOffset);
		}
		void op(const Kernel &kernel, const KernelImpl &kernelImpl)
		{
			prevIter.op(kernel, kernelImpl);
			x = kernelImpl.op(kernel, x);
		}
		void store(char *pdest)
		{
			prevIter.store(pdest);
			KernelImpl::store(pdest+ptrOffset, x);
		}
		void store_u(char *pdest)
		{
			prevIter.store_u(pdest);
			KernelImpl::store_u(pdest+ptrOffset, x);
		}
	};
	// stop after unrollCount recursions of template
	template<class DataType, class Kernel, class SimdConf>
	class simd_unroll_helper<0, DataType, Kernel, SimdConf>
	{
public:
		void load(...) {}
		void load_u(...) {}
		void op(...) {}
		void store(...) {}
		void store_u(...) {}
	};


	// T impl::op(T x) - should return transformed values of x
	template<class DataType, class Kernel, class SimdT>
	static typename std::enable_if<!std::is_same<void,SimdT>::value, void>::type simdpp(const Kernel &kernel, const DataType *src, DataType *dest, size_t data_length)
	{
		using KernelImpl = typename Kernel::template impl<SimdT>;
		const int unrollCount = KernelImpl::unrollCount;
		const int minAlignment = KernelImpl::minAlignment;

		const KernelImpl kernelImpl(kernel);
		size_t i = 0;//overall array position index

		// Check alignment of data
		bool srcAligned = (getAlignOffset_prev(src,minAlignment)==0);
		bool destAligned = (getAlignOffset_prev(dest,minAlignment)==0);
		
		if ((KernelImpl::step_bytes % KernelImpl::minAlignment) != 0)
		{
			// alignment changes on each simd step, so always use unaligned loads and stores
			srcAligned = destAligned = false;
		}
		else
		{
			const int step_bytes = KernelImpl::step_bytes;
			const int step_items = KernelImpl::step_items;
			int srcAlignOffset = getAlignOffset_next(src,minAlignment);
			if (srcAlignOffset!=0 && srcAlignOffset % step_bytes == 0)
			{
				// src is currently unaligned, but this can be resolved by taking single steps (for which alignment is assumed to not be required) until it is aligned
				size_t alignIterCount = srcAlignOffset/step_bytes;
				if (alignIterCount>data_length)
					alignIterCount = data_length;
				single(kernel, src, dest, alignIterCount);
				i += alignIterCount*step_items;

				srcAligned = true;
				destAligned = (getAlignOffset_diff(src, dest, minAlignment)==0); // true if src and dest have the same alignment
			}
		}

		const char *psrc = reinterpret_cast<const char*>(src+i);
		char *pdest = reinterpret_cast<char*>(dest+i);
		size_t maxLoops = (data_length-i)/(unrollCount*KernelImpl::step_items);
		const char *psrcmax = psrc + maxLoops*unrollCount*KernelImpl::step_bytes;
		i += maxLoops*unrollCount*KernelImpl::step_items;
		while (psrc<psrcmax)
		{
			simd_unroll_helper<KernelImpl::unrollCount, DataType, Kernel, SimdT> unrolled;
			if (srcAligned)
				unrolled.load(psrc);
			else
				unrolled.load_u(psrc);

			unrolled.op(kernel, kernelImpl);

			if (destAligned)
				unrolled.store(pdest);
			else
				unrolled.store_u(pdest);

			psrc += unrollCount*KernelImpl::step_bytes, pdest += unrollCount*KernelImpl::step_bytes;
		}

		// Elements at end, not enough for unrolled simd, but maybe enough for one simd operation at a time
		maxLoops = (data_length-i)/KernelImpl::step_items;
		psrcmax = psrc + maxLoops*KernelImpl::step_bytes;
		i += maxLoops*KernelImpl::step_items;
		for (; psrc<psrcmax; psrc+=KernelImpl::step_bytes, pdest+=KernelImpl::step_bytes)
		{
			SimdT x;
			if (srcAligned)
				x = KernelImpl::load(psrc);
			else
				x = KernelImpl::load_u(psrc);

			x = kernelImpl.op(kernel, x);

			if (destAligned)
				KernelImpl::store(pdest, x);
			else
				KernelImpl::store_u(pdest, x);
		}

		// Elements at end, not enough left for simd
		if (i<data_length)
		{
			single(kernel, src+i, dest+i, data_length-i);
		}
	}
#endif
	template<class DataType, class Kernel, class SimdT>
	static void simdpp(...) {
		static_assert(!std::is_same<void,SimdT>::value, "tptalgo: the selected algorithm implementation is not supported by the supplied kernel");
		throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel");
	}
};

}
}

namespace tptalgo
{

template<class DataType, class Kernel>
void transform(const Kernel &kernel, const DataType *src, DataType *dest, size_t data_length)
{
	using SimdT = typename detail::select_impl<DataType, Kernel>::type;
	static_assert(!std::is_same<void,SimdT>::value, "No valid implementation could be found in the supplied kernel");
	if (HAVE_LIBSIMDPP)
	{
		detail::transform::simdpp<DataType, Kernel, SimdT>(kernel, src, dest, data_length);
	}
	else
	{
		detail::transform::single<DataType, Kernel>(kernel, src, dest, data_length);
	}
}

}


#endif

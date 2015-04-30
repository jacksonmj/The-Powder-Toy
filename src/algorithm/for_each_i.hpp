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

#ifndef algorithm_foreachi_h
#define algorithm_foreachi_h

#include "algorithm/common.hpp"

namespace tptalgo
{

typedef Kernel_impl_base_invalid Kernel_i_impl_base_invalid;

class Kernel_i_impl_notfound
{
	static constexpr bool implValid = false;
};

template<size_t StepItems, class Kernel>
class Kernel_i_impl_base
{
public:
	static constexpr size_t step_items = StepItems;
	static constexpr bool implValid = true;
	bool isAligned(...) const { return false; }
	bool canAlign(...) const { return false; }
};

#define TPTALGO_KERN_I_COMMON(KernelName) \
public:\
	template<size_t ImplPriority, class Kernel=KernelName>\
	class i_impl : public tptalgo::Kernel_i_impl_base_invalid {};\
	static std::false_type select_i_impl(tptalgo::overload_priority<0>);\
	static std::false_type select_i_impl_bySize(tptalgo::overload_priority<0>, tptalgo::overload_priority<0>);\

#define TPTALGO_KERN_I_IMPL(StepItems, ImplPriority) \
public:\
	static std::integral_constant<size_t, ImplPriority> select_i_impl(tptalgo::overload_priority<ImplPriority>);\
	static std::integral_constant<size_t, ImplPriority> select_i_impl_bySize(tptalgo::overload_priority<StepItems>, tptalgo::overload_priority<ImplPriority>);\
	template<class Kernel>\
	class i_impl<ImplPriority, Kernel> : public tptalgo::Kernel_i_impl_base<StepItems, Kernel>

class Kernel_i_base
{};


namespace detail
{

class for_each_i
{
public:
	template<class Kernel>
	class select_impl_single
	{
	public:
		using impl_priority = decltype(Kernel::select_i_impl_bySize(overload_priority<1>(nullptr), overload_priority<100>(nullptr)));
		using impl_tmp = typename Kernel::template i_impl<impl_priority::value, Kernel>;
		static constexpr bool found = !std::is_same<impl_priority, std::false_type>::value;
		using type = typename std::conditional<found, impl_tmp, Kernel_i_impl_notfound>::type;
	};
	template<class Kernel>
	class select_impl_multi
	{
	public:
		using impl_priority = decltype(Kernel::select_i_impl(overload_priority<100>(nullptr)));
		using impl_tmp = typename Kernel::template i_impl<impl_priority::value, Kernel>;
		static constexpr bool found = !std::is_same<impl_priority, std::false_type>::value;
		using type = typename std::conditional<found, impl_tmp, Kernel_i_impl_notfound>::type;
	};

	template<class Kernel>
	static typename std::enable_if<select_impl_single<Kernel>::found, void>::type single(const Kernel &kernel, size_t start, size_t end)
	{
		using KernelImpl = typename select_impl_single<Kernel>::type;

		const KernelImpl kernelImpl(kernel);
		const bool isAligned = kernelImpl.isAligned(kernel, start);
		for (size_t i=start; i<end; i++)
		{
			kernelImpl.i_op(kernel, i, isAligned);
		}
	}
	template<class Kernel>
	static void single(const Kernel &kernel, ...)
	{
		static_assert(select_impl_single<Kernel>::found, "tptalgo: no single step implementation found");
		throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel");
	}

	template<class Kernel>
	static typename std::enable_if<select_impl_single<Kernel>::found && select_impl_multi<Kernel>::found, void>::type multi(const Kernel &kernel, size_t start, size_t end)
	{
		using KernelImpl_single = typename select_impl_single<Kernel>::type;
		using KernelImpl = typename select_impl_multi<Kernel>::type;

		size_t length = end-start;
		if (length <= KernelImpl::step_items)
		{
			single(kernel, start, end);
			return;
		}

		const KernelImpl_single kernelImpl_single(kernel);
		const KernelImpl kernelImpl(kernel);

		size_t i=start;
		bool isAligned = kernelImpl.isAligned(kernel, i);
		if (!isAligned && kernelImpl.canAlign(kernel))
		{
			size_t j;
			for (j=0; j<KernelImpl::step_items; i++, j++)
			{
				kernelImpl_single.i_op(kernel, i, false);
				isAligned = kernelImpl.isAligned(kernel, i);
				if (isAligned)
					break;
			}
		}

		size_t end_multi = (end-i)/KernelImpl::step_items * KernelImpl::step_items + i;
		for (; i<end_multi; i+=KernelImpl::step_items)
		{
			kernelImpl.i_op(kernel, i, isAligned);
		}
		

		for (; i<end; i++)
		{
			kernelImpl_single.i_op(kernel, i, false);
		}
	}
	template<class Kernel>
	static void multi(const Kernel &kernel, ...)
	{
		static_assert(select_impl_multi<Kernel>::found, "tptalgo: no implementation found");
		static_assert(select_impl_single<Kernel>::found, "tptalgo: no single step implementation found");
		throw std::logic_error("tptalgo: the selected algorithm implementation is not supported by the supplied kernel");
	}
};

}
}

namespace tptalgo
{

template<class Kernel>
void for_each_i(const Kernel &kernel, size_t start, size_t end)
{
	detail::for_each_i::multi(kernel, start, end);
}
template<class Kernel>
void for_each_i(const Kernel &kernel, size_t length)
{
	for_each_i(kernel, 0, length);
}

}


#endif

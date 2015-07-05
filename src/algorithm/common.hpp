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

#ifndef algorithm_common_h
#define algorithm_common_h

#include "common/Compat.hpp"
#include "common/Intrinsics.hpp"
#include "common/AlignedAlloc.hpp"
#include "common/tpt-stdint.h"
#include <type_traits>
#include <stdexcept>

namespace tptalgo
{

/* overload_priority<P> is a pointer to a class that has P levels of inheritance between it and the base class (overload_priority_helper<0>).
 * For an overloaded function with an argument of type overload_priority<P>, overload resolution selects the function which requires the fewest Derived->Base conversions of the supplied argument. 
 * If calling the function with an argument of overload_priority<something large> (i.e. a pointer to a very derived class), the function with the most derived class pointer is selected.
 * The function selected is therefore the one with the largest P inside the overload_priority<P>
 */
namespace detail
{
template<unsigned P> class overload_priority_helper : public overload_priority_helper<P-1> {};
template<> class overload_priority_helper<0> {};
}
template<unsigned P> using overload_priority = typename detail::overload_priority_helper<P>*;


class Kernel_impl_base_invalid
{
public:
	static constexpr bool implValid = false;
};

template<class T, class Kernel>
class Kernel_impl_base
{
public:
	static constexpr size_t step_items = 1;
	static constexpr size_t step_bytes = sizeof(T);
	static constexpr size_t minAlignment = sizeof(T);
	static constexpr unsigned unrollCount = 2;
	static constexpr bool implValid = true;

	static T load(const char *src)
	{
		return *reinterpret_cast<const T*>(src);
	}
	static T load_u(const char *src)
	{
		return *reinterpret_cast<const T*>(src);
	}
	static void store(char *dest, T data)
	{
		*reinterpret_cast<T*>(dest) = data;
	}
	static void store_u(char *dest, T data)
	{
		*reinterpret_cast<T*>(dest) = data;
	}
};

#if HAVE_LIBSIMDPP
template<class T, class Kernel>
class Kernel_impl_base_simdpp : public Kernel_impl_base<T, Kernel>
{
public:
	static constexpr size_t step_items = T::length;
	static constexpr size_t step_bytes = T::length_bytes;
	static constexpr size_t minAlignment = T::length_bytes;

	static T load(const char *src)
	{
		return simdpp::load(src);
	}
	static T load_u(const char *src)
	{
		return simdpp::load_u(src);
	}
	static void store(char *dest, T data)
	{
		simdpp::store(dest, data);
	}
	static void store_u(char *dest, T data)
	{
		simdpp::store_u(dest, data);
	}
};
#endif


#define TPTALGO_KERN_COMMON(KernelName) \
public:\
	template<typename T, class Kernel=KernelName>\
	class impl : public tptalgo::Kernel_impl_base_invalid {};

#define TPTALGO_KERN_IMPL(DataType) \
public:\
	template<class Kernel>\
	class impl<DataType, Kernel> : public tptalgo::Kernel_impl_base<DataType, Kernel>

#define TPTALGO_KERN_IMPL_SIMDPP(DataType, DataTypeSize) \
public:\
	template<unsigned DataTypeSize, class Kernel>\
	class impl<DataType<DataTypeSize>, Kernel> : public tptalgo::Kernel_impl_base_simdpp< DataType<DataTypeSize>, Kernel>

class Kernel_base
{};


namespace detail
{

template<class Kernel, typename ImplDataType>
class has_impl {
	template<class K, typename T> static typename std::enable_if<K::template impl<T>::implValid, std::true_type>::type test(int a);
	template<class K, typename T> static std::false_type test(...);
public:
	static const bool value = decltype(test<Kernel, ImplDataType>(0))::value;
};

#define TPTALGO_TYPESELECT_LIST_START(DataType) \
template<class Kernel>\
class select_impl<DataType, Kernel>\
{\
	template<class K> static void t(...);\
	template<class K> static typename std::enable_if<has_impl<K, DataType>::value, DataType>::type t(overload_priority<10>);

#define	TPTALGO_TYPESELECT_OPTION(T, Priority) /* larger Priority value = higher priority */\
	template<class K> static typename std::enable_if<has_impl<K, T>::value, T>::type t(overload_priority<Priority>);

#define TPTALGO_TYPESELECT_OPTION_SIMDPP(T, Priority) /* larger Priority value = higher priority */\
	template<class K> static typename std::enable_if<has_impl<K, T>::value, T>::type t(overload_priority<Priority>);

#define TPTALGO_TYPESELECT_LIST_END() \
public:\
	using type = decltype(t<Kernel>(overload_priority<100>(nullptr)));\
};

#ifndef HAVE_LIBSIMDPP
#define TPTALGO_TYPESELECT_OPTION_SIMDPP(T, Priority) /* */
#endif

/* select_impl: selection of the widest natively available type which is supported by a particular Kernel.
 *
 * Partial template specialisation of class select_impl is used to select the list of simdpp types which match a given DataType. 
 * SFINAE (to disable types which the Kernel does not support) and function overloading (overload_priority<P> - selects highest priority type available) within that class is then used to select the best available type in that list.
 *
 * Each list automatically includes the implementation for the list DataType with a priority of 10.
 */

template<class DataType, class Kernel>
class select_impl
{
public:
	using type = DataType;
};

TPTALGO_TYPESELECT_LIST_START(float)
TPTALGO_TYPESELECT_OPTION(double, 1) // allow using double function for float data, but lower priority than float function
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float32<4>, 20)
#endif
#ifdef SIMDPP_USE_AVX
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float32<8>, 21)
#endif
#ifdef SIMDPP_USE_AVX512
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float32<16>, 22)
#endif
TPTALGO_TYPESELECT_LIST_END()

TPTALGO_TYPESELECT_LIST_START(double)
TPTALGO_TYPESELECT_OPTION(float, 1) // allow using float function for double data, but lower priority than double function
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON64)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float64<2>, 20)
#endif
#ifdef SIMDPP_USE_AVX
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float64<4>, 21)
#endif
#ifdef SIMDPP_USE_AVX512
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::float64<8>, 22)
#endif
TPTALGO_TYPESELECT_LIST_END()

TPTALGO_TYPESELECT_LIST_START(uint8_t)
TPTALGO_TYPESELECT_OPTION(uint16_t, 11)
TPTALGO_TYPESELECT_OPTION(uint32_t, 12)
TPTALGO_TYPESELECT_OPTION(uint64_t, 13)
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint8<16>, 20)
#endif
#ifdef SIMDPP_USE_AVX2
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint8<32>, 21)
#endif
TPTALGO_TYPESELECT_LIST_END()

TPTALGO_TYPESELECT_LIST_START(uint16_t)
TPTALGO_TYPESELECT_OPTION(uint32_t, 12)
TPTALGO_TYPESELECT_OPTION(uint64_t, 13)
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint16<8>, 20)
#endif
#ifdef SIMDPP_USE_AVX2
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint16<16>, 21)
#endif
TPTALGO_TYPESELECT_LIST_END()

TPTALGO_TYPESELECT_LIST_START(uint32_t)
TPTALGO_TYPESELECT_OPTION(uint64_t, 13)
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint32<4>, 20)
#endif
#ifdef SIMDPP_USE_AVX2
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint32<8>, 21)
#endif
#ifdef SIMDPP_USE_AVX512
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint32<16>, 22)
#endif
TPTALGO_TYPESELECT_LIST_END()

TPTALGO_TYPESELECT_LIST_START(uint64_t)
#if defined(SIMDPP_USE_SSE2) || defined(SIMDPP_USE_NEON)
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint64<2>, 20)
#endif
#ifdef SIMDPP_USE_AVX2
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint64<4>, 21)
#endif
#ifdef SIMDPP_USE_AVX512
TPTALGO_TYPESELECT_OPTION_SIMDPP(simdpp::uint64<8>, 22)
#endif
TPTALGO_TYPESELECT_LIST_END()

}
}

#endif

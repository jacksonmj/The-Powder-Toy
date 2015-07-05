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

#ifndef Intrinsics_h
#define Intrinsics_h

#if defined(_MSC_VER)
	#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
	#include <x86intrin.h>
#endif

#ifndef HAVE_LIBSIMDPP
#define HAVE_LIBSIMDPP 0
#endif

// TODO: remove
#undef X86_SSE2
#undef X86


#if defined(__SSE2__)
#define SIMDPP_ARCH_X86_SSE2 1
#endif
#if defined(__SSE3__)
#define SIMDPP_ARCH_X86_SSE3 1
#endif
#if defined(__SSSE3__)
#define SIMDPP_ARCH_X86_SSSE3 1
#endif
#if defined(__SSE4_1__)
#define SIMDPP_ARCH_X86_SSE4_1 1
#endif
#if defined(__AVX__)
#define SIMDPP_ARCH_X86_AVX 1
#endif
#if defined(__AVX2__)
#define SIMDPP_ARCH_X86_AVX2 1
#endif
#if defined(__ARM_NEON__)
#define SIMDPP_ARCH_ARM_NEON_FLT_SP 1
#endif

#if HAVE_LIBSIMDPP
#include "simdpp/setup_arch.h"
#include "simdpp/simd.h"
#endif

#endif

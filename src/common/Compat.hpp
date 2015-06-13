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

#ifndef Compat_h
#define Compat_h

// Definitions and functions for compatibility with different compilers/platforms

#if defined(__GNUC__)
// GCC
#define TPT_RESTRICT __restrict
#define TPT_NOINLINE __attribute__ ((noinline))
#define TPT_INLINE inline
#define TPT_FORCEINLINE __attribute__((always_inline))

#elif defined(_MSC_VER)
// Visual Studio
#define TPT_RESTRICT
#define TPT_NOINLINE
#define TPT_INLINE inline
#define TPT_FORCEINLINE __forceinline

#else
#define TPT_RESTRICT
#define TPT_NOINLINE
#define TPT_INLINE
#define TPT_FORCEINLINE

#endif


#if defined(WIN) && defined(__GNUC__)
#define TH_ENTRY_POINT __attribute__((force_align_arg_pointer))
#else
#define TH_ENTRY_POINT
#endif


// TODO: remove this
#define TPT_GNU_INLINE

#endif

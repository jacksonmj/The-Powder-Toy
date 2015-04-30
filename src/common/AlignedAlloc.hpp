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

#ifndef AlignedAlloc_h
#define AlignedAlloc_h

#include <cstdlib>
#include "common/tpt-stdint.h"

void *tpt_aligned_alloc(size_t alignment, size_t size);
void tpt_aligned_free(void *ptr);

// Functions to calculate how unaligned a pointer is
template<class DataType>
constexpr int getAlignOffset_prev(const DataType *ptr, size_t neededAlignment)
{
	// Calculate how many bytes a pointer is after the previous aligned address
	return uintptr_t(ptr) % neededAlignment;
}
template<class DataType>
constexpr int getAlignOffset_next(const DataType *ptr, size_t neededAlignment)
{
	// Calculate how many bytes a pointer is before the next aligned address
	return (neededAlignment - getAlignOffset_prev(ptr, neededAlignment)) % neededAlignment;
}

template<class DataType>
constexpr int getAlignOffset_diff(const DataType *ptr1, const DataType *ptr2, size_t neededAlignment)
{
	// Calculate difference in alignment offset between two pointers
	return getAlignOffset_prev(ptr1, neededAlignment)-getAlignOffset_prev(ptr2, neededAlignment);
}


#endif

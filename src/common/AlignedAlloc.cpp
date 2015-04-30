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

#include "common/AlignedAlloc.hpp"

#if defined(_ISOC11_SOURCE) && _ISOC11_SOURCE

void *tpt_aligned_alloc(size_t alignment, size_t size)
{
	return aligned_alloc(alignment, size);
}

void tpt_aligned_free(void *ptr)
{
	free(ptr);
}

#elif defined(_POSIX_C_SOURCE) && defined(_XOPEN_SOURCE) && (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)

void *tpt_aligned_alloc(size_t alignment, size_t size)
{
	void *tmp = nullptr;
	posix_memalign(&tmp, alignment, size);
	return tmp;
}

void tpt_aligned_free(void *ptr)
{
	free(ptr);
}

#elif defined(_MSC_VER)

#include <malloc.h>

void *tpt_aligned_alloc(size_t alignment, size_t size)
{
	return _aligned_malloc(size, alignment);
}

void tpt_aligned_free(void *ptr)
{
	_aligned_free(ptr);
}

#else

#error No aligned alloc implementation available.

#endif


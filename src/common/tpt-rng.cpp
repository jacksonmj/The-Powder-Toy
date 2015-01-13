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

#include "common/tpt-rng.h"
#include <time.h>

tpt_rng::tpt_rng()
{
	gen.seed(time(NULL));
}

uint_fast32_t tpt_rng::randUint32()
{
	return gen();
}

uint_fast64_t tpt_rng::randUint64()
{
	return (uint_fast64_t(gen())<<32) | gen();
}

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

#ifndef Simulation_Elements_WIRE_h
#define Simulation_Elements_WIRE_h

#include "simulation/Particle.h"

class Element_WIRE
{
public:
	/*
	0:  inactive wire
	1:  spark head
	2:  spark tail
	3:  sparked by external source, acts as though it was spark head except it will not be overwritten by CA rules in WIRE_update

	tmp is previous state, ctype is current state
	*/

	enum class State
	{
		Inactive=0,
		HeadNormal=1,
		Tail=2,
		HeadExternal=3
	};

	// Check whether the particle state on the previous frame matched a certain condition
	static bool wasInactive(const particle &p)
	{
		return !p.tmp;
	}
	static bool wasHead(const particle &p)
	{
		return p.tmp&1;
	}
	static bool wasTail(const particle &p)
	{
		return p.tmp==2;
	}
	static bool wasHeadNormal(const particle &p)
	{
		return p.tmp==1;
	}
	static bool wasHeadExternal(const particle &p)
	{
		return p.tmp==3;
	}

	// Check whether the particle state on the current frame matches a certain condition
	// NB: during Simulation::Update, these are more like unfinished calculations for the next frame. They only represent the current state when Simulation::Update is not running.
	// For WIRE influencing other particles, use the was* functions to avoid being bitten by particle order. E.g. during sim update, wasHead(p) => spark neighbouring conductor.
	// For other particles influencing WIRE, a combination of is* and was* functions are likely to be required. canSpark() and spark() can be used for this, instead of using the is*/was* functions directly.
	static bool isInactive(const particle &p)
	{
		return !p.ctype;
	}
	static bool isHead(const particle &p)
	{
		return p.ctype&1;
	}
	static bool isTail(const particle &p)
	{
		return p.ctype==2;
	}
	static bool isHeadNormal(const particle &p)
	{
		return p.ctype==1;
	}
	static bool isHeadExternal(const particle &p)
	{
		return p.ctype==3;
	}

	static void setState(particle &p, State newState)
	{
		p.ctype = static_cast<int>(newState);
	}
	static bool canSpark(const particle &p)
	{
		return (wasInactive(p) && isInactive(p));
	}
	static void spark_noCheck(particle &p)
	{
		// sets particle state to "triggered by external spark source"
		setState(p, State::HeadExternal);
	}
	static void spark(particle &p)
	{
		if (canSpark(p))
		{
			spark_noCheck(p);
		}
	}
};

#endif

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

#ifndef simulation_ParticleMapIterator_h
#define simulation_ParticleMapIterator_h

#include "simulation/ParticleMap.hpp"

#include <iterator>

/*
Class to iterate through all the particle IDs at a given location
in a slightly but not entirely standard-C++-like manner.
A bit slower than a FOR_PMAP_POS loop.

Equivalent of it.begin() is  it=ParticleMapEntry::iterator(globalSim, PMapCategory::All, pos)
 - TODO: maybe add .begin(category) method to pmap struct
Equivalent of it!=.end() is  !it.finished()


Example:
	for (auto it = ParticleMapEntry::iterator(globalSim, PMapCategory::All, pos); !it.finished(); ++it)
		globalSim->parts[*it].ctype = 1234;

*/

class ParticleMapEntry::iterator : public std::iterator<std::forward_iterator_tag,int>
{
protected:
	int i, count, next;
	particle *parts;
public:
	iterator(ParticleMapEntry &pmape, particle *parts_, PMapCategory c) :
		i(pmape.first(c)), count(pmape.count(c)), next(parts_[i].pmap_next), parts(parts_)
	{}
	iterator(ParticleMap &pmap, particle *parts_, PMapCategory c, SimPosI pos) :
		iterator(pmap(pos), parts_, c)
	{}
	iterator(Simulation *sim, PMapCategory c, SimPosI pos) :
		iterator(sim->pmap(pos), sim->parts, c)
	{}

	iterator& operator++()
	{
		if (count>0)
		{
			i = next;
			next = parts[i].pmap_next;
			count--;
		}
		return *this;
	}
	iterator operator++(int) {iterator tmp(*this); operator++(); return tmp;}
	int operator*() const {return i;}
	bool finished() const {return (count<=0);}
};


#endif
 

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

#ifndef Simulation_PmapIterator_h
#define Simulation_PmapIterator_h

#include "simulation/Simulation.h"

#include <iterator>

/*
Class to iterate through all the particle IDs at a given x,y location
in a slightly but not entirely standard-C++-like manner.
A bit slower than the FOR_PMAP_POSITION loops provided by Simulation.h

Equivalent of it.begin() is  it=PmapIterator::All(sim, x, y)
 - TODO: maybe add .begin_All() etc methods to pmap struct
Equivalent of it!=.end() is  !it.finished()
 - Something could maybe be done with .end() returning an iterator
   which when compared to an iterator it, checks it.count==0. However,
   this could cause complications, since allowing comparisons means we
   need to consider how to compare every possible PmapIterator - just
   check count, or check count, i, x, y, and All/EnergyOnly/NoEnergy,
   or some other combination of variables?


Example:

for (PmapIterator it = PmapIterator::All(sim, x, y); !it.finished(); ++it)
	sim->parts[*it].ctype = 1234;

PmapIterator::All(sim, x, y) - all particles at a coordinate
PmapIterator::EnergyOnly(sim, x, y) - all energy particles at a coordinate
PmapIterator::NoEnergy(sim, x, y) - all non-energy particles at a coordinate
*/

class PmapIterator : public std::iterator<std::forward_iterator_tag,int>
{
protected:
	const Simulation *sim;
	int x, y;
	int count, i, next;
	PmapIterator(const Simulation *sim_, int x_, int y_) :
		sim(sim_), x(x_), y(y_) {}
public:
	static PmapIterator All(const Simulation *sim, int x, int y)
	{
		PmapIterator it(sim, x, y);
		it.count = sim->pmap[y][x].count;
		if (it.count)
		{
			it.i = sim->pmap[y][x].first;
			it.next = sim->parts[it.i].pmap_next;
		}
		else
		{
			it.i = it.next = -1;
		}
		return it;
	}
	static PmapIterator EnergyOnly(const Simulation *sim, int x, int y)
	{
		PmapIterator it(sim, x, y);
		it.count = sim->pmap[y][x].count - sim->pmap[y][x].count_notEnergy;
		if (it.count)
		{
			it.i = sim->pmap[y][x].first_energy;
			it.next = sim->parts[it.i].pmap_next;
		}
		else
		{
			it.i = it.next = -1;
		}
		return it;
	}
	static PmapIterator NoEnergy(const Simulation *sim, int x, int y)
	{
		PmapIterator it(sim, x, y);
		it.count = sim->pmap[y][x].count_notEnergy;
		if (it.count)
		{
			it.i = sim->pmap[y][x].first;
			it.next = sim->parts[it.i].pmap_next;
		}
		else
		{
			it.i = it.next = -1;
		}
		return it;
	}
	PmapIterator(const PmapIterator& it) : sim(it.sim), x(it.x), y(it.y), count(it.count), i(it.i), next(it.next) {}
	PmapIterator& operator++()
	{
		if (count)
		{
			i = next;
			next = sim->parts[i].pmap_next;
			count--;
		}
		else
		{
			i = next = -1;
		}
		return *this;
	}
	PmapIterator operator++(int) {PmapIterator tmp(*this); operator++(); return tmp;}
	int operator*() const {return i;}
	bool finished() const {return (count<=0);}
};


#endif
 

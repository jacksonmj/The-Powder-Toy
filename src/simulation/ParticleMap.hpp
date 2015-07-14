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

#ifndef simulation_ParticleMap_h
#define simulation_ParticleMap_h

#include "simulation/SimulationSharedData.h"
#include "simulation/Position.hpp"
#include "common/tpt-stdint.h"
#include <stdexcept>

/* The particle map is basically a large number of linked lists, one per integer coordinate.
 * 
 * The items in each list are particles. particle.pmap_prev and particle.pmap_next are the particle IDs of the prev/next particle at that coordinate.
 * Each list is sorted into energy and not-energy particles (but with the two sections still connected together as a single list).
 * The length and first particle ID in each section can be obtained from the ParticleMapEntry for that coordinate. This allows iterating over energy, not-energy, or all particles (not-energy first, then energy) in a particular position.
 * 
 * Particles within each section are not necessarily in any particular order.
 */


/* DETAILS, mostly unimportant when using pmap, only when modifying pmap code itself.
 * 
 * Not-energy particles are first in the list.
 * ParticleMapEntry.count is the total number of particles in the list.
 * ParticleMapEntry.count_notEnergy is the total number of not-energy particles in the list.
 * Particles are added to the list by prepending to each section.
 * 
 * If energy particles are present:
 * ParticleMapEntry.first is the ID of the first particle in the list (may be energy or not-energy, depending on whether not-energy particles are present).
 * ParticleMapEntry.first_energy is the ID of the first energy particle in the list.
 * 
 * If no energy particles are present, ParticleMapEntry.first_energy indicates the tail of the list.
 *
 *
 * /- count
 * |
 * | /- count_notEnergy
 * | |
 * @ @ not-energy --- first
 * | | not-energy
 * | @ not-energy
 * |   energy ------- first_energy
 * |   energy
 * |   energy
 * @   energy
 *
 *
 * /- count
 * |
 * | /- count_notEnergy = 0
 * |
 * @   energy ------- first = first_energy
 * |   energy
 * |   energy
 * @   energy
 *
 *
 * /- count
 * |
 * | /- count_notEnergy
 * | |
 * @ @ not-energy --- first
 * | | not-energy
 * @ @ not-energy --- first_energy (NB: tail of list)
 *
 */


#define FOR_PMAP_POS_NDECL(pmap, parts, category, pos, r_count, r_i, r_next) for (r_count=(pmap)(pos).count(category), r_next=(pmap)(pos).first(category); r_count ? (r_i=r_next, r_next=(parts)[r_i].pmap_next, true):false; r_count--)

#define FOR_PMAP_POS(pmap, parts, category, pos, r_i) for (int r_i, pmapit_count=(pmap)(pos).count(category), pmapit_next=(pmap)(pos).first(category); pmapit_count ? (r_i=pmapit_next, pmapit_next=(parts)[r_i].pmap_next, true):false; pmapit_count--)


enum class PMapCategory
{
	Energy=1,
	NotEnergy,
	All
};

class PMapCategory_single
{
protected:
	PMapCategory c;
public:
	PMapCategory_single(PMapCategory c_) : c(c_)
	{
		if (c==PMapCategory::All)
			throw std::invalid_argument("Single category required, not PMapCategory::All");
	}
	operator PMapCategory() { return c; }
};

class ParticleMapEntry
{
public:
	class iterator;
protected:
	uint_least32_t count_;// number of particles, including energy particles
	uint_least32_t count_notEnergy_;// number of particles, not including energy particles
public:
	int first_;// ID of first particle
	int first_energy_;// ID of first energy particle, or if there are no energy particles, ID of last non-energy particle

	void incCount(PMapCategory_single c)
	{
		count_++;
		if (c==PMapCategory::NotEnergy)
			count_notEnergy_++;
	}
	void decCount(PMapCategory_single c)
	{
		count_--;
		if (c==PMapCategory::NotEnergy)
			count_notEnergy_--;
	}

public:
	int count(PMapCategory c=PMapCategory::All) const
	{
		switch (c)
		{
		case PMapCategory::All:
		default:
			return count_;
		case PMapCategory::Energy:
			return count_-count_notEnergy_;
		case PMapCategory::NotEnergy:
			return count_notEnergy_;
		}
	}
	int first(PMapCategory c=PMapCategory::All) const
	{
		if (c==PMapCategory::Energy)
			return first_energy_;
		else
			return first_;
	}


	void add(particle *parts, int i, PMapCategory_single c)
	{
		if (c==PMapCategory::Energy)
		{
			if (count(PMapCategory::Energy))
			{
				// If there are some energy particles already, insert at head of energy particle list
				int prevHead = first_energy_;
				if (count(PMapCategory::NotEnergy))
				{
					// If there are some non-energy particles, link to end of that list
					parts[i].pmap_prev = parts[prevHead].pmap_prev;
					parts[parts[prevHead].pmap_prev].pmap_next = i;
				}
				else
				{
					parts[i].pmap_prev = -1;
				}
				parts[i].pmap_next = prevHead;
				parts[prevHead].pmap_prev = i;
			}
			else if (count(PMapCategory::NotEnergy))
			{
				// If there are no energy particles, then first_energy is the last non-energy particle. Insert this particle after it.
				int i_prev = first_energy_;
				parts[i_prev].pmap_next = i;
				parts[i].pmap_prev = i_prev;
				parts[i].pmap_next = -1;
			}
			else
			{
				// No particles in list yet
				parts[i].pmap_next = -1;
				parts[i].pmap_prev = -1;
			}
			first_energy_ = i;
			if (!count(PMapCategory::NotEnergy))
				first_ = i;
		}
		else
		{
			if (count(PMapCategory::All))
			{
				parts[first_].pmap_prev = i;
				parts[i].pmap_next = first_;
			}
			else
			{
				parts[i].pmap_next = -1;
				// If this is the only particle, it is the last non-energy particle too (which is the ID stored in first_energy when there are no energy particles)
				first_energy_ = i;
			}
			parts[i].pmap_prev = -1;
			first_ = i;
		}

		incCount(c);
	}
	void remove(particle *parts, int i, PMapCategory_single c)
	{
		if (parts[i].pmap_prev>=0)
			parts[parts[i].pmap_prev].pmap_next = parts[i].pmap_next;
		if (parts[i].pmap_next>=0)
			parts[parts[i].pmap_next].pmap_prev = parts[i].pmap_prev;

		if (first_==i)
			first_ = parts[i].pmap_next;

		if (count(PMapCategory::Energy)<=1)
		{
			if (first_energy_==i)
			{
				// energyCount==1 and is first_energy: this is the only energy particle left
				// energyCount==0 and is first_energy: this particle is a non-energy particle and is at the end of the list
				// In both cases, set first_energy to pmap_prev so that first_energy is the ID of the last non-energy particle
				first_energy_ = parts[i].pmap_prev;
			}
		}
		else if (first_energy_==i)
		{
			// this is the first energy particle in the list and is being removed, so update first_energy to point at next item
			first_energy_ = parts[i].pmap_next;
		}

		decCount(c);
	}

	int find_one(particle const *parts, int t, PMapCategory c) const
	{
		int n = count(c);
		int i = first(c);
		for (; n>0; i=parts[i].pmap_next, n--)
		{
			if (parts[i].type==t)
				return i;
		}
		return -1;
	}
	int findDifferentOne(particle const *parts, int t, PMapCategory c=PMapCategory::All) const
	{
		int n = count(c);
		int i = first(c);
		for (; n>0; i=parts[i].pmap_next, n--)
		{
			if (parts[i].type!=t)
				return i;
		}
		return -1;
	}

	void clear()
	{
		count_ = count_notEnergy_ = 0;
	}
};

class ParticleMap
{
protected:
	ParticleMapEntry pmap[YRES][XRES];
public:
	ParticleMapEntry& operator() (SimPosI pos)
	{
		return pmap[pos.y][pos.x];
	}
	ParticleMapEntry const& operator() (SimPosI pos) const
	{
		return pmap[pos.y][pos.x];
	}
	ParticleMapEntry* operator[] (int y)
	{
		return pmap[y];
	}
	ParticleMapEntry const* operator[] (int y) const
	{
		return pmap[y];
	}

	void clear();

	ParticleMap()
	{
		clear();
	}
};

#endif

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

#ifndef simulation_BasicData_h
#define simulation_BasicData_h

#include "simulation/Config.hpp"
#include "simulation/Element.h"
#include "simulation/ElemDataShared.h"
#include "simulation/ElemDataSim.h"
#include "simulation/Particle.h"
#include "simulation/ParticleMap.hpp"
#include "common/tptmath.h"
#include "common/tpt-stdint.h"
#include <array>
#include <memory>

class SimulationSharedData;

class Sim_BasicData
{
protected:
	ElemDataSim *elemDataSim_[PT_NUM];
	ElemDataShared **elemDataShared_;
public:
	std::shared_ptr<SimulationSharedData> simSD;

	Element *elements;
	std::array<int,PT_NUM> elementCount;
	int parts_lastActiveIndex;
	int parts_count;
	int pfree;
	ParticleMap pmap;
	particle parts[NPART];
#ifdef DEBUG_PARTSALLOC
	bool partsFree[NPART];
#endif

	Sim_BasicData(std::shared_ptr<SimulationSharedData> sd);
	virtual ~Sim_BasicData();
	void clear();
	void recalc_freeParticles();
	void recalc_pmap();
	void recalc_elementCount();

	template <class ElemDataClass_T>
	ElemDataClass_T * elemData(int elementId)
	{
		return static_cast<ElemDataClass_T*>(elemDataSim_[elementId]);
	}
	ElemDataSim * elemData(int elementId)
	{
		return elemDataSim_[elementId];
	}
	/*template<class ElemDataClass_T, typename... Args>
	void elemData_create(int elementId, Args&&... args)
	{
		delete elemDataSim_[elementId];
		elemDataSim_[elementId] = new ElemDataClass_T(this, elementId, args...);
	}*/

	template <class ElemDataClass_T>
	ElemDataClass_T * elemDataShared(int elementId)
	{
		return static_cast<ElemDataClass_T*>(elemDataShared_[elementId]);
	}
	ElemDataShared * elemDataShared(int elementId)
	{
		return elemDataShared_[elementId];
	}

	bool element_isValid(int t) const // Returns true if the element ID is valid (although it may be PT_NONE).
	{
		return (t>=0 && t<PT_NUM && elements[t].Enabled);
	}
	bool element_isValidThing(int t) const // Returns true if the element ID is valid and is not PT_NONE. TODO: there's got to be a better name for this...
	{
		return (t>0 && t<PT_NUM && elements[t].Enabled);
	}

	bool pos_isValid(SimPosI p) const
	{
		return (p.x>=0 && p.y>=0 && p.x<XRES && p.y<YRES);
	}
	bool pos_isValid(SimPosCell p) const
	{
		return (p.x>=0 && p.y>=0 && p.x<XRES/CELL && p.y<YRES/CELL);
	}
	SimPosF pos_clampValid(SimPosF p) const
	{
		return SimPosF(tptmath::clamp_flt(p.x, 0, XRES-1), tptmath::clamp_flt(p.y, 0, YRES-1));
	}
	SimPosI pos_clampValid(SimPosI p) const
	{
		return SimPosI(tptmath::clamp_int(p.x, 0, XRES-1), tptmath::clamp_int(p.y, 0, YRES-1));
	}
	SimPosCell pos_clampValid(SimPosCell p) const
	{
		return SimPosCell(tptmath::clamp_int(p.x, 0, XRES/CELL-1), tptmath::clamp_int(p.y, 0, YRES/CELL-1));
	}

	bool pos_inMainArea(SimPosI p) const
	{
		return (p.x>=CELL && p.y>=CELL && p.x<XRES-CELL && p.y<YRES-CELL);
	}
	bool pos_inMainArea(SimPosCell p) const
	{
		return (p.x>=1 && p.y>=1 && p.x<XRES/CELL-1 && p.y<YRES/CELL-1);
	}
	SimPosF pos_clampMainArea(SimPosF p) const
	{
		return SimPosF(tptmath::clamp_flt(p.x, CELL, XRES-CELL-1), tptmath::clamp_flt(p.y, CELL, YRES-CELL-1));
	}
	SimPosI pos_clampMainArea(SimPosI p) const
	{
		return SimPosI(tptmath::clamp_int(p.x, CELL, XRES-CELL-1), tptmath::clamp_int(p.y, CELL, YRES-CELL-1));
	}
	SimPosCell pos_clampMainArea(SimPosCell p) const
	{
		return SimPosCell(tptmath::clamp_int(p.x, 1, XRES/CELL-2), tptmath::clamp_int(p.y, 1, YRES/CELL-2));
	}
	SimPosI pos_wrapMainArea_simple(SimPosI p) const
	{
		// Assume not far out of bounds, so wrapping can be done with single add/subtracts
		const int mainWidth = XRES-CELL*2,  mainHeight = YRES-CELL*2;
		if (p.x < CELL)
			p.x += mainWidth;
		else if (p.x >= XRES-CELL)
			p.x -= mainWidth;
		if (p.y < CELL)
			p.y += mainHeight;
		else if (p.y >= YRES-CELL)
			p.y -= mainHeight;
		return p;
	}
	SimPosI pos_wrapMainArea(SimPosI p) const
	{
		bool x_ok = (p.x>=CELL && p.x<XRES-CELL);
		bool y_ok = (p.y>=CELL && p.y<YRES-CELL);
		if (x_ok && y_ok)
			return p;
		if (!x_ok)
			p.x = tptmath::remainder_p(p.x-CELL, XRES-CELL*2)+CELL;
		if (!y_ok)
			p.y = tptmath::remainder_p(p.y-CELL, YRES-CELL*2)+CELL;
		return p;
	}
	SimPosF pos_wrapMainArea(SimPosF pf) const
	{
		SimPosI p(pf);
		bool x_ok = (p.x>=CELL && p.x<XRES-CELL);
		bool y_ok = (p.y>=CELL && p.y<YRES-CELL);
		if (x_ok && y_ok)
			return pf;
		// Main area is [3.5,379.5) (numbers which round to [CELL,XRES-CELL))
		// remainder_p wraps the number to the range 0 to [0,376)
		// so to convert [3.5,379.5) to that range, subtract 3.5
		if (!x_ok)
			pf.x = tptmath::remainder_p(pf.x-CELL+0.5f, float(XRES-CELL*2))+CELL-0.5f;
		if (!y_ok)
			pf.y = tptmath::remainder_p(pf.y-CELL+0.5f, float(YRES-CELL*2))+CELL-0.5f;
		return pf;
	}

	SimPosI pos_centre() const
	{
		return SimPosI(XRES/2, YRES/2);
	}

	PMapCategory_single pmap_category(int t) const
	{
		return (elements[t].Properties&TYPE_ENERGY) ? PMapCategory::Energy : PMapCategory::Plain;
	}
	void pmap_add(int i, SimPosI pos, int t)
	{
		// NB: all arguments are assumed to be within bounds
		pmap(pos).add(parts, i, pmap_category(t));
	}
	void pmap_remove(int i, SimPosI pos, int t)
	{
		// NB: all arguments are assumed to be within bounds
		pmap(pos).remove(parts, i, pmap_category(t));
	}

	// Finds a particle of type t at position pos, returns the ID. Returns a negative number if no particle is found.
	int pmap_find_one(SimPosI pos, int t) const
	{
		return pmap(pos).find_one(parts, t, pmap_category(t));
	}
	// Finds a particle with type!=t at position pos, returns the ID. Returns a negative number if not found.
	int pmap_findDifferentOne(SimPosI pos, int t, PMapCategory c=PMapCategory::All) const
	{
		return pmap(pos).findDifferentOne(parts, t, c);
	}
	bool pmap_elemExists(SimPosI pos, int t) const
	{
		return (pmap_find_one(pos, t)>=0);
	}
	bool pmap_differentElemExists(SimPosI pos, int t, PMapCategory c=PMapCategory::All) const
	{
		return (pmap_findDifferentOne(pos, t, c)>=0);
	}
	// Returns true if a particle of type t exists halfway between i1 and i2
	bool check_middle_particle_type(int i1, int i2, int t) const
	{
		SimPosI pos = SimPos::midpoint(SimPosF(parts[i1].x, parts[i1].y), SimPosF(parts[i1].x, parts[i1].y));
		if (pmap_find_one(pos, t)>=0)
			return true;
		else
			return false;
	}

	// Copy particle properties, except for position and pmap list links
	static void part_copy_properties(const particle& src, particle& dest)
	{
		float tmp_x = dest.x, tmp_y = dest.y;
		int tmp_pmap_prev = dest.pmap_prev, tmp_pmap_next = dest.pmap_next;

		dest = src;

		dest.x = tmp_x;
		dest.y = tmp_y;
		dest.pmap_prev = tmp_pmap_prev;
		dest.pmap_next = tmp_pmap_next;
	}


	// Most of the time, part_alloc and part_free should not be used directly.
	// Use part_create and part_kill instead.
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		if (i>parts_lastActiveIndex)
			parts_lastActiveIndex = i;
		parts_count++;
#ifdef DEBUG_PARTSALLOC
		if (!partsFree[i])
			printf("Particle allocated that isn't free: %d\n", i);
		partsFree[i] = false;
#endif
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
		parts_count--;
#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle freed twice: %d\n", i);
		partsFree[i] = true;
#endif
	}

	// Move particle #i to nxf,nyf without any checks or reacting with particles it collides with
	bool part_set_pos(int i, SimPosI currPos, SimPosF newPosF)
	{
#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle moved after free: %d\n", i);
		if (SimPosI(SimPosF(parts[i].x, parts[i].y)) != currPos)
			printf("Provided original coords wrong for part_set_pos (particle %d): alleged %d,%d actual %d,%d\n", i, currPos.x, currPos.y, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f));
#endif

		SimPosI newPosI = newPosF;
		if (!pos_isValid(newPosI))
			return false;

		parts[i].x = newPosF.x, parts[i].y = newPosF.y;
		if (newPosI!=currPos)
		{
			pmap_remove(i, currPos, parts[i].type);
			pmap_add(i, newPosI, parts[i].type);
		}
		return true;
	}
	bool part_set_pos(int i, SimPosF newPosF)
	{
		return part_set_pos(i, SimPosF(parts[i].x, parts[i].y), newPosF);
	}


	
};

#endif

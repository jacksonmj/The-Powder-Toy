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

#ifndef Simulation_ElementsShared_pyro_h
#define Simulation_ElementsShared_pyro_h

#include "simulation/ElementsCommon.h"
#include "simulation/elements-shared/noHeatSim.h"

class ElementsShared_pyro
{
public:
	static int update(UPDATE_FUNC_ARGS);

	static int update_neighbour(UPDATE_FUNC_ARGS, int rx, int ry, int rt, int ri)
	{
		if (sim->walls.isProperWall(SimCoordI(x+rx,y+ry)))
			return 0;
		int t = parts[i].type;

		//THRM burning
		if (rt==PT_THRM && (t==PT_FIRE || t==PT_PLSM || t==PT_LAVA))
		{
			if (sim->rng.chance<1,500>()) {
				sim->part_change_type(ri,x+rx,y+ry,PT_LAVA);
				parts[ri].ctype = PT_BMTL;
				parts[ri].temp = 3500.0f;
				sim->air.pv.add(SimCoordI(x+rx,y+ry), 50.0f);
			} else {
				sim->part_change_type(ri,x+rx,y+ry,PT_LAVA);
				parts[ri].life = 400;
				parts[ri].ctype = PT_THRM;
				parts[ri].temp = 3500.0f;
				parts[ri].tmp = 20;
			}
			return 0;
		}

		if ((rt==PT_COAL) || (rt==PT_BCOL))
		{
			if ((t==PT_FIRE || t==PT_PLSM))
			{
				if (parts[ri].life>100 && sim->rng.chance<1,500>()) {
					parts[ri].life = 99;
				}
			}
			else if (t==PT_LAVA)
			{
				if (parts[i].ctype == PT_IRON && sim->rng.chance<1,500>()) {
					parts[i].ctype = PT_METL;
					sim->part_kill(ri);
					return 2;
				}
			}
		}

		if ((surround_space || sim->elements[rt].Explosive) &&
			sim->elements[rt].Flammable && sim->rng.chance(sim->elements[rt].Flammable + (int)(sim->air.pv.get(SimCoordI(x+rx,y+ry)) * 10.0f), 1000) &&
			//exceptions, t is the thing causing the spark and rt is what's burning
			(t!=PT_SPRK || (rt!=PT_RBDM && rt!=PT_LRBD && rt!=PT_INSL)) &&
			(t!=PT_PHOT || rt!=PT_INSL) &&
			(rt!=PT_SPNG || parts[ri].life==0) &&
			(rt!=PT_H2 || parts[ri].temp < 2273.15))
		{
			sim->part_change_type(ri,x+rx,y+ry,PT_FIRE);
			// TODO: add to existing temp instead of setting temp? Might break compatibility.
			sim->part_set_temp(parts[ri], sim->elements[PT_FIRE].DefaultProperties.temp + (sim->elements[rt].Flammable/2));
			parts[ri].life = sim->rng.randInt<180,180+79>();
			parts[ri].tmp = parts[ri].ctype = 0;
			if (sim->elements[rt].Explosive)
				sim->air.pv.add(SimCoordI(x,y), 0.25f * CFDS);
		}

		if (legacy_enable && t!=PT_SPRK)
		{
			return ElementsShared_noHeatSim::update_pyro_neighbour(UPDATE_FUNC_SUBCALL_ARGS, rx, ry, rt, ri);
		}

		return 0;
	}
};

#endif

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

#ifndef Simulation_ElementsShared_noHeatSim_h
#define Simulation_ElementsShared_noHeatSim_h

#include "simulation/ElementsCommon.h"

class ElementsShared_noHeatSim
{
public:
	static int update(UPDATE_FUNC_ARGS);

	static int update_pyro_neighbour(UPDATE_FUNC_ARGS, int rx, int ry, int rt, int ri)
	{
		// Additional interactions for 'hot' things (FIRE, LAVA, etc) which only occur when heat sim is off
		if (bmap[(y+ry)/CELL][(x+rx)/CELL] && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_STREAM)
			return 0;
		int t = parts[i].type;
		float lpv = (int)sim->air.pv.get(SimCoordI(x+rx,y+ry));
		if (lpv < 1) lpv = 1;
		if (sim->elements[rt].Meltable  && ((rt!=PT_RBDM && rt!=PT_LRBD) || t!=PT_SPRK) && ((t!=PT_FIRE&&t!=PT_PLSM) || (rt!=PT_METL && rt!=PT_IRON && rt!=PT_ETRD && rt!=PT_PSCN && rt!=PT_NSCN && rt!=PT_NTCT && rt!=PT_PTCT && rt!=PT_BMTL && rt!=PT_BRMT && rt!=PT_SALT && rt!=PT_INWR)) &&
				sim->rng.chance(sim->elements[rt].Meltable*lpv,1000))
		{
			if (t!=PT_LAVA || parts[i].life>0)
			{
				if (rt==PT_BRMT)
					parts[ri].ctype = PT_BMTL;
				else if (rt==PT_SAND)
					parts[ri].ctype = PT_GLAS;
				else
					parts[ri].ctype = rt;
				sim->part_change_type(ri,x+rx,y+ry,PT_LAVA);
				parts[ri].life = sim->rng.randInt<240,240+119>();
			}
			else
			{
				parts[i].life = 0;
				parts[i].ctype = PT_NONE;
				sim->part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
				return 1;
			}
		}
		if (rt==PT_ICEI || rt==PT_SNOW)
		{
			sim->part_change_type(ri,x+rx,y+ry,PT_WATR);
			if (t==PT_FIRE)
			{
				sim->part_kill(i);
				return 1;
			}
			if (t==PT_LAVA)
			{
				parts[i].life = 0;
				sim->part_change_type(i,x,y,PT_STNE);
			}
		}
		if (rt==PT_WATR || rt==PT_DSTW || rt==PT_SLTW)
		{
			sim->part_kill(ri);
			if (t==PT_FIRE)
			{
				sim->part_kill(i);
				return 1;
			}
			if (t==PT_LAVA)
			{
				parts[i].life = 0;
				parts[i].ctype = PT_NONE;
				sim->part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
			}
		}
		return 0;
	}
	static int update_pyro(UPDATE_FUNC_ARGS);
};

#endif

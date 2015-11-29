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

#include "simulation/ElementsCommon.h"
#include "simulation/elements-shared/noHeatSim.h"

// Additional interactions which only occur when heatMode is Legacy
int ElementsShared_noHeatSim::update(UPDATE_FUNC_ARGS) {
	int rx, ry, rt;
	int rcount, ri, rnext;
	int t = parts[i].type;
	if (sim->option_heatMode() != HeatMode::Legacy)
		return 0;
	if (t==PT_WTRV) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 &&
						x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_WATR||rt==PT_DSTW||rt==PT_SLTW) && sim->rng.chance<1,1000>())
						{
							sim->part_change_type(i,x,y,PT_WATR);
							sim->part_change_type(ri,x+rx,y+ry,PT_WATR);
						}
						if ((rt==PT_ICEI || rt==PT_SNOW) && sim->rng.chance<1,1000>())
						{
							sim->part_change_type(i,x,y,PT_WATR);
							if (sim->rng.chance<1,1000>())
								sim->part_change_type(ri,x+rx,y+ry,PT_WATR);
						}
					}
				}
	}
	else if (t==PT_WATR) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 &&
						x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_FIRE || rt==PT_LAVA) && sim->rng.chance<1,10>())
						{
							sim->part_change_type(i,x,y,PT_WTRV);
						}
					}
				}
	}
	else if (t==PT_SLTW) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 &&
						x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_FIRE || rt==PT_LAVA) && sim->rng.chance<1,10>())
						{
							if (sim->rng.chance<1,4>()) sim->part_change_type(i,x,y,PT_SALT);
							else sim->part_change_type(i,x,y,PT_WTRV);
						}
					}
				}
	}
	else if (t==PT_DSTW) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 &&
						x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_FIRE || rt==PT_LAVA) && sim->rng.chance<1,10>())
						{
							sim->part_change_type(i,x,y,PT_WTRV);
						}
					}
				}
	}
	else if (t==PT_ICEI) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_WATR || rt==PT_DSTW) && sim->rng.chance<1,1000>())
						{
							sim->part_change_type(i,x,y,PT_ICEI);
							sim->part_change_type(ri,x+rx,y+ry,PT_ICEI);
						}
					}
				}
	}
	else if (t==PT_SNOW) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if ((rt==PT_WATR || rt==PT_DSTW) && sim->rng.chance<1,1000>())
						{
							sim->part_change_type(i,x,y,PT_ICEI);
							sim->part_change_type(ri,x+rx,y+ry,PT_ICEI);
						}
						if ((rt==PT_WATR || rt==PT_DSTW) && sim->rng.chance<3,200>())
							sim->part_change_type(i,x,y,PT_WATR);
					}
				}
	}
	if (t==PT_WTRV && sim->air.pv.get(SimPosI(x,y))>4.0f)
		sim->part_change_type(i,x,y,PT_DSTW);
	if (t==PT_OIL && sim->air.pv.get(SimPosI(x,y))<-6.0f)
		sim->part_change_type(i,x,y,PT_GAS);
	if (t==PT_GAS && sim->air.pv.get(SimPosI(x,y))>6.0f)
		sim->part_change_type(i,x,y,PT_OIL);
	if (t==PT_DESL && sim->air.pv.get(SimPosI(x,y))>12.0f)
	{
		sim->part_change_type(i,x,y,PT_FIRE);
		parts[i].life = sim->rng.randInt<120,120+49>();
	}
	return 0;
}

// Additional interactions for 'hot' things (FIRE, LAVA, etc) which only occur when heatMode is Legacy
int ElementsShared_noHeatSim::update_pyro(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	if (parts[i].type==PT_SPRK)
		return 0;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					int ret = update_pyro_neighbour(UPDATE_FUNC_SUBCALL_ARGS, rx, ry, rt, ri);
					if (ret==1)
						return 1;
				}
			}
	return 0;
}

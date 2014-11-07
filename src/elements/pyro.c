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

int update_PYRO(UPDATE_FUNC_ARGS) {
	int rx, ry, rt, t = parts[i].type;
	int rcount, ri, rnext;
	switch (t)
	{
	case PT_PLSM:
		if (parts[i].life <= 1)
		{
			if (parts[i].ctype == PT_NBLE)
			{
				part_change_type(i,x,y,PT_NBLE);
				parts[i].life = 0;
			}
			else if ((parts[i].tmp&3)==3)
			{
				part_change_type(i,x,y,PT_DSTW);
				parts[i].life = 0;
				parts[i].ctype = PT_FIRE;
			}
		}
		break;
	case PT_FIRE:
		if(parts[i].life <= 1)
		{
			if ((parts[i].tmp&3)==3)
			{
				part_change_type(i,x,y,PT_DSTW);
				parts[i].life = 0;
				parts[i].ctype = PT_FIRE;
			}
			else if (parts[i].temp<625)
			{
				part_change_type(i,x,y,PT_SMKE);
				parts[i].life = rand()%20+250;
			}
		}
		break;
	default:
		break;
	}
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (bmap[(y+ry)/CELL][(x+rx)/CELL] && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_STREAM)
						continue;
					rt = parts[ri].type;
					if ((surround_space || ptypes[rt].explosive) &&
						(t!=PT_SPRK || (rt!=PT_RBDM && rt!=PT_LRBD && rt!=PT_INSL)) &&
						(t!=PT_PHOT || rt!=PT_INSL) &&
						(rt!=PT_SPNG || parts[ri].life==0) &&
						(rt!=PT_H2 || parts[ri].temp < 2273.15) &&
						ptypes[rt].flammable && (ptypes[rt].flammable + (int)(pv[(y+ry)/CELL][(x+rx)/CELL]*10.0f))>(rand()%1000))
					{
						part_change_type(ri,x+rx,y+ry,PT_FIRE);
						parts[ri].temp = restrict_flt(ptypes[PT_FIRE].heat + (ptypes[rt].flammable/2), MIN_TEMP, MAX_TEMP);
						parts[ri].life = rand()%80+180;
						parts[ri].tmp = parts[ri].ctype = 0;
						if (ptypes[rt].explosive)
							pv[y/CELL][x/CELL] += 0.25f * CFDS;
					}
				}
			}
	if (legacy_enable && t!=PT_SPRK) // SPRK has no legacy reactions
		update_legacy_PYRO(UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

int update_legacy_PYRO(UPDATE_FUNC_ARGS) {
	int rx, ry, rt, lpv, t = parts[i].type;
	int rcount, ri, rnext;
	if (t==PT_SPRK)
		return 0;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (bmap[(y+ry)/CELL][(x+rx)/CELL] && bmap[(y+ry)/CELL][(x+rx)/CELL]!=WL_STREAM)
						continue;
					rt = parts[ri].type;
					lpv = (int)pv[(y+ry)/CELL][(x+rx)/CELL];
					if (lpv < 1) lpv = 1;
					if (ptypes[rt].meltable  && ((rt!=PT_RBDM && rt!=PT_LRBD) || t!=PT_SPRK) && ((t!=PT_FIRE&&t!=PT_PLSM) || (rt!=PT_METL && rt!=PT_IRON && rt!=PT_ETRD && rt!=PT_PSCN && rt!=PT_NSCN && rt!=PT_NTCT && rt!=PT_PTCT && rt!=PT_BMTL && rt!=PT_BRMT && rt!=PT_SALT && rt!=PT_INWR)) &&
							ptypes[rt].meltable*lpv>(rand()%1000))
					{
						if (t!=PT_LAVA || parts[i].life>0)
						{
							if (rt==PT_BRMT)
								parts[ri].ctype = PT_BMTL;
							else if (rt==PT_SAND)
								parts[ri].ctype = PT_GLAS;
							else
								parts[ri].ctype = rt;
							part_change_type(ri,x+rx,y+ry,PT_LAVA);
							parts[ri].life = rand()%120+240;
						}
						else
						{
							parts[i].life = 0;
							parts[i].ctype = PT_NONE;
							part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
							return 1;
						}
					}
					if (rt==PT_ICEI || rt==PT_SNOW)
					{
						part_change_type(ri,x+rx,y+ry,PT_WATR);
						if (t==PT_FIRE)
						{
							kill_part(i);
							return 1;
						}
						if (t==PT_LAVA)
						{
							parts[i].life = 0;
							part_change_type(i,x,y,PT_STNE);
						}
					}
					if (rt==PT_WATR || rt==PT_DSTW || rt==PT_SLTW)
					{
						kill_part(ri);
						if (t==PT_FIRE)
						{
							kill_part(i);
							return 1;
						}
						if (t==PT_LAVA)
						{
							parts[i].life = 0;
							parts[i].ctype = PT_NONE;
							part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
						}
					}
				}
			}
	return 0;
}

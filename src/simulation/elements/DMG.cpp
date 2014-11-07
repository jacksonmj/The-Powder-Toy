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

int DMG_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry, nb, nxi, nxj, t, dist;
	int rcount, ri, rnext;
	int rrcount, rri, rrnext;
	int rad = 25;
	float angle, fx, fy;
	
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt!=PT_DMG && rt!=PT_EMBR && rt!=PT_DMND && rt!=PT_CLNE && rt!=PT_PCLN && rt!=PT_BCLN)
					{
						kill_part(i);
						for (nxj=-rad; nxj<=rad; nxj++)
							for (nxi=-rad; nxi<=rad; nxi++)
								if (x+nxi>=0 && y+nxj>=0 && x+nxi<XRES && y+nxj<YRES && (nxi || nxj))
								{
									dist = sqrtf(powf(nxi, 2)+powf(nxj, 2));//;(pow((float)nxi,2))/(pow((float)rad,2))+(pow((float)nxj,2))/(pow((float)rad,2));
									if (!dist || (dist <= rad))
									{
										FOR_PMAP_POSITION_NOENERGY(sim, x+nxi, y+nxj, rrcount, rri, rrnext)
										{
											angle = atan2f(nxj, nxi);
											fx = cosf(angle) * 7.0f;
											fy = sinf(angle) * 7.0f;

											parts[rri].vx += fx;
											parts[rri].vy += fy;
											
											vx[(y+nxj)/CELL][(x+nxi)/CELL] += fx;
											vy[(y+nxj)/CELL][(x+nxi)/CELL] += fy;

											pv[(y+nxj)/CELL][(x+nxi)/CELL] += 1.0f;
											
											t = parts[rri].type;
											if(t && ptransitions[t].pht>-1 && ptransitions[t].pht<PT_NUM)
												part_change_type(rri, x+nxi, y+nxj, ptransitions[t].pht);
											else if(t == PT_BMTL)
												part_change_type(rri, x+nxi, y+nxj, PT_BRMT);
											else if(t == PT_GLAS)
												part_change_type(rri, x+nxi, y+nxj, PT_BGLA);
											else if(t == PT_COAL)
												part_change_type(rri, x+nxi, y+nxj, PT_BCOL);
											else if(t == PT_QRTZ)
												part_change_type(rri, x+nxi, y+nxj, PT_PQRT);
											else if(t == PT_TUNG)
											{
												part_change_type(rri, x+nxi, y+nxj, PT_BRMT);
												parts[rri].ctype = PT_TUNG;
											}
										}
									}
								}
						return 1;
					}
				}
			}
	return 0;
}

int DMG_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_FLARE;
	return 1;
}

void DMG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_DMG";
	elem->Name = "DMG";
	elem->Colour = COLPACK(0x88FF88);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP-2.0f  +273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Generates damaging pressure and breaks elements it hits.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DMG_update;
	elem->Graphics = &DMG_graphics;
}


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

int PBCN_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	if (parts[i].life>0 && parts[i].life!=10)
		parts[i].life--;
	if (!parts[i].tmp2 && pv[y/CELL][x/CELL]>4.0f)
		parts[i].tmp2 = rand()%40+80;
	if (parts[i].tmp2)
	{
		float advection = 0.1f;
		parts[i].vx += advection*vx[y/CELL][x/CELL];
		parts[i].vy += advection*vy[y/CELL][x/CELL];
		parts[i].tmp2--;
		if(!parts[i].tmp2){
			kill_part(i);
			return 1;
		}
	}
	if (parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !ptypes[parts[i].ctype].enabled || (parts[i].ctype==PT_LIFE && (parts[i].tmp<0 || parts[i].tmp>=NGOLALT)))
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					r = photons[y+ry][x+rx];
					if (!r)
						r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)!=PT_CLNE && (r&0xFF)!=PT_PCLN &&
				        (r&0xFF)!=PT_BCLN &&  (r&0xFF)!=PT_SPRK &&
				        (r&0xFF)!=PT_NSCN && (r&0xFF)!=PT_PSCN &&
				        (r&0xFF)!=PT_STKM && (r&0xFF)!=PT_STKM2 &&
				        (r&0xFF)!=PT_PBCN && (r&0xFF)<PT_NUM)
					{
						parts[i].ctype = r&0xFF;
						if ((r&0xFF)==PT_LIFE || (r&0xFF)==PT_LAVA)
							parts[i].tmp = parts[r>>8].ctype;
					}
				}
	if (parts[i].life==10)
	{
		
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)==PT_PBCN)
					{
						if (parts[r>>8].life<10&&parts[r>>8].life>0)
							parts[i].life = 9;
						else if (parts[r>>8].life==0)
							parts[r>>8].life = 10;
					}
				}
	}
	if (parts[i].ctype>0 && parts[i].ctype<PT_NUM && ptypes[parts[i].ctype].enabled && parts[i].life==10) {
		if (parts[i].ctype==PT_PHOT) {//create photons a different way
			for (rx=-1; rx<2; rx++) {
				for (ry=-1; ry<2; ry++)
				{
					if (rx || ry)
					{
						int r = create_part(-1, x+rx, y+ry, parts[i].ctype);
						if (r!=-1)
						{
							parts[r].vx = rx*3;
							parts[r].vy = ry*3;
							if (r>i)
							{
								// Make sure movement doesn't happen until next frame, to avoid gaps in the beams of photons produced
								parts[r].flags |= FLAG_SKIPMOVE;
							}
						}
					}
				}
			}
		}
		else if (parts[i].ctype==PT_LIFE) {//create life a different way
			for (rx=-1; rx<2; rx++) {
				for (ry=-1; ry<2; ry++) {
					create_part(-1, x+rx, y+ry, parts[i].ctype|(parts[i].tmp<<8));
				}
			}
		}
		else if (parts[i].ctype!=PT_LIGH || (rand()%30)==0)
		{
			int np = create_part(-1, x+rand()%3-1, y+rand()%3-1, parts[i].ctype);
			if (np>=0)
			{
				if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && ptransitions[parts[i].tmp].tht==PT_LAVA)
					parts[np].ctype = parts[i].tmp;
			}
		}
	}
	return 0;
}

int PBCN_graphics(GRAPHICS_FUNC_ARGS)
{
	int lifemod = ((cpart->life>10?10:cpart->life)*10);
	*colr += lifemod;
	*colg += lifemod/2;
	return 0;
}

void PBCN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PBCN";
	elem->Name = "PBCN";
	elem->Colour = COLPACK(0x3B1D0A);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWERED;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.50f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 12;

	elem->Weight = 100;

	elem->CreationTemperature = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Powered breakable clone";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &PBCN_update;
	elem->Graphics = &PBCN_graphics;
}


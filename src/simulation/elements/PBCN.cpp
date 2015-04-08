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
	int rx, ry, rt;
	int rcount, ri, rnext;
	if (!parts[i].tmp2 && sim->air.pv.get(SimCoordI(x,y))>4.0f)
		parts[i].tmp2 = sim->rng.randInt<80,80+39>();
	if (parts[i].tmp2)
	{
		float advection = 0.1f;
		parts[i].vx += advection*sim->air.vx.get(SimCoordI(x,y));
		parts[i].vy += advection*sim->air.vy.get(SimCoordI(x,y));
		parts[i].tmp2--;
		if(!parts[i].tmp2){
			sim->part_kill(i);
			return 1;
		}
	}
	if (parts[i].ctype<=0 || !sim->IsValidElement(parts[i].ctype) || (parts[i].ctype==PT_LIFE && (parts[i].tmp<0 || parts[i].tmp>=NGOLALT)))
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						rt = parts[ri].type;
						if (rt!=PT_CLNE && rt!=PT_PCLN &&
							rt!=PT_BCLN && rt!=PT_SPRK &&
							rt!=PT_NSCN && rt!=PT_PSCN &&
							rt!=PT_STKM && rt!=PT_STKM2 &&
							rt!=PT_PBCN)
						{
							parts[i].ctype = rt;
							if (rt==PT_LIFE || rt==PT_LAVA)
								parts[i].tmp = parts[ri].ctype;
						}
					}
				}
	if (parts[i].life!=10)
	{
		if (parts[i].life>0)
			parts[i].life--;
	}
	else
	{
		
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type==PT_PBCN)
						{
							if (parts[ri].life<10&&parts[ri].life>0)
								parts[i].life = 9;
							else if (parts[ri].life==0)
								parts[ri].life = 10;
						}
					}
				}
		if (parts[i].ctype>0 && sim->IsValidElement(parts[i].ctype)) {
			if (parts[i].ctype==PT_PHOT) {//create photons a different way
				for (rx=-1; rx<2; rx++) {
					for (ry=-1; ry<2; ry++)
					{
						if (rx || ry)
						{
							int r = sim->part_create(-1, x+rx, y+ry, parts[i].ctype);
							if (r>=0)
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
						// TODO: change this create_part
						create_part(-1, x+rx, y+ry, parts[i].ctype|(parts[i].tmp<<8));
					}
				}
			}
			else if (parts[i].ctype!=PT_LIGH || sim->rng.chance<1,30>())
			{
				int rx,ry;
				sim->randomRelPos_1_noCentre(&rx,&ry);
				int np = sim->part_create(-1, x+rx, y+ry, parts[i].ctype);
				if (np>=0)
				{
					if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && sim->elements[parts[i].tmp].HighTemperatureTransitionElement==PT_LAVA)
						parts[np].ctype = parts[i].tmp;
				}
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
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_PBCN";
	elem->ui->Name = "PBCN";
	elem->Colour = COLPACK(0x3B1D0A);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWERED;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Powered breakable clone.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID | PROP_NOCTYPEDRAW;

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


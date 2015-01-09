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

int QRTZ_update(UPDATE_FUNC_ARGS)
{
	int tmp, trade, rx, ry, np, t;
	int rcount, ri, rnext;
	t = parts[i].type;
	if (t == PT_QRTZ)
	{
		parts[i].pavg[0] = parts[i].pavg[1];
		parts[i].pavg[1] = pv[y/CELL][x/CELL];
		if (parts[i].pavg[1]-parts[i].pavg[0] > 0.05*(parts[i].temp/3) || parts[i].pavg[1]-parts[i].pavg[0] < -0.05*(parts[i].temp/3))
		{
			part_change_type(i,x,y,PT_PQRT);
			parts[i].life = 5; //timer before it can grow or diffuse again
		}
	}
	if (parts[i].life>5)
		parts[i].life = 5;
	// absorb SLTW
	if (parts[i].tmp!=-1)
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type==PT_SLTW && !(rand()%500))
						{
							kill_part(ri);
							parts[i].tmp ++;
						}
					}
				}
	// grow if absorbed SLTW
	if (parts[i].tmp > 0 && (parts[i].vx*parts[i].vx + parts[i].vy*parts[i].vy)<0.2f && parts[i].life<=0)
	{
		bool stopgrow=false;
		int rnd, sry, srx;
		for ( trade = 0; trade<9; trade ++)
		{
			rnd = rand()%0x3FF;
			rx = (rnd%5)-2;
			srx = (rnd%3)-1;
			rnd = rnd>>3;
			ry = (rnd%5)-2;
			sry = (rnd%3)-1;
			if (x+srx>=0 && y+sry>=0 && x+srx<XRES && y+sry<YRES && (srx || sry))
			{
				if (!stopgrow && !sim->pmap[y+sry][x+srx].count_notEnergy && parts[i].tmp!=0)
				{
					np = sim->part_create(-1,x+srx,y+sry,PT_QRTZ);
					if (np>-1)
					{
						parts[np].temp = parts[i].temp;
						parts[np].tmp2 = parts[i].tmp2;
						parts[i].tmp--;
						if (t == PT_PQRT)
						{
							// If PQRT is stationary and has started growing particles of QRTZ, the PQRT is basically part of a new QRTZ crystal. So turn it back into QRTZ so that it behaves more like part of the crystal.
							sim->part_change_type(i,x,y,PT_QRTZ);
						}
						if (rand()%2)
						{
							parts[np].tmp=-1;//dead qrtz
						}
						else if (!parts[i].tmp && 1>rand()%15)
						{
							parts[i].tmp=-1;
						}
						stopgrow=true;
					}
				}
			}
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if ((parts[ri].type==PT_QRTZ || parts[ri].type==PT_PQRT) && (parts[i].tmp>parts[ri].tmp) && parts[ri].tmp>=0 )//diffusion
					{
						tmp = parts[i].tmp - parts[ri].tmp;
						if (tmp ==1)
						{
							parts[ri].tmp ++;
							parts[i].tmp --;
							break;
						}
						if (tmp>0)
						{
							parts[ri].tmp += tmp/2;
							parts[i].tmp -= tmp/2;
							break;
						}
					}
				}
			}
		}
	}
	return 0;
}

int QRTZ_graphics(GRAPHICS_FUNC_ARGS) //QRTZ and PQRT
{
	int t = cpart->type, z = cpart->tmp2 - 5;//speckles!
	if (cpart->temp>(ptransitions[t].thv-800.0f))//hotglow for quartz
	{
		float frequency = 3.1415/(2*ptransitions[t].thv-(ptransitions[t].thv-800.0f));
		int q = (cpart->temp>ptransitions[t].thv)?ptransitions[t].thv-(ptransitions[t].thv-800.0f):cpart->temp-(ptransitions[t].thv-800.0f);
		*colr += sin(frequency*q) * 226 + (z * 16);
		*colg += sin(frequency*q*4.55 +3.14) * 34 + (z * 16);
		*colb += sin(frequency*q*2.22 +3.14) * 64 + (z * 16);
	}
	else
	{
		*colr += z * 16;
		*colg += z * 16;
		*colb += z * 16;
	}
	return 0;
}

void QRTZ_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp2 = (rand()%11);
	sim->parts[i].pavg[1] = pv[y/CELL][x/CELL];
}

void QRTZ_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_QRTZ";
	elem->ui->Name = "QRTZ";
	elem->Colour = COLPACK(0xAADDDD);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 3;
	elem->Latent = 0;
	elem->ui->Description = "Quartz, breakable mineral. Conducts but becomes brittle at lower temperatures.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_HOT_GLOW|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2573.15f;
	elem->HighTemperatureTransitionElement = PT_LAVA;

	elem->Update = &QRTZ_update;
	elem->Graphics = &QRTZ_graphics;
	elem->Func_Create = &QRTZ_create;
}


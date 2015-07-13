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

int MERC_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, trade, np;
	int rcount, ri, rnext;
	int maxtmp = ((10000/(parts[i].temp + 1))-1);
	if (sim->rng.chance(10000%((int)parts[i].temp+1), parts[i].temp+1))
		maxtmp ++;
	if (parts[i].tmp < maxtmp)
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[i].tmp >=maxtmp)
							break;
						if (parts[ri].type==PT_MERC && sim->rng.chance<1,3>())
						{
							if ((parts[i].tmp + parts[ri].tmp + 1) <= maxtmp)
							{
								parts[i].tmp += parts[ri].tmp + 1;
								sim->part_kill(ri);
							}
						}
					}
				}
	}
	else
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					if (parts[i].tmp<=maxtmp)
						continue;
					if (!sim->pmap[y+ry][x+rx].count(PMapCategory::NotEnergy) && parts[i].tmp>=1)//if nothing then create MERC
					{
						np = sim->part_create(-1,x+rx,y+ry,PT_MERC);
						if (np<0) continue;
						parts[i].tmp--;
						parts[np].temp = parts[i].temp;
						parts[np].tmp = 0;
						parts[np].dcolour = parts[i].dcolour;
					}
				}
	for ( trade = 0; trade<4; trade ++)
	{
		sim->randomRelPos_2_noCentre(&rx,&ry);
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
		{
			FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
			{
				if (parts[ri].type==PT_MERC&&(parts[i].tmp>parts[ri].tmp)&&parts[i].tmp>0)//diffusion
				{
					int temp = parts[i].tmp - parts[ri].tmp;
					if (temp ==1)
					{
						parts[ri].tmp ++;
						parts[i].tmp --;
					}
					else if (temp>0)
					{
						parts[ri].tmp += temp/2;
						parts[i].tmp -= temp/2;
					}
				}
			}
		}
	}
	return 0;
}

void MERC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_MERC";
	elem->ui->Name = "MERC";
	elem->Colour = COLPACK(0x736B6D);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.3f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 91;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Mercury. Volume changes with temperature, Conductive.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_NEUTABSORB|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = 10;

	elem->Update = &MERC_update;
	elem->Graphics = NULL;
}


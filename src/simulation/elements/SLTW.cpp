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

int SLTW_update(UPDATE_FUNC_ARGS) {
	int rx, ry;
	int rcount, ri, rnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					switch (parts[ri].type)
					{
					case PT_SALT:
						if (sim->rng.chance<1,2000>())
							sim->part_change_type(ri,x+rx,y+ry,PT_SLTW);
						break;
					case PT_PLNT:
						if (sim->rng.chance<1,40>())
							sim->part_kill(ri);
						break;
					case PT_RBDM:
					case PT_LRBD:
						if ((!sim->heat_mode || parts[i].temp>(273.15f+12.0f)) && sim->rng.chance<1,100>())
						{
							sim->part_change_type(i,x,y,PT_FIRE);
							parts[i].life = 4;
							parts[i].ctype = PT_WATR;
						}
						break;
					case PT_FIRE:
						if (parts[ri].ctype!=PT_WATR){
							sim->part_kill(ri);
							if(sim->rng.chance<1,30>()){
								sim->part_kill(i);
								return 1;
							}
						}
						break;
					default:
						break;
					}
				}
			}
	return 0;
}

void SLTW_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_SLTW";
	elem->ui->Name = "SLTW";
	elem->Colour = COLPACK(0x4050F0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 75;
	elem->Latent = 7500;
	elem->ui->Description = "Saltwater, conducts electricity, difficult to freeze.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = 252.05f;
	elem->LowTemperatureTransitionElement = PT_ICEI;
	elem->HighTemperatureTransitionThreshold = 383.0f;
	elem->HighTemperatureTransitionElement = ST;

	elem->Update = &SLTW_update;
	elem->Graphics = NULL;
}


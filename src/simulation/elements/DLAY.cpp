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

int DLAY_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, oldl;
	int rcount, ri, rnext;
	oldl = parts[i].life;
	if (parts[i].life>0)
		parts[i].life--;

	if (parts[i].temp<= 1.0f+273.15f)
		parts[i].temp = 1.0f+273.15f;

	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				if (sim->pmap[y+ry][x+rx].count_notEnergy==0 || sim->is_spark_blocked(x,y,x+rx,y+ry))
					continue;
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type==PT_SPRK && parts[i].life==0 && parts[ri].life>0 && parts[ri].life<4 && parts[ri].ctype==PT_PSCN)
					{
						parts[i].life = (int)(parts[i].temp-273.15);
					}
					else if (parts[ri].type==PT_DLAY)
					{
						if(!parts[i].life)
						{
							if (parts[ri].life)
							{
								parts[i].life = parts[ri].life;
								if(ri>i) //If the other particle hasn't been life updated
									parts[i].life--;
							}
						}
						else if (!parts[ri].life)
						{
							parts[ri].life = parts[i].life;
							if((ri)>i) //If the other particle hasn't been life updated
								parts[ri].life++;
						}
					}
					else if(parts[ri].type==PT_NSCN && oldl==1)
					{
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
					}
				}
			}
	return 0;
}

int DLAY_graphics(GRAPHICS_FUNC_ARGS)
{
	int stage = (int)(((float)cpart->life/(cpart->temp-273.15))*100.0f);
	*colr += stage;
	*colg += stage;
	*colb += stage;
	return 0;
}

void DLAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_DLAY";
	elem->ui->Name = "DLAY";
	elem->Colour = COLPACK(0x753590);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_POWERED;
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 4.0f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Conducts with temperature-dependent delay. (use HEAT/COOL).";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DLAY_update;
	elem->Graphics = &DLAY_graphics;
}


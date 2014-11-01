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

int in_radius(int rd, int x, int y); //defined in DTEC.cpp

int TSNS_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt, rd = parts[i].tmp2;
	int rcount, ri, rnext;
	if (rd > 25) parts[i].tmp2 = rd = 25;
	if (parts[i].life)
	{
		parts[i].life = 0;
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					if (sim->pmap[y+ry][x+rx].count<=0 || sim->is_spark_blocked(x,y,x+rx,y+ry))
					continue;
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
					{
						rt = parts[ri].type;
						if ((ptypes[rt].properties&PROP_CONDUCTS) && !(rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR) && parts[ri].life==0)
						{
							sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
						}
					}
				}
	}
	for (rx=-rd; rx<rd+1; rx++)
		for (ry=-rd; ry<rd+1; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].temp >= parts[i].temp && parts[ri].type != PT_TSNS && parts[ri].type != PT_METL)
						parts[i].life = 1;
				}
			}
	return 0;
}

void TSNS_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_TSNS";
	elem->Name = "TSNS";
	elem->Colour = COLPACK(0xFD9D18);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SENSOR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.96f;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f  +273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Creates a spark when there's a nearby particle with equal or greater temperature";

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

	elem->DefaultProperties.tmp2 = 1;

	elem->Update = &TSNS_update;
	elem->Graphics = NULL;
}


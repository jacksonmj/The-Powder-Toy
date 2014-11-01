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

int ACID_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, trade, np;
	int rcount, ri, rnext;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
				{
					int rt = parts[ri].type;
					if (rt!=PT_ACID && rt!=PT_CAUS)
					{
						if (rt==PT_PLEX || rt==PT_NITR || rt==PT_GUNP || rt==PT_RBDM || rt==PT_LRBD)
						{
							part_change_type(i,x,y,PT_FIRE);
							part_change_type(ri,x+rx,y+ry,PT_FIRE);
							parts[i].life = 4;
							parts[ri].life = 4;
						}
						else if (rt==PT_WTRV)
						{
							if(!(rand()%250))
							{
								part_change_type(i, x, y, PT_CAUS);
								parts[i].life = (rand()%50)+25;
								kill_part(ri);
							}
						}
						else if ((rt!=PT_CLNE && rt!=PT_PCLN && ptypes[rt].hardness>(rand()%1000))&&parts[i].life>=50)
						{
							if (!sim->check_middle_particle_type(i, ri, PT_GLAS))//GLAS protects stuff from acid
							{
								float newtemp = ((60.0f-(float)ptypes[ri].hardness))*7.0f;
								if(newtemp < 0){
									newtemp = 0;
								}
								parts[i].temp += newtemp;
								parts[i].life--;
								kill_part(ri);
							}
						}
						else if (parts[i].life<=50)
						{
							kill_part(i);
							return 1;
						}
					}
				}
			}
	for ( trade = 0; trade<2; trade ++)
	{
		rx = rand()%5-2;
		ry = rand()%5-2;
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
			{
				if (parts[ri].type==PT_ACID&&(parts[i].life>parts[ri].life)&&parts[i].life>0)//diffusion
				{
					int temp = parts[i].life - parts[ri].life;
					if (temp ==1)
					{
						parts[ri].life ++;
						parts[i].life --;
					}
					else if (temp>0)
					{
						parts[ri].life += temp/2;
						parts[i].life -= temp/2;
					}
				}
			}
		}
	}
	return 0;
}

int ACID_graphics(GRAPHICS_FUNC_ARGS)
{
	int s = cpart->life;
	if (s>75) s = 75; //These two should not be here.
	if (s<49) s = 49;
	s = (s-49)*3;
	if (s==0) s = 1;
	*colr += s*4;
	*colg += s*1;
	*colb += s*2;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}

void ACID_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ACID";
	elem->Name = "ACID";
	elem->Colour = COLPACK(0xED55FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
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

	elem->Flammable = 40;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x1FE001FE;

	elem->Weight = 10;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 34;
	elem->Latent = 0;
	elem->Description = "Dissolves almost everything.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 75;

	elem->Update = &ACID_update;
	elem->Graphics = &ACID_graphics;
}


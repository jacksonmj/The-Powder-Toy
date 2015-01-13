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

int GEL_update(UPDATE_FUNC_ARGS) 
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	float dx, dy;
	int absorbChanceDenom;
	if (parts[i].tmp>100) parts[i].tmp = 100;
	if (parts[i].tmp<0) parts[i].tmp = 0;
	absorbChanceDenom = parts[i].tmp*10 + 500;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					//Desaturation
					switch (rt)
					{
					case PT_WATR:
					case PT_DSTW:
					case PT_FRZW:
						if (parts[i].tmp<100 && sim->rng.chance(500,absorbChanceDenom))
						{
							parts[i].tmp++;
							kill_part(ri);
						}
						break;
					case PT_PSTE:
						if (parts[i].tmp<100 && sim->rng.chance(20,absorbChanceDenom))
						{
							parts[i].tmp++;
							sim->part_create(ri, x+rx, y+ry, PT_CLST);
						}
						break;
					case PT_SLTW:
						if (parts[i].tmp<100 && sim->rng.chance(50,absorbChanceDenom))
						{
							parts[i].tmp++;
							if (sim->rng.chance<1,4>())
								kill_part(ri);
							else
								part_change_type(ri, x+rx, y+ry, PT_SALT);
						}
						break;
					case PT_CBNW:
						if (parts[i].tmp<100 && sim->rng.chance(100,absorbChanceDenom))
						{
							parts[i].tmp++;
							part_change_type(ri, x+rx, y+ry, PT_CO2);
						}
						break;
					case PT_SPNG:
						if (parts[i].tmp<100 && ((parts[ri].life+1)>parts[i].tmp))
						{
							parts[ri].life--;
							parts[i].tmp++;
						}
						else if ((parts[ri].life+1)<parts[i].tmp)
						{
							parts[ri].life++;
							parts[i].tmp--;
						}
						break;
					case PT_GEL:
						//Concentration diffusion
						if ((parts[ri].tmp+1)<parts[i].tmp)
						{
							parts[ri].tmp++;
							parts[i].tmp--;
						}
						break;
					default:
						break;
					}

					dx = parts[i].x - parts[ri].x;
					dy = parts[i].y - parts[ri].y;

					//Stickness
					if ((dx*dx + dy*dy)>1.5 && (rt==PT_GEL || !ptypes[rt].falldown || (fabs(rx)<2 && fabs(ry)<2)))
					{
						float per, nd;
						nd = dx*dx + dy*dy - 0.5;

						per = 5*(1 - parts[i].tmp/100)*(nd/(dx*dx + dy*dy + nd) - 0.5);
						if (ptypes[rt].state==ST_LIQUID)
							per *= 0.1;
						
						dx *= per; dy *= per;
						parts[i].vx += dx; 
						parts[i].vy += dy; 

						if ((ptypes[rt].properties&TYPE_PART) || rt==PT_GOO)
						{
							parts[ri].vx -= dx;
							parts[ri].vy -= dy;
						}
					}
				}
			}
	return 0;
}

int GEL_graphics(GRAPHICS_FUNC_ARGS)
{
	int q = cpart->tmp;
	*colr = q*(32-255)/120+255;
	*colg = q*(48-186)/120+186;
	*colb = q*208/120;
	return 0;
}

void GEL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_GEL";
	elem->ui->Name = "GEL";
	elem->Colour = COLPACK(0xFF9900);
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

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->ui->Description = "Gel. A liquid with variable viscosity and heat conductivity.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_LIFE_DEC|PROP_NEUTPENETRATE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &GEL_update;
	elem->Graphics = &GEL_graphics;
}


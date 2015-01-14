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

/* ctype - colour RRGGBB (optional)
 * life - decremented each frame, disappears when life reaches zero
 * tmp - mode
 *   0 - BOMB sparks
 *   1 - firework sparks (colour defaults to white)
 *   2 - flash (colour defaults to white)
 */
int EMBR_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if ((sim->elements[parts[ri].type].Properties & (TYPE_SOLID | TYPE_PART | TYPE_LIQUID)) && !(sim->elements[parts[ri].type].Properties & PROP_SPARKSETTLE))
					{
						sim->part_kill(i);
						return 1;
					}
				}
			}
	return 0;
}

int EMBR_graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->ctype&0xFFFFFF)
	{
		int maxComponent;
		*colr = (cpart->ctype&0xFF0000)>>16;
		*colg = (cpart->ctype&0x00FF00)>>8;
		*colb = (cpart->ctype&0x0000FF);
		maxComponent = *colr;
		if (*colg>maxComponent) maxComponent = *colg;
		if (*colb>maxComponent) maxComponent = *colb;
		if (maxComponent<60)//make sure it isn't too dark to see
		{
			float multiplier = 60.0f/maxComponent;
			*colr *= multiplier;
			*colg *= multiplier;
			*colb *= multiplier;
		}
	}
	else if (cpart->tmp != 0)
	{
		*colr = *colg = *colb = 255;
	}

	if (decorations_enable && cpart->dcolour)
	{
		int a = (cpart->dcolour>>24)&0xFF;
		*colr = (a*((cpart->dcolour>>16)&0xFF) + (255-a)**colr) >> 8;
		*colg = (a*((cpart->dcolour>>8)&0xFF) + (255-a)**colg) >> 8;
		*colb = (a*((cpart->dcolour)&0xFF) + (255-a)**colb) >> 8;
	}
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	if (cpart->tmp==1)
	{
		*pixel_mode = FIRE_ADD | PMODE_BLEND | PMODE_GLOW;
		*firea = (cpart->life-15)*4;
		*cola = (cpart->life+15)*4;
	}
	else if (cpart->tmp==2)
	{
		*pixel_mode = PMODE_FLAT | FIRE_ADD;
		*firea = 255;
	}
	else
	{
		*pixel_mode = PMODE_SPARK | PMODE_ADD;
		if (cpart->life<64) *cola = 4*cpart->life;
	}
	return 0;
}

void EMBR_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_EMBR";
	elem->ui->Name = "EMBR";
	elem->Colour = COLPACK(0xFFF288);
	elem->ui->MenuVisible = 0;
	elem->ui->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.001f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.90f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.07f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = 500.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->ui->Description = "Sparks. Formed by explosions.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 50;

	elem->Update = &EMBR_update;
	elem->Graphics = &EMBR_graphics;
}


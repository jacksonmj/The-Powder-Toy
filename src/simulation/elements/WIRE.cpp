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
#include "WIRE.hpp"

int WIRE_update(UPDATE_FUNC_ARGS)
{
	int rx,ry,rt;
	int rcount, ri, rnext;

	if (Element_WIRE::isHeadExternal(parts[i]) && !Element_WIRE::wasHeadExternal(parts[i]))
	{
		// Sparked on this frame by external source, so don't set state based on previous self and neighbouring WIRE states
	}
	else
	{
		if (Element_WIRE::wasHead(parts[i]))
		{
			Element_WIRE::setState(parts[i], Element_WIRE::State::Tail);
		}
		else
		{
			Element_WIRE::setState(parts[i], Element_WIRE::State::Inactive);
		}
	}

	int count=0;
	for(rx=-1; rx<2; rx++)
		for(ry=-1; ry<2; ry++)
		{
			if(x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if(rt==PT_SPRK && parts[ri].life==3 && parts[ri].ctype==PT_PSCN)
					{
							Element_WIRE::spark(parts[i]);
							return 0;
					}
					else if (rt==PT_NSCN && Element_WIRE::wasHead(parts[i]))
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
					else if (rt==PT_WIRE && Element_WIRE::wasHead(parts[ri]))
						count++;
				}
			}
		}
	if (Element_WIRE::canSpark(parts[i]) && (count==1 || count==2))
		Element_WIRE::setState(parts[i], Element_WIRE::State::HeadNormal);
	return 0;
}

int WIRE_graphics(GRAPHICS_FUNC_ARGS)
{
	if (Element_WIRE::isHead(*cpart))
	{
		*colr = 50;
		*colg = 100;
		*colb = 255;
		//*pixel_mode |= PMODE_GLOW;
		return 0;
	}
	if (Element_WIRE::isTail(*cpart))
	{
		*colr = 255;
		*colg = 100;
		*colb = 50;
		//*pixel_mode |= PMODE_GLOW;
		return 0;
	}
	else
	{
		*colr = 255;
		*colg = 204;
		*colb = 0;
		return 0;
	}
}

void WIRE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_WIRE";
	elem->ui->Name = "WWLD";
	elem->Colour = COLPACK(0xFFCC00);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 250;
	elem->Latent = 0;
	elem->ui->Description = "WireWorld wires, conducts based on a set of GOL-like rules.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WIRE_update;
	elem->Graphics = &WIRE_graphics;
}


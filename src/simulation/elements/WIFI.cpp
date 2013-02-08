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
#include "simulation/ElementDataContainer.h"

class WIFI_ElementDataContainer : public ElementDataContainer
{
public:
	int wireless[CHANNELS][2];
	bool wifi_lastframe;
	WIFI_ElementDataContainer()
	{
		memset(wireless, 0, sizeof(wireless));
		wifi_lastframe = false;
	}
	virtual void Simulation_Cleared(Simulation *sim)
	{
		memset(wireless, 0, sizeof(wireless));
		wifi_lastframe = false;
	}
	virtual void Simulation_BeforeUpdate(Simulation *sim)
	{
		if (!sim->elementCount[PT_WIFI] && !wifi_lastframe)
		{
			return;
		}
		if (sim->elementCount[PT_WIFI])
		{
			wifi_lastframe = true;
		}
		else
		{
			wifi_lastframe = false;
		}

		int q;
		for ( q = 0; q<(int)(MAX_TEMP-73.15f)/100+2; q++)
		{
			wireless[q][0] = wireless[q][1];
			wireless[q][1] = 0;
		}
	}
};

int WIFI_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	parts[i].tmp = (int)((parts[i].temp-73.15f)/100+1);
	if (parts[i].tmp>=CHANNELS) parts[i].tmp = CHANNELS-1;
	else if (parts[i].tmp<0) parts[i].tmp = 0;
	int (*channel) = ((WIFI_ElementDataContainer*)sim->elementData[PT_WIFI])->wireless[parts[i].tmp];
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				// channel[0] - whether channel is active on this frame
				// channel[1] - whether channel should be active on next frame
				if (channel[0])
				{
					if (((r&0xFF)==PT_NSCN||(r&0xFF)==PT_PSCN||(r&0xFF)==PT_INWR)&&parts[r>>8].life==0)
					{
						sim->spark_conductive(r>>8, x+rx, y+ry);
					}
				}
				else
				{
					if ((r&0xFF)==PT_SPRK && parts[r>>8].ctype!=PT_NSCN && parts[r>>8].life>=3)
					{
						channel[1] = 1;
					}
				}
			}
	return 0;
}

int WIFI_graphics(GRAPHICS_FUNC_ARGS)
{
	float frequency = 0.0628;
	int q = cpart->tmp;
	*colr = sin(frequency*q + 0) * 127 + 128;
	*colg = sin(frequency*q + 2) * 127 + 128;
	*colb = sin(frequency*q + 4) * 127 + 128;
	*pixel_mode |= EFFECT_LINES;
	return 0;
}

void WIFI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_WIFI";
	elem->Name = "WIFI";
	elem->Colour = COLPACK(0x40A060);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
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
	elem->Hardness = 2;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Wireless transmitter, color coded.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 15.0f;
	elem->HighPressureTransitionElement = PT_BRMT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WIFI_update;
	elem->Graphics = &WIFI_graphics;

	if (sim->elementData[t])
	{
		delete sim->elementData[t];
	}
	sim->elementData[t] = new WIFI_ElementDataContainer;
}


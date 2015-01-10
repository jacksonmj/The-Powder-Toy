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
#include "simulation/ElemDataSim.h"
#include "simulation/elements/WIFI.h"

WIFI_ElemDataSim::WIFI_ElemDataSim(Simulation *s, float chanStep, int chanCount)
	: ElemDataSim_channels(s),
	  obs_simCleared(sim->hook_cleared, this, &WIFI_ElemDataSim::Simulation_Cleared),
	  obs_simBeforeUpdate(sim->hook_beforeUpdate, this, &WIFI_ElemDataSim::Simulation_BeforeUpdate),
	  channelStep(chanStep),
	  channelCount(chanCount)
{
	if (channelStep<1)
		channelStep = 1;
	if (channelCount<0)//default value=-1 : set channelCount automatically based on channelStep
		channelCount = 1 + int((MAX_TEMP-73.15f)/channelStep+1) - int((MIN_TEMP-73.15f)/channelStep+1);

	channels = new WifiChannel[channelCount];
	Simulation_Cleared();
}

WIFI_ElemDataSim::~WIFI_ElemDataSim()
{
	delete[] channels;
}

void WIFI_ElemDataSim::Simulation_Cleared()
{
	for (int q=0; q<channelCount; q++)
	{
		channels[q].activeThisFrame = false;
		channels[q].activeNextFrame = false;
	}
	wifi_lastframe = false;
}

void WIFI_ElemDataSim::Simulation_BeforeUpdate()
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

	for (int q = 0; q<channelCount; q++)
	{
		channels[q].activeThisFrame = channels[q].activeNextFrame;
		channels[q].activeNextFrame = false;
	}
}

int WIFI_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt;
	int rcount, ri, rnext;
	WIFI_ElemDataSim *ed = sim->elemData<WIFI_ElemDataSim>(PT_WIFI);
	parts[i].tmp = ed->GetChannelId(parts[i]);
	WifiChannel *channel = ed->GetParticleChannel(parts[i]);
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (channel->activeThisFrame)
					{
						if ((rt==PT_NSCN||rt==PT_PSCN||rt==PT_INWR)&&parts[ri].life==0)
						{
							sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
						}
					}
					else
					{
						if (rt==PT_SPRK && parts[ri].ctype!=PT_NSCN && parts[ri].life==3)
						{
							channel->activeNextFrame = 1;
						}
					}
				}
			}
	return 0;
}

int WIFI_graphics(GRAPHICS_FUNC_ARGS)
{
	float frequency = 0.0628;
	int q = sim->elemData<WIFI_ElemDataSim>(PT_WIFI)->GetChannelId(*cpart);
	*colr = sin(frequency*q + 0) * 127 + 128;
	*colg = sin(frequency*q + 2) * 127 + 128;
	*colb = sin(frequency*q + 4) * 127 + 128;
	*pixel_mode |= EFFECT_LINES;
	return 0;
}

void WIFI_Simulation_Init(ELEMENT_SIMINIT_FUNC_ARGS)
{
	sim->elemData_create<WIFI_ElemDataSim>(t, sim);
}

void WIFI_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_WIFI";
	elem->ui->Name = "WIFI";
	elem->Colour = COLPACK(0x40A060);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
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
	elem->ui->Description = "Wireless transmitter, transfers spark to any other wifi on the same temperature channel.";

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
	elem->Func_SimInit = &SimInit_createElemData<WIFI_ElemDataSim>;
}


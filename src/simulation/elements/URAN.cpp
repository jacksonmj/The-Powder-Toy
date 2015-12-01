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

int URAN_update(UPDATE_FUNC_ARGS)
{
	if (sim->option_heatMode()!=HeatMode::Legacy && sim->air.pv.get(SimPosI(x,y))>0.0f)
	{
		if (parts[i].temp-MIN_TEMP < 0.01f)
		{
			parts[i].temp += .01f;
		}
		else
		{
			float tempScaleFactor = 1+sim->air.pv.get(SimPosI(x,y))/2000;
			sim->part_set_temp(parts[i], (parts[i].temp-MIN_TEMP)*tempScaleFactor + MIN_TEMP);
		}
	}
	return 0;
}

void URAN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_URAN";
	elem->ui->Name = "URAN";
	elem->Colour = COLPACK(0x707020);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.4f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;
	elem->PhotonReflectWavelengths = 0x003FC000;

	elem->Weight = 90;

	elem->DefaultProperties.temp = R_TEMP+30.0f+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Heavy particles. Generates heat under pressure.";

	elem->Properties = TYPE_PART | PROP_RADIOACTIVE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &URAN_update;
	elem->Graphics = NULL;
}


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

int NBLE_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 5273.15 && pv[y/CELL][x/CELL] > 100.0f)
	{
		parts[i].tmp |= 0x1;
		if (sim->rng.chance<1,5>())
		{
			int j, rx, ry;
			float temp = parts[i].temp;
			sim->part_create(i,x,y,PT_CO2);

			sim->randomRelPos_1(&rx,&ry);
			j = sim->part_create(-3,x+rx,y+ry,PT_NEUT);
			if (j != -1)
				sim->part_set_temp(parts[j], temp);
			if (sim->rng.chance<1,25>())
			{
				sim->randomRelPos_1(&rx,&ry);
				j = sim->part_create(-3,x+rx,y+ry,PT_ELEC);
				if (j != -1)
					sim->part_set_temp(parts[j], temp);
			}
			sim->randomRelPos_1(&rx,&ry);
			j = sim->part_create(-3,x+rx,y+ry,PT_PHOT);
			if (j != -1)
			{
				parts[j].ctype = 0xF800000;
				sim->part_set_temp(parts[j], temp);
				parts[j].tmp = 0x1;
			}

			sim->randomRelPos_1(&rx,&ry);
			j = sim->part_create(-3,x+rx,y+ry,PT_PLSM);
			if (j != -1)
			{
				sim->part_set_temp(parts[j], temp);
				parts[j].tmp |= 4;
			}

			sim->part_set_temp(parts[i], temp+sim->rng.randInt<1750,1750+499>());
			pv[y/CELL][x/CELL] += 50;
		}
	}
	return 0;
}

void NBLE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_NBLE";
	elem->ui->Name = "NBLE";
	elem->Colour = COLPACK(0xEB4917);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_GAS;
	elem->Enabled = 1;

	elem->Advection = 1.0f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.99f;
	elem->Loss = 0.30f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.75f;
	elem->PressureAdd_NoAmbHeat = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x3FFF8000;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP+2.0f	+273.15f;
	elem->HeatConduct = 106;
	elem->Latent = 0;
	elem->ui->Description = "Noble Gas. Diffuses and conductive. Ionizes into plasma when introduced to electricity.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_CONDUCTS|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &NBLE_update;
	elem->Graphics = NULL;
}


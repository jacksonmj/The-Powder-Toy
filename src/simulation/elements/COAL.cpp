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

int COAL_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life<=0) {
		sim->part_create(i, x, y, PT_FIRE);
		return 1;
	} else if (parts[i].life < 100) {
		parts[i].life--;
		int rx, ry;
		sim->randomRelPos_1_noCentre(&rx, &ry);
		sim->part_create(-1, x+rx, y+ry, PT_FIRE);
	}
	if ((sim->air.pv.get(SimPosI(x,y)) > 4.3f)&&parts[i].tmp>40)
		parts[i].tmp=39;
	else if (parts[i].tmp<40&&parts[i].tmp>0)
		parts[i].tmp--;
	else if (parts[i].tmp<=0) {
		sim->part_create(i, x, y, PT_BCOL);
		return 1;
	}
	if(parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = parts[i].temp;
	return 0;
}

int COAL_graphics(GRAPHICS_FUNC_ARGS) //Both COAL and Broken Coal
{
	*colr += (cpart->tmp2-295.15f)/3;
	
	if (*colr > 170)
		*colr = 170;
	if (*colr < *colg)
		*colr = *colg;
		
	*colg = *colb = *colr;

	if((cpart->temp-295.15f) > 300.0f-200.0f)
	{
		float frequency = 3.1415/(2*300.0f-(300.0f-200.0f));
		int q = ((cpart->temp-295.15f)>300.0f)?300.0f-(300.0f-200.0f):(cpart->temp-295.15f)-(300.0f-200.0f);

		*colr += sin(frequency*q) * 226;
		*colg += sin(frequency*q*4.55 +3.14) * 34;
		*colb += sin(frequency*q*2.22 +3.14) * 64;
	}
	return 0;
}

void COAL_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_COAL";
	elem->ui->Name = "COAL";
	elem->Colour = COLPACK(0x222222);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.0f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;
	elem->PhotonReflectWavelengths = 0x00000000;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 200;
	elem->Latent = 0;
	elem->ui->Description = "Coal, Burns very slowly. Gets red when hot.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 110;
	elem->DefaultProperties.tmp = 50;

	elem->Update = &COAL_update;
	elem->Graphics = &COAL_graphics;
}


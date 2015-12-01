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

int FWRK_update(UPDATE_FUNC_ARGS)
{
	int np;
	if (parts[i].life==0 && ((parts[i].temp>400 && sim->rng.chance(9+parts[i].temp/40,100000) && surround_space)||parts[i].ctype==PT_DUST))
	{
		float gx, gy, multiplier, gmax;
		sim->GetGravityAccel(x,y, sim->elements[PT_FWRK].Gravity, 1.0f, gx,gy);
		if (gx*gx+gy*gy < 0.001f)
		{
			// in low gravity, use a vector with random direction instead
			float angle = sim->rng.randFloat(0, 2*M_PI);
			gx += sinf(angle)*sim->elements[PT_FWRK].Gravity*0.5f;
			gy += cosf(angle)*sim->elements[PT_FWRK].Gravity*0.5f;
		}
		gmax = fmaxf(fabsf(gx), fabsf(gy));
		if (MoveResult::WillSucceed(sim->part_canMove(PT_FWRK, (int)(x-(gx/gmax)+0.5f), (int)(y-(gy/gmax)+0.5f))))
		{
			multiplier = 15.0f/sqrtf(gx*gx+gy*gy);

			float randTmp;
			//Some variation in speed parallel to gravity direction
			randTmp = sim->rng.randFloat(-0.2f,0.2f);
			gx += gx*randTmp;
			gy += gy*randTmp;
			//and a bit more variation in speed perpendicular to gravity direction
			randTmp = sim->rng.randFloat(-0.5f,0.5f);
			gx += -gy*randTmp;
			gy += gx*randTmp;

			parts[i].life=sim->rng.randInt<18,27>();
			parts[i].ctype=0;
			parts[i].vx -= gx*multiplier;
			parts[i].vy -= gy*multiplier;
			return 0;
		}
	}
	if (parts[i].life<3&&parts[i].life>0)
	{
		int r = sim->rng.randInt<11,255>();
		int g = sim->rng.randInt<11,255>();
		int b = sim->rng.randInt<11,255>();
		int n;
		float angle, magnitude;
		unsigned col = (r<<16) | (g<<8) | b;
		for (n=0; n<40; n++)
		{
			np = sim->part_create(-3, x, y, PT_EMBR);
			if (np>-1)
			{
				magnitude = sim->rng.randInt<40,100>()*0.05f;
				angle = sim->rng.randFloat(0, 2*M_PI);
				parts[np].vx = parts[i].vx*0.5f + cosf(angle)*magnitude;
				parts[np].vy = parts[i].vy*0.5f + sinf(angle)*magnitude;
				parts[np].ctype = col;
				parts[np].tmp = 1;
				parts[np].life = sim->rng.randInt<70,70+39>();
				sim->part_set_temp(parts[np], sim->rng.randInt<5750,5750+499>());
				parts[np].dcolour = parts[i].dcolour;
			}
		}
		sim->air.pv.add(SimPosI(x,y), 8.0f);
		sim->part_kill(i);
		return 1;
	}
	if (parts[i].life>=45)
		parts[i].life=0;
	return 0;
}

void FWRK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_FWRK";
	elem->ui->Name = "FWRK";
	elem->Colour = COLPACK(0x666666);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
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
	elem->Hardness = 1;

	elem->Weight = 97;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->ui->Description = "Original version of fireworks, activated by heat/neutrons.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &FWRK_update;
	elem->Graphics = NULL;
}


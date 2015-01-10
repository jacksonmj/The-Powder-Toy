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

void PROT_DeutImplosion(Simulation * sim, int n, int x, int y, float temp, int t)
{
	int i;
	n = (n/50);
	if (n < 1)
		n = 1;
	else if (n > 340)
		n = 340;

	for (int c = 0; c < n; c++)
	{
		i = sim->part_create(-3, x, y, t);
		if (i >= 0)
			sim->parts[i].temp = temp;
		else if (sim->pfree < 0)
			break;
	}
	pv[y/CELL][x/CELL] -= (6.0f * CFDS)*n;
}

int PROT_update(UPDATE_FUNC_ARGS)
{
	pv[y/CELL][x/CELL] -= .003f;
	int rcount, ri, rnext, rt;
	//slowly kill it if it's not inside an element
	if (!sim->pmap[y][x].count_notEnergy && parts[i].life)
	{
		if (!--parts[i].life)
		{
			sim->part_kill(i);
			return 1;
		}
	}
	//if this proton has collided with another last frame, change it into a heavier element
	if (parts[i].tmp)
	{
		int newID, element;
		if (parts[i].tmp > 500000)
			element = PT_SING; //particle accelerators are known to create earth-destroying black holes
		else if (parts[i].tmp > 500)
			element = PT_PLUT;
		else if (parts[i].tmp > 320)
			element = PT_URAN;
		else if (parts[i].tmp > 150)
			element = PT_PLSM;
		else if (parts[i].tmp > 50)
			element = PT_O2;
		else if (parts[i].tmp > 20)
			element = PT_CO2;
		else
			element = PT_NBLE;
		newID = sim->part_create(-1, x+rand()%3-1, y+rand()%3-1, element);
		if (newID>=0)
		{
			parts[newID].temp = restrict_flt(100.0f*parts[i].tmp, MIN_TEMP, MAX_TEMP);
			sim->part_kill(i);
			return 1;
		}
	}

	FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)
	{
		rt = parts[ri].type;
		//set off explosives (only when hot because it wasn't as fun when it made an entire save explode)
		if (parts[i].temp > 273.15f+500.0f && (sim->elements[rt].Flammable || sim->elements[rt].Explosive || rt == PT_BANG))
		{
			sim->part_create(ri, x, y, PT_FIRE);
			parts[ri].temp += restrict_flt(sim->elements[rt].Flammable*5, MIN_TEMP, MAX_TEMP);
			pv[y/CELL][x/CELL] += 1.00f;
		}
		//remove active sparks
		else if (rt == PT_SPRK)
		{
			sim->part_change_type(ri, x, y, parts[ri].ctype);
			parts[ri].life = 44+parts[ri].life;
			parts[ri].ctype = 0;
		}
		else if (rt == PT_DEUT)
		{
			if ((-((int)pv[y/CELL][x/CELL]-4)+(parts[ri].life/100)) > rand()%200)
			{
				PROT_DeutImplosion(sim, parts[ri].life, x, y, restrict_flt(parts[ri].temp + parts[ri].life*500, MIN_TEMP, MAX_TEMP), PT_PROT);
				sim->part_kill(ri);
				continue;
			}
		}
		//prevent inactive sparkable elements from being sparked
		else if ((sim->elements[rt].Properties&PROP_CONDUCTS) && parts[ri].life <= 4)
		{
			parts[ri].life = 40+parts[ri].life;
		}
		//Powered LCRY reaction: PROT->PHOT
		else if (rt == PT_LCRY && parts[ri].life > 5 && !(rand()%10))
		{
			sim->part_change_type(i, x, y, PT_PHOT);
			parts[i].life *= 2;
			parts[i].ctype = 0x3FFFFFFF;
		} 
		else if (rt == PT_EXOT)
			parts[ri].ctype = PT_PROT;

		//make temp of other things closer to it's own temperature. This will change temp of things that don't conduct, and won't change the PROT's temperature
		//now changed so that PROT goes through portal, so only the WIFI part applies
		if (rt == PT_WIFI/* || (under&0xFF) == PT_PRTI || (under&0xFF) == PT_PRTO*/)
		{
			float change;
			if (parts[i].temp<173.15f) change = -1000.0f;
			else if (parts[i].temp<273.15f) change = -100.0f;
			else if (parts[i].temp>473.15f) change = 1000.0f;
			else if (parts[i].temp>373.15f) change = 100.0f;
			else change = 0.0f;
			parts[ri].temp = restrict_flt(parts[ri].temp+change, MIN_TEMP, MAX_TEMP);
		}
		else if (rt!=PT_PROT)
		{
			parts[ri].temp = restrict_flt(parts[ri].temp-(parts[ri].temp-parts[i].temp)/4.0f, MIN_TEMP, MAX_TEMP);
		}

		//collide with other protons to make heavier materials
		if (ri != i && rt == PT_PROT)
		{
			float velocity1 = powf(parts[i].vx, 2.0f)+powf(parts[i].vy, 2.0f);
			float velocity2 = powf(parts[ri].vx, 2.0f)+powf(parts[ri].vy, 2.0f);
			float normalisedDotProduct = (parts[i].vx*parts[ri].vx + parts[i].vy*parts[ri].vy) / sqrtf(velocity1 * velocity2);
			if (normalisedDotProduct < -0.995f && velocity1 + velocity2 > 10.0f)
			{
				parts[i].tmp += (int)(velocity1 + velocity2) + parts[ri].tmp;
				sim->part_kill(ri);
			}
		}
	}
	return 0;
}

int PROT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 7;
	*firer = 250;
	*fireg = 170;
	*fireb = 170;

	*pixel_mode |= FIRE_BLEND;
	return 1;
}

void PROT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = (rand()%36)* 0.17453f;
	sim->parts[i].vx = 2.0f*cosf(a);
	sim->parts[i].vy = 2.0f*sinf(a);
}

void PROT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_PROT";
	elem->ui->Name = "PROT";
	elem->Colour = COLPACK(0x990000);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+273.15f;
	elem->HeatConduct = 61;
	elem->Latent = 0;
	elem->ui->Description = "Protons. Transfer heat to materials, and removes sparks.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 680;

	elem->Update = &PROT_update;
	elem->Graphics = &PROT_graphics;
	elem->Func_Create = &PROT_create;
}


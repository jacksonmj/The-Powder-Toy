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

int WARP_update(UPDATE_FUNC_ARGS)
{
	int trade, rx, ry, rt;
	int rcount, ri, rnext;

	if (parts[i].tmp2>2000)
	{
		parts[i].temp = 10000;
		sim->air.pv.add(SimCoordI(x,y), (parts[i].tmp2/5000) * CFDS);
		if (sim->rng.chance<1,50>())
			sim->part_create(-3, x, y, PT_ELEC);
	}
	for ( trade = 0; trade<5; trade ++)
	{
		sim->randomRelPos_1_noCentre(&rx,&ry);
		if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
		{
			FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
			{
				rt = parts[ri].type;
				if (rt!=PT_WARP&&rt!=PT_STKM&&rt!=PT_STKM2&&rt!=PT_DMND&&rt!=PT_CLNE&&rt!=PT_BCLN&&rt!=PT_PCLN)
				{
					float tmpx = parts[i].x, tmpy = parts[i].y;
					sim->part_set_pos(i, x, y, parts[ri].x, parts[ri].y);
					sim->part_set_pos(ri, x+rx, y+ry, tmpx, tmpy);
					parts[ri].vx = sim->rng.randInt<0,3>()-1.5;
					parts[ri].vy = sim->rng.randInt<0,3>()-1.5;
					parts[i].life += 4;
					trade = 5;
					break;
				}
			}
		}
	}
	return 0;
}

int WARP_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode &= ~PMODE;
	return 0;
}

void WARP_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.randInt<70,70+94>();
}

void WARP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_WARP";
	elem->ui->Name = "WARP";
	elem->Colour = COLPACK(0x101010);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.8f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.9f;
	elem->Loss = 0.70f;
	elem->Collision = -0.1f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 3.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 30;

	elem->Weight = 1;

	elem->DefaultProperties.temp = R_TEMP +273.15f;
	elem->HeatConduct = 100;
	elem->Latent = 0;
	elem->ui->Description = "Displaces other elements.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_GAS|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &WARP_update;
	elem->Graphics = &WARP_graphics;
	elem->Func_Create = &WARP_create;
}


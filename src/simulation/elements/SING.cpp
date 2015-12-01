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

int SING_update(UPDATE_FUNC_ARGS) {
	int rx, ry, cry, crx, nb, j, spawncount;
	int rcount, ri, rnext;
	int singularity = -parts[i].life;
	float angle, v;

	for (ry=-1; ry<2; ry++)
	{
		for (rx=-1; rx<2; rx++)
		{
			SimPosCell c(x/CELL+rx,y/CELL+ry);
			if (sim->pos_isValid(c))
				sim->air.pv.blend_legacy(c, singularity, 0.1f);
		}
	}
	if (parts[i].life<1) {
		//Pop!
		for (rx=-1; rx<2; rx++) {
			crx = (x/CELL)+rx;
			for (ry=-1; ry<2; ry++) {
				cry = (y/CELL)+ry;
				if (cry >= 0 && crx >= 0 && crx < (XRES/CELL) && cry < (YRES/CELL)) {
					sim->air.pv.add(SimPosI(crx,cry), (float)parts[i].tmp);
				}
			}
		}
		spawncount = (parts[i].tmp>255)?255:parts[i].tmp;
		if (spawncount>=1)
			spawncount = spawncount/8;
		spawncount = spawncount*spawncount*M_PI;
		for (j=0;j<spawncount;j++)
		{
			switch(sim->rng.randInt<0,2>())
			{
				case 0:
					nb = sim->part_create(-3, x, y, PT_PHOT);
					break;
				case 1:
					nb = sim->part_create(-3, x, y, PT_NEUT);
					break;
				case 2:
					nb = sim->part_create(-3, x, y, PT_ELEC);
					break;
			}
			if (nb!=-1) {
				parts[nb].life = sim->rng.randInt<0,299>();
				sim->part_set_temp(parts[nb], MAX_TEMP/2);
				angle = sim->rng.randFloat(0, 2*M_PI);
				v = sim->rng.randFloat(0, 5.0f);
				parts[nb].vx = v*cosf(angle);
				parts[nb].vy = v*sinf(angle);
			}
			else if (sim->pfree==-1)
				break;//if we've run out of particles, stop trying to create them - saves a lot of lag on "sing bomb" saves
		}
		sim->part_kill(i);
		return 1;
	}
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type!=PT_DMND && sim->rng.chance<1,3>())
					{
						if (parts[ri].type==PT_SING && parts[ri].life >10)
						{
							if (parts[i].life+parts[ri].life > 255)
								continue;
							parts[i].life += parts[ri].life;
						}
						else
						{
							if (parts[i].life+3 > 255)
							{
								if (parts[ri].type!=PT_SING && sim->rng.chance<1,100>())
								{
									sim->part_create(ri,x+rx,y+ry,PT_SING);
								}
								continue;
							}
							parts[i].life += 3;
							parts[i].tmp++;
						}
						sim->part_add_temp(parts[i], parts[ri].temp);
						sim->part_kill(ri);
					}
				}
			}
	return 0;
}

void SING_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.randInt<60,60+49>();
}

void SING_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_SING";
	elem->ui->Name = "SING";
	elem->Colour = COLPACK(0x242424);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.7f;
	elem->AirDrag = 0.36f * CFDS;
	elem->AirLoss = 0.96f;
	elem->Loss = 0.80f;
	elem->Collision = 0.1f;
	elem->Gravity = 0.12f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = -0.001f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 86;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 70;
	elem->Latent = 0;
	elem->ui->Description = "Singularity. Creates huge amounts of negative pressure and destroys everything.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &SING_update;
	elem->Graphics = NULL;
	elem->Func_Create = &SING_create;
}


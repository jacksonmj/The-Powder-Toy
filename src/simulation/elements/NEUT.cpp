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
#include "simulation/elements-shared/pyro.h"

int NEUT_DeutExplosion(Simulation *sim, int n, int x, int y, float vx, float vy, float temp)
{
	int i, c;
	n = (n/50);
	if (n<1) {
		n = 1;
	}
	if (n>340) {
		n = 340;
	}
	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return -1;

	for (c=0; c<n; c++) {
		float r = sim->rng.randInt<128,128+127>()/127.0f;
		float a = sim->rng.randInt<0,359>()*M_PI/180.0f;
		i = sim->part_alloc();
		if (i<0)
			return -1;

		parts[i] = sim->elements[PT_NEUT].DefaultProperties;
		parts[i].x = (float)x;
		parts[i].y = (float)y;
#ifdef OGLR
		parts[i].lastX = (float)x;
		parts[i].lastY = (float)y;
#endif
		parts[i].type = PT_NEUT;
		parts[i].life = sim->rng.randInt<480,480+479>();
		parts[i].vx = r*cosf(a);
		parts[i].vy = r*sinf(a);
		sim->pmap_add(i, x, y, PT_NEUT);
		sim->elementCount[PT_NEUT]++;

		sim->air.pv.add(SimPosI(x,y), 6.0f * CFDS);
	}
	return 0;
}

int NEUT_update(UPDATE_FUNC_ARGS)
{
	int rx, ry;
	int rcount, ri, rnext;
	int pressureFactor = 3 + (int)sim->air.pv.get(SimPosI(x,y));
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES)
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					switch (parts[ri].type)
					{
					case PT_WATR:
						if (sim->rng.chance<3,20>())
							sim->part_change_type(ri,x+rx,y+ry,PT_DSTW);
						// fallthrough
					case PT_ICEI:
					case PT_SNOW:
						parts[i].vx *= 0.995;
						parts[i].vy *= 0.995;
						break;
					case PT_PLUT:
						if (sim->rng.chance(pressureFactor,1000))
						{
							if (sim->rng.chance<1,3>())
							{
								sim->part_create(ri, x+rx, y+ry, sim->rng.chance<2,3>() ? PT_LAVA : PT_URAN);
								sim->part_set_temp(parts[ri], MAX_TEMP);
								if (parts[ri].type==PT_LAVA) {
									parts[ri].tmp = 100;
									parts[ri].ctype = PT_PLUT;
								}
							}
							else
							{
								sim->part_create(ri, x+rx, y+ry, PT_NEUT);
								parts[ri].vx = 0.25f*parts[ri].vx + parts[i].vx;
								parts[ri].vy = 0.25f*parts[ri].vy + parts[i].vy;
							}
							sim->air.pv.add(SimPosI(x,y), 10.0f * CFDS); //Used to be 2, some people said nukes weren't powerful enough
							ElementsShared_pyro::update(UPDATE_FUNC_SUBCALL_ARGS);
						}
						break;
					case PT_DEUT:
						if (sim->rng.chance(pressureFactor+1+(parts[ri].life/100),1000))
						{
							NEUT_DeutExplosion(sim, parts[ri].life, x+rx, y+ry, parts[i].vx, parts[i].vy, tptmath::clamp_flt(parts[ri].temp + parts[ri].life*500.0f, MIN_TEMP, MAX_TEMP));
							sim->part_kill(ri);
						}
						break;
					case PT_GUNP:
						if (sim->rng.chance<3,200>())
							sim->part_change_type(ri,x+rx,y+ry,PT_DUST);
						break;
					case PT_DYST:
						if (sim->rng.chance<3,200>())
							sim->part_change_type(ri,x+rx,y+ry,PT_YEST);
						break;
					case PT_YEST:
						sim->part_change_type(ri,x+rx,y+ry,PT_DYST);
						break;
					case PT_PLEX:
						if (sim->rng.chance<3,200>())
							sim->part_change_type(ri,x+rx,y+ry,PT_GOO);
						break;
					case PT_NITR:
						if (sim->rng.chance<3,200>())
							sim->part_change_type(ri,x+rx,y+ry,PT_DESL);
						break;
					case PT_PLNT:
						if (sim->rng.chance<1,20>())
							sim->part_create(ri, x+rx, y+ry, PT_WOOD);
						break;
					case PT_DESL:
					case PT_OIL:
						if (sim->rng.chance<3,200>())
							sim->part_change_type(ri,x+rx,y+ry,PT_GAS);
						break;
					case PT_COAL:
						if (sim->rng.chance<1,20>())
							sim->part_create(ri, x+rx, y+ry, PT_WOOD);
						break;
					case PT_DUST:
						if (sim->rng.chance<1,20>())
							sim->part_change_type(ri, x+rx, y+ry, PT_FWRK);
						break;
					case PT_FWRK:
						if (sim->rng.chance<1,20>())
							parts[ri].ctype = PT_DUST;
						break;
					case PT_ACID:
						if (sim->rng.chance<1,20>())
							sim->part_create(ri, x+rx, y+ry, PT_ISOZ);
						break;
					case PT_TTAN:
						if (sim->rng.chance<1,20>())
						{
							sim->part_kill(i);
							return 1;
						}
						break;
					case PT_EXOT:
						if (sim->rng.chance<1,20>())
							parts[ri].life = 1500;
						break;
					default:
						break;
					}
				}
			}
	return 0;
}

int NEUT_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 120;
	*firer = 10;
	*fireg = 80;
	*fireb = 120;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD;
	return 1;
}

void NEUT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float r = sim->rng.randInt<128,128+127>()/127.0f;
	float a = sim->rng.randInt<0,359>()*3.14159f/180.0f;
	sim->parts[i].life = sim->rng.randInt<480,480+479>();
	sim->parts[i].vx = r*cosf(a);
	sim->parts[i].vy = r*sinf(a);
}

void NEUT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_NEUT";
	elem->ui->Name = "NEUT";
	elem->Colour = COLPACK(0x20E0FF);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.01f;
	elem->PressureAdd_NoAmbHeat = 0.002f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+4.0f	+273.15f;
	elem->HeatConduct = 60;
	elem->Latent = 0;
	elem->ui->Description = "Neutrons. Interact with matter in odd ways.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &NEUT_update;
	elem->Graphics = &NEUT_graphics;
	elem->Func_Create = &NEUT_create;
}


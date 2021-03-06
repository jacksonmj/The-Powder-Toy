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

int BOMB_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, nb;
	int rcount, ri, rnext;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int rt = parts[ri].type;
					if (rt!=PT_BOMB && rt!=PT_EMBR && rt!=PT_DMND && rt!=PT_CLNE && rt!=PT_PCLN && rt!=PT_BCLN && rt!=PT_VIBR)
					{
						int rad = 8;
						int nxi;
						int nxj;
						int nrcount, nri;
						for (nxj=-rad; nxj<=rad; nxj++)
							for (nxi=-rad; nxi<=rad; nxi++)
								if ((pow(nxi,2))/(pow(rad,2))+(pow(nxj,2))/(pow(rad,2))<=1)
								{
									bool posExplode = false;
									FOR_PMAP_POSITION(sim, x+nxi, y+nxj, nrcount, nri, rnext)
									{
										int nrt = parts[nri].type;
										if (nri!=i && nrt!=PT_DMND && nrt!=PT_CLNE && nrt!=PT_PCLN && nrt!=PT_BCLN && nrt!=PT_VIBR)
										{
											sim->part_kill(nri);
											posExplode = true;
										}
									}
									if (posExplode)
									{
										sim->air.pv.add(SimPosI(x+nxi,y+nxj), 0.1f);
										nb = sim->part_create(-3, x+nxi, y+nxj, PT_EMBR);
										if (nb!=-1) {
											parts[nb].tmp = 2;
											parts[nb].life = 2;
											parts[nb].temp = MAX_TEMP;
										}
									}
								}
						for (nxj=-(rad+1); nxj<=(rad+1); nxj++)
							for (nxi=-(rad+1); nxi<=(rad+1); nxi++)
								if ((pow(nxi,2))/(pow((rad+1),2))+(pow(nxj,2))/(pow((rad+1),2))<=1 && !sim->pmap[y+nxj][x+nxi].count(PMapCategory::Plain)) {
									nb = sim->part_create(-3, x+nxi, y+nxj, PT_EMBR);
									if (nb!=-1) {
										parts[nb].tmp = 0;
										parts[nb].life = 50;
										parts[nb].temp = MAX_TEMP;
										parts[nb].vx = sim->rng.randInt<-20,20>();
										parts[nb].vy = sim->rng.randInt<-20,20>();
									}
								}
						sim->part_kill(i);
						return 1;
					}
				}
			}
	return 0;
}

int BOMB_graphics(GRAPHICS_FUNC_ARGS)
{
	*pixel_mode |= PMODE_FLARE;
	return 1;
}

void BOMB_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_BOMB";
	elem->ui->Name = "BOMB";
	elem->Colour = COLPACK(0xFFF288);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 30;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->ui->Description = "Bomb. Explodes and destroys all surrounding particles when it touches something.";

	elem->Properties = TYPE_PART|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC|PROP_SPARKSETTLE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &BOMB_update;
	elem->Graphics = &BOMB_graphics;
}


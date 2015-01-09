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
#include "simulation/elements/FILT.h"

int ARAY_update(UPDATE_FUNC_ARGS)
{
	int nxx, nyy, docontinue, nxi, nyi, rx, ry, np, ry1, rx1;
	int rcount, ri, rnext, scount, si, snext;
	if (parts[i].life==0) {	
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, scount, si, snext)
					{
						if (parts[si].type==PT_SPRK && parts[si].life==3) {
							bool isBlackDeco = false;
							int destroy = (parts[si].ctype==PT_PSCN)?1:0;
							int nostop = (parts[si].ctype==PT_INST)?1:0;
							int colored = 0;
							for (docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi) {
								if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
									break;
								}
								np = sim->part_create(-1, x+nxi+nxx, y+nyi+nyy, PT_BRAY);
								if (np>=0)
								{
									if (destroy)//if it came from PSCN
									{
										parts[np].tmp = 2;
										parts[np].life = 2;
									}
									else
									{
										parts[np].ctype = colored;
									}
									parts[np].temp = parts[i].temp;
									if (isBlackDeco)
										parts[np].dcolour = 0xFF000000;
								} else if (!destroy) {
									FOR_PMAP_POSITION_NOENERGY(sim, x+nxi+nxx, y+nyi+nyy, rcount, ri, rnext)
									{
										int rt = parts[ri].type;
										if (rt==PT_BRAY)
										{
											//cases for hitting different BRAY modes
											switch(parts[ri].tmp) {
											case 0://normal white
												if (nyy!=0 || nxx!=0) {
													parts[ri].life = 1020;//makes it last a while
													parts[ri].tmp = 1;
													if (!parts[ri].ctype)//and colors it if it isn't already
														parts[ri].ctype = colored;
												}
												//fallthrough
											case 2://red bray, stop
											default://stop any other random tmp mode
												docontinue = 0;//then stop it
												break;
											case 1://long life, reset it
												parts[ri].life = 1020;
												//docontinue = 1;
												break;
											}
											if (isBlackDeco)
												parts[ri].dcolour = 0xFF000000;
										} else if (rt==PT_FILT) {//get color if passed through FILT
											if (parts[ri].tmp != 6)
											{
												colored = Element_FILT::interactWavelengths(&parts[ri], colored);
												if (!colored)
													break;
											}
											isBlackDeco = (parts[ri].dcolour==0xFF000000);
											parts[ri].life = 4;
										//this if prevents BRAY from stopping on certain materials
										} else if (rt!=PT_STOR && !sim->part_cmp_conductive(parts[ri], PT_INWR) && rt!=PT_ARAY && rt!=PT_WIFI && !(rt==PT_SWCH && parts[ri].life>=10)) {
											if (nyy!=0 || nxx!=0) {
												sim->spark_particle(ri, x+nxi+nxx, y+nyi+nyy);
											}
											if (!(nostop && parts[ri].type==PT_SPRK && parts[ri].ctype >= 0 && parts[ri].ctype < PT_NUM && (sim->elements[parts[ri].ctype].Properties&PROP_CONDUCTS))) {
											// TODO: when breaking compatibility, sim->part_is_sparkable(parts[ri]) instead
												docontinue = 0;
											}
										} else if(rt==PT_STOR) {
											if(parts[ri].tmp)
											{
												//Cause STOR to release
												for(ry1 = 1; ry1 >= -1; ry1--){
													for(rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1){
														int np = sim->part_create(-1, x+nxi+nxx+rx1, y+nyi+nyy+ry1, parts[ri].tmp);
														if (np!=-1)
														{
															parts[np].temp = parts[ri].temp;
															parts[np].life = parts[ri].tmp2;
															parts[np].tmp = parts[ri].pavg[0];
															parts[np].ctype = parts[ri].pavg[1];
															parts[ri].tmp = 0;
															parts[ri].life = 10;
															break;
														}
													}
												}
											}
											else
											{
												parts[ri].life = 10;
											}
										}
									}
								} else if (destroy) {
									FOR_PMAP_POSITION_NOENERGY(sim, x+nxi+nxx, y+nyi+nyy, rcount, ri, rnext)
									{
										int rt = parts[ri].type;
										if (rt==PT_BRAY) {
											parts[ri].tmp = 2;
											parts[ri].life = 1;
											if (isBlackDeco)
												parts[ri].dcolour = 0xFF000000;
										//this if prevents red BRAY from stopping on certain materials
										} else if (rt==PT_STOR || sim->part_cmp_conductive(parts[ri], PT_INWR) || rt==PT_ARAY || rt==PT_WIFI || rt==PT_FILT || (rt==PT_SWCH && parts[ri].life>=10)) {
											if(rt==PT_STOR)
											{
												parts[ri].tmp = 0;
												parts[ri].life = 0;
											}
											else if (rt==PT_FILT)
											{
												isBlackDeco = (parts[ri].dcolour==0xFF000000);
												parts[ri].life = 2;
											}
										} else {
											docontinue = 0;
										}
									}
								}
							}
							break;//break out of FOR_PMAP_POSITION loop, so that stacked sparks don't cause multiple activation
						}
					}
				}
	}
	return 0;
}

void ARAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_ARAY";
	elem->ui->Name = "ARAY";
	elem->Colour = COLPACK(0xFFBB00);
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
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Ray Emitter. Rays create points when they collide.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ARAY_update;
	elem->Graphics = NULL;
}


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

unsigned int CRAY_wavelengthToDecoColour(int wavelength)
{
	int colr = 0, colg = 0, colb = 0, x;
	unsigned int dcolour = 0;
	for (x=0; x<12; x++) {
		colr += (wavelength >> (x+18)) & 1;
		colb += (wavelength >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		colg += (wavelength >> (x+9))  & 1;
	x = 624/(colr+colg+colb+1);
	colr *= x;
	colg *= x;
	colb *= x;

	if(colr > 255) colr = 255;
	else if(colr < 0) colr = 0;
	if(colg > 255) colg = 255;
	else if(colg < 0) colg = 0;
	if(colb > 255) colb = 255;
	else if(colb < 0) colb = 0;

	return (255<<24) | (colr<<16) | (colg<<8) | colb;
}

int CRAY_update(UPDATE_FUNC_ARGS)
{
	int r, nxx, nyy, docontinue, nxi, nyi, ri, rcount, rnext, rx, ry, nr, ry1, rx1;
	// set ctype to things that touch it if it doesn't have one already
	if(parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !sim->elements[parts[i].ctype].Enabled)
	{
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						int rt = parts[ri].type;
						if (rt!=PT_CRAY && rt!=PT_PSCN && rt!=PT_INST && rt!=PT_METL && rt!=PT_SPRK)
						{
							parts[i].ctype = rt;
							parts[i].temp = parts[ri].temp;
						}
					}
				}
	}
	else if (parts[i].life==0)
	{
		// only fire when life is 0, but nothing sets the life right now
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type==PT_SPRK && parts[ri].life==3)
						{
							//spark found, start creating
							unsigned int colored = 0;
							bool destroy = parts[ri].ctype==PT_PSCN;
							bool nostop = parts[ri].ctype==PT_INST;
							bool createSpark = (parts[ri].ctype==PT_INWR);
							int partsRemaining = 255;
							if (parts[i].tmp) //how far it shoots
								partsRemaining = parts[i].tmp;
							for (docontinue = 1, nxx = 0, nyy = 0, nxi = -rx, nyi = -ry; docontinue; nyy+=nyi, nxx+=nxi) {
								if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
									break;
								}
								if (!sim->IsWallBlocking(x+nxi+nxx, y+nyi+nyy, parts[i].ctype) && (!sim->pmap[y+nyi+nyy][x+nxi+nxx].count_notEnergy || createSpark)) { // create, also set color if it has passed through FILT
									int nr;
									if (parts[i].ctype == PT_LIFE)
										nr = create_part(-1, x+nxi+nxx, y+nyi+nyy, parts[i].ctype|(parts[i].tmp2<<8));
									else if (parts[i].ctype == PT_SPRK)
										nr = sim->spark_position(x+nxi+nxx, y+nyi+nyy);
									else
										nr = sim->part_create(-1, x+nxi+nxx, y+nyi+nyy, parts[i].ctype);
									if (nr>=0) {
										if (colored)
											parts[nr].dcolour = colored;
										parts[nr].temp = parts[i].temp;
										if(!--partsRemaining)
											docontinue = 0;
									}
								}
								else
								{
									int ncount, ni, nnext;
									FOR_PMAP_POSITION_NOENERGY(sim, x+nxi+nxx, y+nyi+nyy, ncount, ni, nnext)
									{
										if (parts[ni].type==PT_FILT)
										{
											// get color if passed through FILT
											if (parts[ni].dcolour == 0xFF000000)
												colored = 0xFF000000;
											else if (parts[ni].tmp==0)
											{
												colored = CRAY_wavelengthToDecoColour(Element_FILT::getWavelengths(sim, &parts[ni]));
											}
											else if (colored==0xFF000000)
												colored = 0;
											parts[ni].life = 4;
										}
										else if (parts[ni].type==PT_CRAY)
										{
											// ignore CRAY - don't destroy or treat as an obstacle
										}
										else if (destroy && (parts[ni].type != PT_DMND))
										{
											sim->part_kill(ni);
											if(!--partsRemaining)
												docontinue = 0;
										}
										else if (!nostop)
											docontinue = 0;// stop at obstacles
									}
								}
								if(!partsRemaining)
									docontinue = 0;
							}
						}
					}
				}
	}
	return 0;
}

void CRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_CRAY";
	elem->ui->Name = "CRAY";
	elem->Colour = COLPACK(0xBBFF00);
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
	elem->ui->Description = "Particle Ray Emitter. Creates a beam of particles set by its ctype, with a range set by tmp.";

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

	elem->Update = &CRAY_update;
	elem->Graphics = NULL;
}

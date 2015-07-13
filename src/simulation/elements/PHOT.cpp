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
#include "simulation/elements/PHOT.h"
#include "simulation/elements-shared/pyro.h"
#include "simulation/elements-shared/Element_UI_ctypeWavelengths.h"

void Element_PHOT::create_gain_photon(Simulation *sim, int pp)//photons from PHOT going through GLOW
{
	float xx, yy;
	int i, temp_bin;

	if (sim->rng.chance<1,2>()) {
		xx = sim->parts[pp].x - 0.3*sim->parts[pp].vy;
		yy = sim->parts[pp].y + 0.3*sim->parts[pp].vx;
	} else {
		xx = sim->parts[pp].x + 0.3*sim->parts[pp].vy;
		yy = sim->parts[pp].y - 0.3*sim->parts[pp].vx;
	}

	SimPosF newPosF = SimPosFT(xx,yy);
	SimPosI newPos = newPosF;

	if (!sim->pos_isValid(newPos))
		return;

	int glow_i = sim->pmap_find_one(newPos, PT_GLOW);
	if (glow_i<0)
		return;

	i = sim->part_create(-1, newPosF, PT_PHOT);
	if (i<0)
		return;

	sim->parts[i].life = 680;
	sim->parts[i].vx = sim->parts[pp].vx;
	sim->parts[i].vy = sim->parts[pp].vy;
	sim->parts[i].temp = sim->parts[glow_i].temp;

	temp_bin = (int)((parts[i].temp-273.0f)*0.25f);
	if (temp_bin < 0) temp_bin = 0;
	if (temp_bin > 25) temp_bin = 25;
	parts[i].ctype = 0x1F << temp_bin;
}

void Element_PHOT::create_cherenkov_photon(Simulation *sim, int pp)//photons from NEUT going through GLAS
{
	int i, nx, ny;
	float r;

	nx = (int)(parts[pp].x + 0.5f);
	ny = (int)(parts[pp].y + 0.5f);
	int glass_i = sim->pmap_find_one(nx, ny, PT_GLAS);
	if (glass_i<0)
		return;

	if (hypotf(sim->parts[pp].vx, sim->parts[pp].vy) < 1.44f)
		return;

	i = sim->part_create(-1, sim->parts[pp].x,sim->parts[pp].y, PT_PHOT);
	if (i<0)
		return;

	sim->parts[i].ctype = 0x00000F80;
	sim->parts[i].life = 680;
	sim->parts[i].temp = sim->parts[glass_i].temp;
	sim->parts[i].pavg[0] = sim->parts[i].pavg[1] = 0.0f;

	if (sim->rng.chance<1,2>()) {
		sim->parts[i].vx = sim->parts[pp].vx - 2.5f*sim->parts[pp].vy;
		sim->parts[i].vy = sim->parts[pp].vy + 2.5f*sim->parts[pp].vx;
	} else {
		sim->parts[i].vx = sim->parts[pp].vx + 2.5f*sim->parts[pp].vy;
		sim->parts[i].vy = sim->parts[pp].vy - 2.5f*sim->parts[pp].vx;
	}

	/* photons have speed of light. no discussion. */
	r = 1.269 / hypotf(sim->parts[i].vx, sim->parts[i].vy);
	sim->parts[i].vx *= r;
	sim->parts[i].vy *= r;
}

int PHOT_update(UPDATE_FUNC_ARGS)
{
	int rt, rx, ry;
	int rcount, ri, rnext;
	float rr, rrr;
	if (!(parts[i].ctype&0x3FFFFFFF)) {
		sim->part_kill(i);
		return 1;
	}
	if (parts[i].temp > 506)
		if (sim->rng.chance<1,10>())
		{
			if (ElementsShared_pyro::update(UPDATE_FUNC_SUBCALL_ARGS)==1)
				return 1;
		}

	bool isQuartz = false;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					// TODO: could these reactions go in PHOT movement?
					if (rt==PT_ISOZ || rt==PT_ISZS)
					{
						parts[i].vx *= 0.90;
						parts[i].vy *= 0.90;
						sim->part_create(ri, x+rx, y+ry, PT_PHOT);
						rrr = sim->rng.randInt<0,359>()*3.14159f/180.0f;
						if (rt==PT_ISOZ)
							rr = sim->rng.randInt<128,128+127>()/127.0f;
						else
							rr = sim->rng.randInt<128,128+227>()/127.0f;
						parts[ri].vx = rr*cosf(rrr);
						parts[ri].vy = rr*sinf(rrr);
						sim->air.pv.add(SimPosI(x,y), -15.0f * CFDS);
					}
					else if (rt==PT_QRTZ && !rx && !ry)
						isQuartz = true;
					else if (rt == PT_FILT && parts[ri].tmp==9)
					{
						parts[i].vx += sim->rng.randFloat(0.5f,-0.5f);
						parts[i].vy += sim->rng.randFloat(0.5f,-0.5f);
					}
				}
			}
	if (isQuartz)
	{
		float a = sim->rng.randInt<0,359>()*3.14159f/180.0f;
		parts[i].vx = 3.0f*cosf(a);
		parts[i].vy = 3.0f*sinf(a);
		if(parts[i].ctype == 0x3FFFFFFF)
			parts[i].ctype = 0x1F << (sim->rng.randInt<0,26>());
		parts[i].life++; //Delay death
	}

	return 0;
}

int PHOT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x = 0;
	*colr = *colg = *colb = 0;
	for (x=0; x<12; x++) {
		*colr += (cpart->ctype >> (x+18)) & 1;
		*colb += (cpart->ctype >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (cpart->ctype >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	*colr *= x;
	*colg *= x;
	*colb *= x;

	*firea = 100;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
	if (cpart->flags & FLAG_PHOTDECO)
	{
		*pixel_mode &= ~NO_DECO;
	}
	return 0;
}

void PHOT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	// default velocity angle is a multiple of 45 degrees, so can use randomRelPos_1 to select it
	int rx, ry;
	sim->randomRelPos_1_noCentre(&rx,&ry);
	// scale so that magnitude of velocity is 3
	const float diagScale = 3.0f/sqrtf(2);
	float scale = (rx && ry) ? diagScale : 3.0f;
	sim->parts[i].vx = scale*rx;
	sim->parts[i].vy = scale*ry;
	int rcount, ri, rnext;
	FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
	{
		if (parts[ri].type==PT_FILT)
			sim->parts[i].ctype = Element_FILT::interactWavelengths(sim, &(sim->parts[ri]), sim->parts[i].ctype);
	}
}

void PHOT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI_ctypeWavelengths>();

	elem->Identifier = "DEFAULT_PT_PHOT";
	elem->ui->Name = "PHOT";
	elem->Colour = COLPACK(0xFFFFFF);
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

	elem->DefaultProperties.temp = R_TEMP+900.0f+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->ui->Description = "Photons. Refracts through glass, scattered by quartz, and color-changed by different elements. Ignites flammable materials.";

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

	elem->DefaultProperties.life = 680;
	elem->DefaultProperties.ctype = 0x3FFFFFFF;

	elem->Update = &PHOT_update;
	elem->Graphics = &PHOT_graphics;
	elem->Func_Create = &PHOT_create;
}


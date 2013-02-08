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

int NPTCT_update(UPDATE_FUNC_ARGS);

int SPRK_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, rt, conduct_sprk, nearp, pavg, ct = parts[i].ctype;
	int rcount, ri, rnext;
	update_PYRO(UPDATE_FUNC_SUBCALL_ARGS);

	if (parts[i].life<=0)
	{
		if (ct==PT_WATR||ct==PT_SLTW||ct==PT_PSCN||ct==PT_NSCN||ct==PT_ETRD||ct==PT_INWR)
			parts[i].temp = R_TEMP + 273.15f;
		if (ct<=0 || ct>=PT_NUM || !ptypes[ct].enabled)
			ct = PT_METL;
		part_change_type(i,x,y,ct);
		parts[i].ctype = PT_NONE;
		parts[i].life = 4;
		if (ct == PT_WATR)
			parts[i].life = 64;
		if (ct == PT_SLTW)
			parts[i].life = 54;
		if (ct == PT_SWCH)
			parts[i].life = 14;
		return 0;
	}
	if (ct==PT_SPRK)
	{
		kill_part(i);
		return 1;
	}
	else if (ct==PT_NTCT || ct==PT_PTCT)
	{
		NPTCT_update(UPDATE_FUNC_SUBCALL_ARGS);
	}
	else if (ct==PT_ETRD&&parts[i].life==1)
	{
		nearp = nearest_part(i, PT_ETRD, -1);
		if (nearp!=-1&&parts_avg(i, nearp, PT_INSL)!=PT_INSL)
		{
			create_line(x, y, (int)(parts[nearp].x+0.5f), (int)(parts[nearp].y+0.5f), 0, 0, PT_PLSM, 0);
			part_change_type(i,x,y,ct);
			ct = parts[i].ctype = PT_NONE;
			parts[i].life = 20;
			part_change_type(nearp,(int)(parts[nearp].x+0.5f),(int)(parts[nearp].y+0.5f),PT_SPRK);
			parts[nearp].life = 9;
			parts[nearp].ctype = PT_ETRD;
		}
	}
	else if (ct==PT_NBLE&&parts[i].life<=1&&parts[i].temp<5273.15)
	{
		parts[i].life = rand()%150+50;
		part_change_type(i,x,y,PT_PLSM);
		parts[i].ctype = PT_NBLE;
		parts[i].temp = 3500;
		pv[y/CELL][x/CELL] += 1;
	}
	else if (ct==PT_TESC) // tesla coil code
	{
		if (parts[i].tmp>300)
			parts[i].tmp=300;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					// TODO: pmap: if (!r)
					if (parts[i].tmp>4 && rand()%(parts[i].tmp*parts[i].tmp/20+6)==0)
					{
						int p=sim->part_create(-1, x+rx*2, y+ry*2, PT_LIGH);
						if (p!=-1)
						{
							parts[p].life=rand()%(2+parts[i].tmp/15)+parts[i].tmp/7;
							if (parts[i].life>60)
								parts[i].life=60;
							parts[p].temp=parts[p].life*parts[i].tmp/2.5;
							parts[p].tmp2=1;
							parts[p].tmp=atan2(-ry, rx)/M_PI*360;
							parts[i].temp-=parts[i].tmp*2+parts[i].temp/5; // slight self-cooling
							if (fabs(pv[y/CELL][x/CELL])!=0.0f)
							{
								if (fabs(pv[y/CELL][x/CELL])<=0.5f)
									pv[y/CELL][x/CELL]=0;
								else
									pv[y/CELL][x/CELL]-=(pv[y/CELL][x/CELL]>0)?0.5:-0.5;
							}
						}
					}
				}
	}
	else if (ct==PT_IRON) {
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
					{
						rt = parts[ri].type;
						if ((rt == PT_DSTW && 30>(rand()/(RAND_MAX/1000))) ||
								(rt == PT_SLTW && 30>(rand()/(RAND_MAX/1000))) ||
								(rt == PT_WATR && 30>(rand()/(RAND_MAX/1000))))
						{
							if (rand()<RAND_MAX/3)
								part_change_type(ri,x+rx,y+ry,PT_O2);
							else
								part_change_type(ri,x+rx,y+ry,PT_H2);
						}
					}
				}
	}
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)// TODO: not energy parts
				{
					rt = parts[ri].type;
					conduct_sprk = 1;

					pavg = parts_avg(ri, i,PT_INSL);// TODO: only evaluate this once for each position
					if ((rt==PT_SWCH||(rt==PT_SPRK&&parts[ri].ctype==PT_SWCH)) && pavg!=PT_INSL && parts[i].life<4) // make sparked SWCH turn off correctly
					{
						if (rt==PT_SWCH&&ct==PT_PSCN&&parts[ri].life<10) {
							parts[ri].life = 10;
						}
						if (ct==PT_NSCN) {
							part_change_type(ri,x+rx,y+ry,PT_SWCH);
							parts[ri].ctype = PT_NONE;
							parts[ri].life = 9;
						}
					}
					else if ((ct==PT_PSCN||ct==PT_NSCN) && (rt==PT_PUMP||rt==PT_GPMP||rt==PT_HSWC||rt==PT_PBCN) && parts[i].life<4) // PROP_PTOGGLE, Maybe? We seem to use 2 different methods for handling actived elements, this one seems better. Yes, use this one for new elements, PCLN is different for compatibility with existing saves
					{
						if (ct==PT_PSCN) parts[ri].life = 10;
						else if (ct==PT_NSCN && parts[ri].life>=10) parts[ri].life = 9;
					}
					else if ((ct==PT_PSCN||ct==PT_NSCN) && (rt==PT_LCRY&&abs(rx)<2&&abs(ry)<2) && parts[i].life<4)
					{
						if (ct==PT_PSCN && parts[ri].tmp == 0) parts[ri].tmp = 2;
						else if (ct==PT_NSCN && parts[ri].tmp == 3) parts[ri].tmp = 1;
					}

					if (rt == PT_PPIP && parts[i].life == 3 && pavg!=PT_INSL)
					{
						if (ct == PT_NSCN || ct == PT_PSCN || ct == PT_INST)
							PPIP_flood_trigger(sim, x+rx, y+ry, ct);
					}

					// ct = spark from material, rt = spark to material. Make conduct_sprk = 0 if conduction not allowed

					if (pavg == PT_INSL) conduct_sprk = 0;
					if (!(ptypes[rt].properties&PROP_CONDUCTS||rt==PT_INST||rt==PT_QRTZ)) conduct_sprk = 0;
					if (abs(rx)+abs(ry)>=4 &&ct!=PT_SWCH&&rt!=PT_SWCH)
						conduct_sprk = 0;


					if (ct==PT_METL && (rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR||(rt==PT_SPRK&&(parts[ri].ctype==PT_NTCT||parts[ri].ctype==PT_PTCT))) && pavg!=PT_INSL && parts[i].life<4)
					{
						parts[ri].temp = 473.0f;
						if (rt==PT_NTCT||rt==PT_PTCT)
							conduct_sprk = 0;
					}
					if (ct==PT_NTCT && !(rt==PT_PSCN || rt==PT_NTCT || (rt==PT_NSCN&&parts[i].temp>373.0f)))
						conduct_sprk = 0;
					if (ct==PT_PTCT && !(rt==PT_PSCN || rt==PT_PTCT || (rt==PT_NSCN&&parts[i].temp<373.0f)))
						conduct_sprk = 0;
					if (ct==PT_INWR && !(rt==PT_NSCN || rt==PT_INWR || rt==PT_PSCN))
						conduct_sprk = 0;
					if (ct==PT_NSCN && rt==PT_PSCN)
						conduct_sprk = 0;
					if (ct==PT_ETRD && !(rt==PT_METL||rt==PT_ETRD||rt==PT_BMTL||rt==PT_BRMT||rt==PT_LRBD||rt==PT_RBDM||rt==PT_PSCN||rt==PT_NSCN))
						conduct_sprk = 0;
					if (ct==PT_INST&&rt!=PT_NSCN) conduct_sprk = 0;
					if (ct==PT_SWCH && (rt==PT_PSCN||rt==PT_NSCN||rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR))
						conduct_sprk = 0;
					if (rt==PT_QRTZ && !((ct==PT_NSCN||ct==PT_METL||ct==PT_PSCN||ct==PT_QRTZ) && (parts[ri].temp<173.15||pv[(y+ry)/CELL][(x+rx)/CELL]>8)))
						conduct_sprk = 0;
					if (rt==PT_NTCT && !(ct==PT_NSCN || ct==PT_NTCT || (ct==PT_PSCN&&parts[ri].temp>373.0f)))
						conduct_sprk = 0;
					if (rt==PT_PTCT && !(ct==PT_NSCN || ct==PT_PTCT || (ct==PT_PSCN&&parts[ri].temp<373.0f)))
						conduct_sprk = 0;
					if (rt==PT_INWR && !(ct==PT_NSCN || ct==PT_INWR || ct==PT_PSCN))
						conduct_sprk = 0;
					if (rt==PT_INST&&ct!=PT_PSCN)
						conduct_sprk = 0;
					if (rt == PT_NBLE && parts[ri].temp > 5273.15)
						conduct_sprk = 0;

					if (conduct_sprk) {
						if (rt==PT_WATR||rt==PT_SLTW)
						{
							if (parts[ri].life==0 && parts[i].life<3)
							{
								part_change_type(ri,x+rx,y+ry,PT_SPRK);
								if (rt==PT_WATR) parts[ri].life = 6;
								else parts[ri].life = 5;
								parts[ri].ctype = rt;
							}
						}
						else if (rt==PT_INST)
						{
							if (parts[ri].life==0 && parts[i].life<4)
							{
								flood_INST(x+rx,y+ry,PT_SPRK,PT_INST);//spark the wire
							}
						}
						else if (parts[ri].life==0 && parts[i].life<4)
						{
							parts[ri].life = 4;
							parts[ri].ctype = rt;
							part_change_type(ri,x+rx,y+ry,PT_SPRK);
							if (parts[ri].temp+10.0f<673.0f&&!legacy_enable&&(rt==PT_METL||rt==PT_BMTL||rt==PT_BRMT||rt==PT_PSCN||rt==PT_NSCN||rt==PT_ETRD||rt==PT_NBLE||rt==PT_IRON))
								parts[ri].temp = parts[ri].temp+10.0f;
						}
						else if (ct==PT_ETRD && parts[i].life==5)
						{
							part_change_type(i,x,y,ct);
							parts[i].ctype = PT_NONE;
							parts[i].life = 20;
							parts[ri].life = 4;
							parts[ri].ctype = rt;
							part_change_type(ri,x+rx,y+ry,PT_SPRK);
						}
					}
				}
			}
	return 0;
}

int SPRK_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 80;
	
	*firer = 170;
	*fireg = 200;
	*fireb = 220;
	//*pixel_mode |= FIRE_ADD;
	*pixel_mode |= FIRE_ADD;
	return 1;
}

void SPRK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPRK";
	elem->Name = "SPRK";
	elem->Colour = COLPACK(0xFFFF80);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x00000000;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Electricity. Conducted by metal and water.";

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

	elem->Update = &SPRK_update;
	elem->Graphics = &SPRK_graphics;
}


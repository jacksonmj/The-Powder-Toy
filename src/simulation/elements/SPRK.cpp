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
	int rx, ry, nearp, ct = parts[i].ctype;
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
		else if (ct == PT_SLTW)
			parts[i].life = 54;
		else if (ct == PT_SWCH)
			parts[i].life = 14;
		return 0;
	}
	switch (ct)
	{
	case PT_SPRK:
		kill_part(i);
		return 1;
	case PT_NTCT:
	case PT_PTCT:
		NPTCT_update(UPDATE_FUNC_SUBCALL_ARGS);
		break;
	case PT_ETRD:
		if (parts[i].life==1)
		{
			nearp = nearest_part(i, PT_ETRD, -1);
			if (nearp!=-1&& !sim->is_spark_blocked(i, nearp))
			{
				create_line(x, y, (int)(parts[nearp].x+0.5f), (int)(parts[nearp].y+0.5f), 0, 0, PT_PLSM, 0);
				part_change_type(i,x,y,ct);
				ct = parts[i].ctype = PT_NONE;
				parts[i].life = 20;
				sim->spark_particle_conductiveOnly(nearp, (int)(parts[nearp].x+0.5f),(int)(parts[nearp].y+0.5f));
				parts[nearp].life = 9;
			}
		}
		break;
	case PT_NBLE:
		if (parts[i].life<=1&&parts[i].temp<5273.15)
		{
			parts[i].life = rand()%150+50;
			part_change_type(i,x,y,PT_PLSM);
			parts[i].ctype = PT_NBLE;
			if (parts[i].temp > 5273.15)
				parts[i].tmp |= 4;
			parts[i].temp = 3500;
			pv[y/CELL][x/CELL] += 1;
		}
		break;
	case PT_TESC:
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
		break;
	case PT_IRON:
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						int rt = parts[ri].type;
						if (rt == PT_DSTW || rt == PT_SLTW || rt == PT_WATR)
						{
							int rnd = rand()%100;
							if (!rnd)
								part_change_type(ri,x+rx,y+ry,PT_O2);
							else if (3>rnd)
								part_change_type(ri,x+rx,y+ry,PT_H2);
						}
					}
				}
		break;
	case PT_TUNG:
		if(parts[i].temp < 3595.0f){
			parts[i].temp += (rand()%20)-4;
		}
		break;
	default:
		break;
	}
	int receiver, sender = ct;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				if (sim->pmap[y+ry][x+rx].count_notEnergy<=0) continue;
				bool spark_blocked = sim->is_spark_blocked(x,y,x+rx,y+ry); // is spark blocked by insl
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					receiver = parts[ri].type;
					//receiver is the element SPRK is trying to conduct to
					//sender is the element the SPRK is on
					//First, some checks usually for (de)activation of elements
					switch (receiver)
					{
					case PT_SWCH:
						if (!spark_blocked && parts[i].life<4)
						{
							if (sender==PT_PSCN && parts[ri].life<10) {
								parts[ri].life = 10;
							}
							else if (sender==PT_NSCN) {
								parts[ri].ctype = PT_NONE;
								parts[ri].life = 9;
							}
						}
						break;
					case PT_SPRK:
						if (!spark_blocked && parts[i].life<4)
						{
							// make sparked SWCH turn off correctly
							if (parts[ri].ctype==PT_SWCH) 
							{
								if (sender==PT_NSCN)
								{
									part_change_type(ri,x+rx,y+ry,PT_SWCH);
									parts[ri].ctype = PT_NONE;
									parts[ri].life = 9;
								}
							}
							else if (parts[ri].ctype==PT_NTCT||parts[ri].ctype==PT_PTCT||parts[ri].ctype==PT_INWR)
							{
								if (sender==PT_METL)
									parts[ri].temp = 473.0f;
							}
						}
						continue;//no conduction, destination already sparked
					case PT_PUMP:
					case PT_GPMP:
					case PT_HSWC:
					case PT_PBCN:
						// PROP_PTOGGLE, Maybe? We seem to use 2 different methods for handling actived elements, this one seems better. Yes, use this one for new elements, PCLN is different for compatibility with existing saves
						if (parts[i].life<4) 
						{
							if (sender==PT_PSCN) parts[ri].life = 10;
							else if (sender==PT_NSCN && parts[ri].life>=10) parts[ri].life = 9;
						}
						continue;//no conduction
					case PT_LCRY:
						if (abs(rx)<2 && abs(ry)<2 && parts[i].life<4)
						{
							if (sender==PT_PSCN && parts[ri].tmp == 0) parts[ri].tmp = 2;
							else if (sender==PT_NSCN && parts[ri].tmp == 3) parts[ri].tmp = 1;
						}
						continue;//no conduction
					case PT_PPIP:
						if (parts[i].life == 3 && !spark_blocked)
						{
							if (sender == PT_NSCN || sender == PT_PSCN || sender == PT_INST)
								PPIP_flood_trigger(sim, x+rx, y+ry, sender);
						}
						continue;//no conduction
					case PT_NTCT:
					case PT_PTCT:
					case PT_INWR:
						if (sender==PT_METL && !spark_blocked && parts[i].life<4)
							parts[ri].temp = 473.0f;
						break;
					default:
						break;
					}

					if (spark_blocked)
						continue;//Insulation blocks everything past here
					if (!((ptypes[receiver].properties&PROP_CONDUCTS)||receiver==PT_INST||receiver==PT_QRTZ))
						continue; //Stop non-conducting recievers, allow INST and QRTZ as special cases
					if (abs(rx)+abs(ry)>=4 &&sender!=PT_SWCH&&receiver!=PT_SWCH)
						continue; //Only switch conducts really far
					if (receiver==sender && receiver!=PT_INST && receiver!=PT_QRTZ)
						goto conduct; //Everything conducts to itself, except INST and QRTZ (when temperature/pressure conditions are wrong).

					//Sender cases, where elements can have specific outputs
					switch (sender)
					{
					case PT_INST:
						if (receiver==PT_NSCN)
							goto conduct;
						continue;
					case PT_SWCH:
						if (receiver==PT_PSCN||receiver==PT_NSCN||receiver==PT_WATR||receiver==PT_SLTW||receiver==PT_NTCT||receiver==PT_PTCT||receiver==PT_INWR)
							continue;
						break;
					case PT_ETRD:
						if (receiver==PT_METL||receiver==PT_BMTL||receiver==PT_BRMT||receiver==PT_LRBD||receiver==PT_RBDM||receiver==PT_PSCN||receiver==PT_NSCN)
							goto conduct;
						continue;
					case PT_NTCT:
						if (receiver==PT_PSCN || (receiver==PT_NSCN && parts[i].temp>373.0f))
							goto conduct;
						continue;
					case PT_PTCT:
						if (receiver==PT_PSCN || (receiver==PT_NSCN && parts[i].temp<373.0f))
							goto conduct;
						continue;
					case PT_INWR:
						if (receiver==PT_NSCN || receiver==PT_PSCN)
							goto conduct;
						continue;
					default:
						break;
					}
					//Receiving cases, where elements can have specific inputs
					switch (receiver)
					{
					case PT_QRTZ:
						if ((sender==PT_NSCN||sender==PT_METL||sender==PT_PSCN||sender==PT_QRTZ) && (parts[ri].temp<173.15||pv[(y+ry)/CELL][(x+rx)/CELL]>8))
							goto conduct;
						continue;
					case PT_NTCT:
						if (sender==PT_NSCN || (sender==PT_PSCN&&parts[ri].temp>373.0f))
							goto conduct;
						continue;
					case PT_PTCT:
						if (sender==PT_NSCN || (sender==PT_PSCN&&parts[ri].temp<373.0f))
							goto conduct;
						continue;
					case PT_INWR:
						if (sender==PT_NSCN || sender==PT_PSCN)
							goto conduct;
						continue;
					case PT_INST:
						if (sender==PT_PSCN)
							goto conduct;
						continue;
					case PT_NBLE:
						if (!(parts[ri].temp > 5273.15))
							goto conduct;
						continue;
					case PT_PSCN:
						if (sender!=PT_NSCN)
							goto conduct;
						continue;
					default:
						break;
					}
				conduct:
					//Yay, passed normal conduction rules, check a few last things and change receiver to spark
					if (receiver==PT_WATR||receiver==PT_SLTW)
					{
						if (parts[ri].life==0 && parts[i].life<3)
						{
							sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
						}
					}
					else if (receiver==PT_INST)
					{
						if (parts[ri].life==0 && parts[i].life<4)
						{
							INST_flood_spark(sim, x+rx, y+ry);
						}
					}
					else if (parts[ri].life==0 && parts[i].life<4)
					{
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
					}
					else if (sender==PT_ETRD && parts[i].life==5) //ETRD is odd and conducts to others only at life 5, this could probably be somewhere else
					{
						part_change_type(i,x,y,sender);
						parts[i].ctype = PT_NONE;
						parts[i].life = 20;
						sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
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


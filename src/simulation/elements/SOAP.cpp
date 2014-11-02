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

void SOAP_attach(Simulation *sim, int i1, int i2)
{
	if (!(sim->parts[i2].ctype&4))
	{
		sim->parts[i1].ctype |= 2;
		sim->parts[i1].tmp = i2;

		sim->parts[i2].ctype |= 4;
		sim->parts[i2].tmp2 = i1;
	}
	else
	if (!(sim->parts[i2].ctype&2))
	{
		sim->parts[i1].ctype |= 4;
		sim->parts[i1].tmp2= i2;

		sim->parts[i2].ctype |= 2;
		sim->parts[i2].tmp = i1;
	}
}

void SOAP_detach(Simulation *sim, int i)
{
	if ((sim->parts[i].ctype&2) == 2)
	{
		if ((sim->parts[sim->parts[i].tmp].ctype&4) == 4)
			sim->parts[sim->parts[i].tmp].ctype ^= 4;
	}

	if ((sim->parts[i].ctype&4) == 4)
	{
		if ((sim->parts[sim->parts[i].tmp2].ctype&2) == 2)
			sim->parts[sim->parts[i].tmp2].ctype ^= 2;
	}

	sim->parts[i].ctype = 0;
}

int SOAP_update(UPDATE_FUNC_ARGS) 
{
	int rx, ry, rt, nr, ng, nb, na;
	int rcount, ri, rnext;
	float tr, tg, tb, ta;
	float blend;
	
	//0x01 - bubble on/off
	//0x02 - first mate yes/no
	//0x04 - "back" mate yes/no

	if (parts[i].ctype&1)
	{
		if (parts[i].temp>0)
		{
			if (parts[i].life<=0)
			{
				if ((parts[i].ctype&6) != 6 && (parts[i].ctype&6))
				{
					int target;

					target = i;

					while((parts[target].ctype&6) != 6 && (parts[target].ctype&6))
					{
						if (parts[target].ctype&2)
						{
							target = parts[target].tmp;
							SOAP_detach(sim, target);
						}

						if (parts[target].ctype&4)
						{
							target = parts[target].tmp2;
							SOAP_detach(sim, target);
						}
					}
				}

				if ((parts[i].ctype&6) != 6)
					parts[i].ctype = 0;

				if ((parts[i].ctype&6) == 6 && (parts[parts[i].tmp].ctype&6) == 6 && parts[parts[i].tmp].tmp == i)
					SOAP_detach(sim, i);
			}

			parts[i].vy -= 0.1f;

			parts[i].vy *= 0.5f;
			parts[i].vx *= 0.5f;
		}

		if (!(parts[i].ctype&2))
		{
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
						{
							if ((parts[ri].type == PT_SOAP) && (parts[ri].ctype&1) && !(parts[ri].ctype&4))
								SOAP_attach(sim, i, ri);
						}
					}
		}
		else
		{
			if (parts[i].life<=0)
				for (rx=-2; rx<3; rx++)
					for (ry=-2; ry<3; ry++)
						if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
						{
							if (parts[i].temp>0 && bmap[(y+ry)/CELL][(x+rx)/CELL])
							{
								SOAP_detach(sim, i);
								continue;
							}

							FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
							{
								rt = parts[ri].type;

								if (parts[i].temp>0)
								{
									if (ptypes[rt].state != ST_GAS && rt != PT_SOAP && rt != PT_GLAS)
									{
										SOAP_detach(sim, i);
										continue;
									}
								}

								if (rt == PT_SOAP && parts[ri].ctype == 1)
								{
									int buf;

									buf = parts[i].tmp;

									parts[i].tmp = ri;
									parts[buf].tmp2 = ri;
									parts[ri].tmp2 = i;
									parts[ri].tmp = buf;
									parts[ri].ctype = 7;
								}

								if (rt == PT_SOAP && parts[ri].ctype == 7 && parts[i].tmp != ri && parts[i].tmp2 != ri)
								{
									parts[parts[i].tmp].tmp2 = parts[ri].tmp2;
									parts[parts[ri].tmp2].tmp = parts[i].tmp;
									parts[ri].tmp2 = i;
									parts[i].tmp = ri;
								}
							}
						}
		}

		if(parts[i].ctype&2)
		{
			float d, dx, dy;

			dx = parts[i].x - parts[parts[i].tmp].x;
			dy = parts[i].y - parts[parts[i].tmp].y;

			d = 9/(pow(dx, 2)+pow(dy, 2)+9)-0.5;

			parts[parts[i].tmp].vx -= dx*d;
			parts[parts[i].tmp].vy -= dy*d;

			parts[i].vx += dx*d;
			parts[i].vy += dy*d;

			if ((parts[parts[i].tmp].ctype&2) && (parts[parts[i].tmp].ctype&1) 
					&& (parts[parts[parts[i].tmp].tmp].ctype&2) && (parts[parts[parts[i].tmp].tmp].ctype&1))
			{
				int ii;

				ii = parts[parts[parts[i].tmp].tmp].tmp;

				dx = parts[ii].x - parts[parts[i].tmp].x;
				dy = parts[ii].y - parts[parts[i].tmp].y;

				d = 81/(pow(dx, 2)+pow(dy, 2)+81)-0.5;

				parts[parts[i].tmp].vx -= dx*d*0.5f;
				parts[parts[i].tmp].vy -= dy*d*0.5f;

				parts[ii].vx += dx*d*0.5f;
				parts[ii].vy += dy*d*0.5f;
			}
		}
	}
	else
	{
		if (pv[y/CELL][x/CELL]>0.5f || pv[y/CELL][x/CELL]<(-0.5f))
		{
			parts[i].ctype = 1;
			parts[i].life = 10;
		}

		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type == PT_OIL)
						{
							float ax, ay;

							parts[i].vy -= 0.1f;

							parts[i].vy *= 0.5f;
							parts[i].vx *= 0.5f;

							ax = (parts[i].vx + parts[ri].vx)/2;
							ay = (parts[i].vy + parts[ri].vy)/2;

							parts[i].vx = ax;
							parts[i].vy = ay;
							parts[ri].vx = ax;
							parts[ri].vy = ay;
						}
					}
				}
	}
	
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (rt!=PT_SOAP)
					{
						blend = 0.85f;
						tr = (parts[ri].dcolour>>16)&0xFF;
						tg = (parts[ri].dcolour>>8)&0xFF;
						tb = (parts[ri].dcolour)&0xFF;
						ta = (parts[ri].dcolour>>24)&0xFF;
						
						nr = (tr*blend);
						ng = (tg*blend);
						nb = (tb*blend);
						na = (ta*blend);
						
						parts[ri].dcolour = nr<<16 | ng<<8 | nb | na<<24;
					}
				}
			}

	return 0;
}

void SOAP_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (from==PT_SOAP && to!=PT_SOAP)
	{
		SOAP_detach(sim, i);
	}
}

void SOAP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SOAP";
	elem->Name = "SOAP";
	elem->Colour = COLPACK(0xF5F5DC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Soap. Creates bubbles.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_NEUTPENETRATE|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITL;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = -1;
	elem->DefaultProperties.tmp2 = -1;

	elem->Update = &SOAP_update;
	elem->Graphics = NULL;
	elem->Func_ChangeType = &SOAP_ChangeType;
}


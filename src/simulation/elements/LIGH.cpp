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
#include "simulation/elements/STKM.h"
#include "simulation/elements-shared/pyro.h"

#define LIGHTING_POWER 0.65

int LIGH_nearest_part(Simulation *sim, int ci, int max_d)
{
	particle *parts = sim->parts;
	int distance = (max_d!=-1)?max_d:MAX_DISTANCE;
	int ndistance = 0;
	int id = -1;
	int i = 0;
	int cx = (int)parts[ci].x;
	int cy = (int)parts[ci].y;
	for (i=0; i<=sim->parts_lastActiveIndex; i++)
	{
		if (parts[i].type && !parts[i].life && i!=ci && parts[i].type!=PT_LIGH && parts[i].type!=PT_THDR && parts[i].type!=PT_NEUT && parts[i].type!=PT_PHOT)
		{
			ndistance = abs(cx-parts[i].x)+abs(cy-parts[i].y);// Faster but less accurate  Older: sqrt(pow(cx-parts[i].x, 2)+pow(cy-parts[i].y, 2));
			if (ndistance<distance)
			{
				distance = ndistance;
				id = i;
			}
		}
	}
	return id;
}

int contact_part(Simulation* sim, int i, int tp)
{
	int x=parts[i].x, y=parts[i].y;
	int r,rx,ry;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = sim->pmap_find_one(x+rx, y+ry, tp);// TODO: not energy parts?
				if (r>=0) return r;
			}
	return -1;
}

// Create LIGH particle, or return true if it was eaten by void / black hole
bool create_LIGH_line_part(Simulation * sim, int x, int y, int temp, int life, int tmp, int tmp2, bool last)
{
	int p = sim->part_create(-1, x, y, PT_LIGH);
	if (p != -1)
	{
		sim->parts[p].life = life;
		sim->parts[p].temp = temp;
		sim->parts[p].tmp = tmp;
		if (last)
		{
			sim->parts[p].tmp2=1+(rand()%200>tmp2*tmp2/10+60);
			sim->parts[p].life=(int)(life/1.5-rand()%2);
		}
		else
		{
			sim->parts[p].life = life;
			sim->parts[p].tmp2 = 0;
		}
	}
	else if (x >= 0 && x < XRES && y >= 0 && y < YRES)
	{
		int ri, rcount, rnext;
		FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
		{
			int rt = sim->parts[ri].type;
			// Eaten by void if not excluded due to void ctype+tmp setting
			if ((rt==PT_VOID || (rt==PT_PVOD && sim->parts[ri].life >= 10)) && (!sim->parts[ri].ctype || (sim->parts[ri].ctype==PT_LIGH)!=(sim->parts[ri].tmp&1)))
				return true;
			// Eaten by black hole
			if (rt==PT_BHOL || rt==PT_NBHL)
				return true;
		}
	}
	else return true;
	return false;
}


void create_LIGH_line(Simulation *sim, int x1, int y1, int x2, int y2, int temp, int life, int tmp, int tmp2)
{
	bool reverseXY = abs(y2-y1) > abs(x2-x1), back = false;
	int x, y, dx, dy, Ystep;
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
		back = 1;
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	Ystep = (y1<y2) ? 1 : -1;
	if (!back)
	{
		for (x=x1; x<=x2; x++)
		{
			bool wasEaten;
			if (reverseXY)
				wasEaten = create_LIGH_line_part(sim, y, x, temp, life, tmp, tmp2, x==x2);
			else
				wasEaten = create_LIGH_line_part(sim, x, y, temp, life, tmp, tmp2, x==x2);
			if (wasEaten)
				return;

			e += de;
			if (e>=0.5f)
			{
				y += Ystep;
				e -= 1.0f;
			}
		}
	}
	else
	{
		for (x=x1; x>=x2; x--)
		{
			bool wasEaten;
			if (reverseXY)
				wasEaten = create_LIGH_line_part(sim, y, x, temp, life, tmp, tmp2, x==x2);
			else
				wasEaten = create_LIGH_line_part(sim, x, y, temp, life, tmp, tmp2, x==x2);
			if (wasEaten)
				return;

			e += de;
			if (e<=-0.5f)
			{
				y += Ystep;
				e += 1.0f;
			}
		}
	}
}

int LIGH_update(UPDATE_FUNC_ARGS)
{
	/*
	 *
	 * tmp2:
	 * -1 - part will be removed
	 * 0 - "branches" of the lightning
	 * 1 - bending
	 * 2 - branching
	 * 3 - transfer spark or make destruction
	 * 4 - first pixel
	 *
	 * life - "thickness" of lighting (but anyway one pixel)
	 *
	 * tmp - angle of lighting, measured in degrees anticlockwise from the positive x direction
	 *
	*/
	int rx, ry, rt, multipler, powderful;
	int rcount, ri, rnext;
	int angle, angle2=-1;
	int near;
	powderful = parts[i].temp*(1+parts[i].life/40)*LIGHTING_POWER;
	if (aheat_enable)
	{
		hv[y/CELL][x/CELL]+=powderful/50;
		if (hv[y/CELL][x/CELL]>MAX_TEMP)
			hv[y/CELL][x/CELL]=MAX_TEMP;
	}

	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					int ret = ElementsShared_pyro::update_neighbour(UPDATE_FUNC_SUBCALL_ARGS, rx,ry, parts[ri].type, ri);
					if (ret==1) return 1;
					if (ret==2) continue;

					rt = parts[ri].type;
					if (rt==PT_LIGH || rt==PT_TESC)
						continue;
					if (rt==PT_CLNE || rt==PT_THDR || rt==PT_DMND || rt==PT_FIRE)
					{
						parts[ri].temp = restrict_flt(parts[ri].temp+powderful/10, MIN_TEMP, MAX_TEMP);
					}
					else
					{
						if ((ptypes[rt].properties&PROP_CONDUCTS) && parts[ri].life==0)
						{
							sim->spark_particle_conductiveOnly(ri, x+rx, y+ry);
						}
						pv[y/CELL][x/CELL] += powderful/400;
						if (ptypes[rt].hconduct)
							parts[ri].temp = restrict_flt(parts[ri].temp+powderful/1.3, MIN_TEMP, MAX_TEMP);
					}
					if (rt==PT_DEUT || rt==PT_PLUT) // start nuclear reactions
					{
						parts[ri].temp = restrict_flt(parts[ri].temp+powderful, MIN_TEMP, MAX_TEMP);
						pv[y/CELL][x/CELL] +=powderful/35;
						if (!(rand()%3))
						{
							part_change_type(ri,x+rx,y+ry,PT_NEUT);
							parts[ri].life = rand()%480+480;
							parts[ri].vx=rand()%10-5;
							parts[ri].vy=rand()%10-5;
						}
					}
					if (rt==PT_COAL || rt==PT_BCOL) // ignite coal
					{
						if (parts[ri].life>=100) {
							parts[ri].life = 99;
						}
					}
					if ((rt==PT_STKM && Stickman_data::get(sim, PT_STKM)->elem!=PT_LIGH) || (rt==PT_STKM2 && Stickman_data::get(sim, PT_STKM2)->elem!=PT_LIGH))
					{
						parts[ri].life-=powderful/100;
					}
				}
			}
	if (parts[i].tmp2==3)
	{
		parts[i].tmp2=0;
		return 1;
	}
	else if (parts[i].tmp2<=-1)
	{
		kill_part(i);
		return 1;
	}
	else if (parts[i].tmp2<=0 || parts[i].life<=1)
	{
		if (parts[i].tmp2>0)
			parts[i].tmp2=0;
		parts[i].tmp2--;
		return 1;
	}

	angle2=-1;

	near = LIGH_nearest_part(sim, i, parts[i].life*2.5);
	if (near!=-1)
	{
		int t=parts[near].type;
		float n_angle; // angle to nearest part
		float angle_diff;
		rx=parts[near].x-x;
		ry=parts[near].y-y;
		if (rx!=0 || ry!=0)
			n_angle = atan2f(-ry, rx);
		else
			n_angle = 0;
		if (n_angle<0)
			n_angle+=M_PI*2;
		angle_diff = fabsf(n_angle-parts[i].tmp*M_PI/180);
		if (angle_diff>M_PI)
			angle_diff = M_PI*2 - angle_diff;
		if (parts[i].life<5 || angle_diff<M_PI*0.8) // lightning strike
		{
			create_LIGH_line(sim, x, y, x+rx, y+ry, parts[i].temp, parts[i].life, parts[i].tmp-90, 0);

			if (t!=PT_TESC)
			{
				near=contact_part(sim, near, PT_LIGH);
				if (near!=-1)
				{
					parts[near].tmp2=3;
					parts[near].life=(int)(1.0*parts[i].life/2-1);
					parts[near].tmp=parts[i].tmp-180;
					parts[near].temp=parts[i].temp;
				}
			}
		}
		else near=-1;
	}

	//if (parts[i].tmp2==1/* || near!=-1*/)
	//angle=0;//parts[i].tmp-30+rand()%60;
	angle = (parts[i].tmp+330+rand()%60)%360;
	if (parts[i].tmp2==2 && near==-1)
	{
		angle2=(angle+460-rand()%200)%360;
	}

	multipler=parts[i].life*1.5+rand()%((int)(parts[i].life+1));
	rx=cos(angle*M_PI/180)*multipler;
	ry=-sin(angle*M_PI/180)*multipler;
	create_LIGH_line(sim, x, y, x+rx, y+ry, parts[i].temp, parts[i].life, angle, parts[i].tmp2);

	if (angle2!=-1)
	{
		multipler=parts[i].life*1.5+rand()%((int)(parts[i].life+1));
		rx=cos(angle2*M_PI/180)*multipler;
		ry=-sin(angle2*M_PI/180)*multipler;
		create_LIGH_line(sim, x, y, x+rx, y+ry, parts[i].temp, parts[i].life, angle2, parts[i].tmp2);
	}

	parts[i].tmp2=-1;
	return 1;
}

int LIGH_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 120;
	*firer = *colr = 235;
	*fireg = *colg = 245;
	*fireb = *colb = 255;
	*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	return 1;
}

void LIGH_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float gx, gy, gsize;
	sim->parts[i].life = 30;
	sim->parts[i].temp = sim->parts[i].life*150.0f; // temperature of the lightning shows the power of the lightning
	sim->GetGravityAccel(x,y, 1.0f, 1.0f, gx,gy);
	gsize = gx*gx+gy*gy;
	if (gsize<0.0016f)
	{
		float angle = (rand()%6284)*0.001f;//(in radians, between 0 and 2*pi)
		gsize = sqrtf(gsize);
		// randomness in weak gravity fields (more randomness with weaker fields)
		gx += cosf(angle)*(0.04f-gsize);
		gy += sinf(angle)*(0.04f-gsize);
	}
	sim->parts[i].tmp = (((int)(atan2f(-gy, gx)*(180.0f/M_PI)))+rand()%40-20+360)%360;
	sim->parts[i].tmp2 = 4;
}

void LIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_LIGH";
	elem->ui->Name = "LIGH";
	elem->Colour = COLPACK(0xFFFFC0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_EXPLOSIVE;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Lightning. Change the brush size to set the size of the lightning.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &LIGH_update;
	elem->Graphics = &LIGH_graphics;
	elem->Func_Create = &LIGH_create;
}


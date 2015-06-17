/**
 * Powder Toy - particle simulation
 *
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

#include <stdint.h>
#include <math.h>
#include <defines.h>
#include <powder.h>
#include <misc.h>
#include "gravity.h"
#ifdef LUACONSOLE
#include <luaconsole.h>
#endif

#include "common/tptmath.h"
#include "simulation/Simulation.h"
#include "simulation/CoordStack.h"
#include "simulation/ElemDataSim.h"
#include "simulation/elements/PHOT.h"

#include "algorithm/for_each_i.hpp"

int lighting_recreate = 0;
int force_stacking_check = 0;//whether to force a check for excessively stacked particles

particle *parts;
particle *cb_parts;
const particle emptyparticle = {};

int water_equal_test = 0;

unsigned char bmap[YRES/CELL][XRES/CELL];
unsigned char emap[YRES/CELL][XRES/CELL];

unsigned char cb_bmap[YRES/CELL][XRES/CELL];
unsigned char cb_emap[YRES/CELL][XRES/CELL];

int NUM_PARTS = 0;

int GRAV;
int GRAV_R;
int GRAV_G;
int GRAV_B;
int GRAV_R2;
int GRAV_G2;
int GRAV_B2;
int GSPEED = 1;
int GENERATION = 0;
int CGOL = 0;

unsigned char gol[YRES][XRES];
unsigned short gol2[YRES][XRES][9];

wall_type wtypes[] =
{
	{PIXPACK(0xC0C0C0), PIXPACK(0x101010), 0, "Blocks everything. Conductive."},
	{PIXPACK(0x808080), PIXPACK(0x808080), 0, "E-Wall. Becomes transparent when electricity is connected."},
	{PIXPACK(0xFF8080), PIXPACK(0xFF2008), 1, "Detector. Generates electricity when a particle is inside."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Streamline. Set start point of a streamline."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Sign. Displays text. Click on a sign to edit it or anywhere else to place a new one."},
	{PIXPACK(0x8080FF), PIXPACK(0x000000), 1, "Fan. Accelerates air. Use the line tool to set direction and strength."},
	{PIXPACK(0xC0C0C0), PIXPACK(0x101010), 2,  "Allows liquids, blocks all other particles. Conductive."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 1, "Absorbs particles but lets air currents through."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Erases walls."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 3, "Basic wall, blocks everything."},
	{PIXPACK(0x3C3C3C), PIXPACK(0x000000), 1, "Allows air, but blocks all particles."},
	{PIXPACK(0x575757), PIXPACK(0x000000), 1, "Allows powders, blocks all other particles."},
	{PIXPACK(0xFFFF22), PIXPACK(0x101010), 2, "Conductor. Allows all particles to pass through and conducts electricity."},
	{PIXPACK(0x242424), PIXPACK(0x101010), 0, "E-Hole. absorbs particles, releases them when powered."},
	{PIXPACK(0xFFFFFF), PIXPACK(0x000000), -1, "Air, creates airflow and pressure"},
	{PIXPACK(0xFFBB00), PIXPACK(0x000000), -1, "Heats the targeted element."},
	{PIXPACK(0x00BBFF), PIXPACK(0x000000), -1, "Cools the targeted element."},
	{PIXPACK(0x303030), PIXPACK(0x000000), -1, "Vacuum, reduces air pressure."},
	{PIXPACK(0x579777), PIXPACK(0x000000), 1, "Allows gases, blocks all other particles."},
	{PIXPACK(0x000000), PIXPACK(0x000000), -1, "Creates air movement."},
	{PIXPACK(0xFFEE00), PIXPACK(0xAA9900), 4, "Gravity wall. Newtonian Gravity has no effect inside a box drawn with this."},
	{PIXPACK(0x0000BB), PIXPACK(0x000000), -1, "Positive gravity tool."},
	{PIXPACK(0x000099), PIXPACK(0x000000), -1, "Negative gravity tool."},
	{PIXPACK(0xFFAA00), PIXPACK(0xAA5500), 4, "Allows energy particles, blocks all other particles."},
	{PIXPACK(0xFFAA00), PIXPACK(0xAA5500), -1, "Property edit tool"},
	{PIXPACK(0xDCDCDC), PIXPACK(0x000000), 1, "Allows all particles, but blocks air."},
};

static unsigned direction_to_map(float dx, float dy, int t)
{
	// TODO:
	// Adding extra directions causes some inaccuracies.
	// Not adding them causes problems with some diagonal surfaces (photons absorbed instead of reflected).
	// For now, don't add them.
	// Solution may involve more intelligent setting of initial i0 value in find_next_boundary?
	// or rewriting normal/boundary finding code

	return (dx >= 0) |
	       (((dx + dy) >= 0) << 1) |     /*  567  */
	       ((dy >= 0) << 2) |            /*  4+0  */
	       (((dy - dx) >= 0) << 3) |     /*  321  */
	       ((dx <= 0) << 4) |
	       (((dx + dy) <= 0) << 5) |
	       ((dy <= 0) << 6) |
	       (((dy - dx) <= 0) << 7);
	/*
	return (dx >= -0.001) |
	       (((dx + dy) >= -0.001) << 1) |     //  567
	       ((dy >= -0.001) << 2) |            //  4+0
	       (((dy - dx) >= -0.001) << 3) |     //  321
	       ((dx <= 0.001) << 4) |
	       (((dx + dy) <= 0.001) << 5) |
	       ((dy <= 0.001) << 6) |
	       (((dy - dx) <= 0.001) << 7);
	}*/
}

static int is_blocking(int t, int x, int y)
{
	if (t & REFRACT) {
		if (x<0 || y<0 || x>=XRES || y>=YRES)
			return 0;
		if (globalSim->pmap_find_one(x, y, PT_GLAS)>=0)
			return 1;
		return 0;
	}

	return !globalSim->part_canMove(t, x, y);
}

static int is_boundary(int pt, int x, int y)
{
	if (!is_blocking(pt,x,y))
		return 0;
	if (is_blocking(pt,x,y-1) && is_blocking(pt,x,y+1) && is_blocking(pt,x-1,y) && is_blocking(pt,x+1,y))
		return 0;
	return 1;
}

static int find_next_boundary(int pt, int *x, int *y, int dm, int *em)
{
	static int dx[8] = {1,1,0,-1,-1,-1,0,1};
	static int dy[8] = {0,1,1,1,0,-1,-1,-1};
	static int de[8] = {0x83,0x07,0x0E,0x1C,0x38,0x70,0xE0,0xC1};
	int i, ii, i0;

	if (*x <= 0 || *x >= XRES-1 || *y <= 0 || *y >= YRES-1)
		return 0;

	if (*em != -1) {
		i0 = *em;
		dm &= de[i0];
	} else
		i0 = 0;

	for (ii=0; ii<8; ii++) {
		i = (ii + i0) & 7;
		if ((dm & (1 << i)) && is_boundary(pt, *x+dx[i], *y+dy[i])) {
			*x += dx[i];
			*y += dy[i];
			*em = i;
			return 1;
		}
	}

	return 0;
}

int get_normal(int pt, int x, int y, float dx, float dy, float *nx, float *ny)
{
	int ldm, rdm, lm, rm;
	int lx, ly, lv, rx, ry, rv;
	int i, j;
	float r, ex, ey;

	if (!dx && !dy)
		return 0;

	if (!is_boundary(pt, x, y))
		return 0;

	ldm = direction_to_map(-dy, dx, pt);
	rdm = direction_to_map(dy, -dx, pt);
	lx = rx = x;
	ly = ry = y;
	lv = rv = 1;
	lm = rm = -1;

	j = 0;
	for (i=0; i<SURF_RANGE; i++) {
		if (lv)
			lv = find_next_boundary(pt, &lx, &ly, ldm, &lm);
		if (rv)
			rv = find_next_boundary(pt, &rx, &ry, rdm, &rm);
		j += lv + rv;
		if (!lv && !rv)
			break;
	}

	if (j < NORMAL_MIN_EST)
		return 0;

	if ((lx == rx) && (ly == ry))
		return 0;

	ex = rx - lx;
	ey = ry - ly;
	r = 1.0f/hypot(ex, ey);
	*nx =  ey * r;
	*ny = -ex * r;

	return 1;
}

int get_normal_interp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny)
{
	int x, y, i;

	dx /= NORMAL_FRAC;
	dy /= NORMAL_FRAC;

	for (i=0; i<NORMAL_INTERP; i++) {
		x = (int)(x0 + 0.5f);
		y = (int)(y0 + 0.5f);
		if (is_boundary(pt, x, y))
			break;
		x0 += dx;
		y0 += dy;
	}
	if (i >= NORMAL_INTERP)
		return 0;

	if (pt == PT_PHOT)
		Element_PHOT::photoelectric_effect(globalSim, x, y);

	return get_normal(pt, x, y, dx, dy, nx, ny);
}

int create_part(int p, int x, int y, int tv)//the function for creating a particle, use p=-1 for creating a new particle, -2 is from a brush, or a particle number to replace a particle.
{
	int rcount, ri, rnext;
	int i;

	int t = tv & 0xFF;
	int v = (tv >> 8) & 0xFF;
	
	if (x<0 || y<0 || x>=XRES || y>=YRES || (!globalSim->IsValidElement(t) && t!=SPC_HEAT&&t!=SPC_COOL&&t!=SPC_AIR&&t!=SPC_VACUUM&&t!=SPC_PGRV&&t!=SPC_NGRV))
		return -1;
	if(t==SPC_PROP) {
		return -1;	//Prop tool works on a mouse click basic, make sure it doesn't do anything here
	}

	if (t==SPC_HEAT||t==SPC_COOL)
	{
		if (!globalSim->pmap[y][x].count)
			return -1;
		FOR_PMAP_POSITION(globalSim, x, y, rcount, ri, rnext)
		{
			if (t==SPC_HEAT&&parts[ri].temp<MAX_TEMP)
			{
				float heatchange;
				int fast = ((sdl_mod & (KMOD_SHIFT)) && (sdl_mod & (KMOD_CTRL)));
				if (parts[ri].type==PT_PUMP || parts[ri].type==PT_GPMP)
					heatchange = fast?1.0f:.1f;
				else
					heatchange = fast?50.0f:4.0f;
				
				globalSim->part_add_temp(parts[ri], heatchange);
			}
			if (t==SPC_COOL&&parts[ri].temp>MIN_TEMP)
			{
				float heatchange;
				int fast = ((sdl_mod & (KMOD_SHIFT)) && (sdl_mod & (KMOD_CTRL)));
				if (parts[ri].type==PT_PUMP || parts[ri].type==PT_GPMP)
					heatchange = fast?1.0f:.1f;
				else
					heatchange = fast?50.0f:4.0f;
				
				globalSim->part_add_temp(parts[ri], -heatchange);
			}
		}
		return ri;
	}
	if (t==SPC_AIR)
	{
		globalSim->air.pv.add(SimCoordI(x,y), 0.10f);
		return -1;
	}
	if (t==SPC_VACUUM)
	{
		globalSim->air.pv.add(SimCoordI(x,y), -0.10f);
		return -1;
	}
	if (t==SPC_PGRV)
	{
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = 5;
		return -1;
	}
	if (t==SPC_NGRV)
	{
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = -5;
		return -1;
	}


	if (t==PT_SPRK)
	{
		int index, type, lastIndex=-1;
		FOR_PMAP_POSITION_NOENERGY(globalSim, x, y, rcount, index, rnext)
		{
			type = parts[index].type;
			if (p == -2 || type != PT_INST)
			{
				if (globalSim->spark_particle(index, x, y))
					lastIndex = index;
			}
			if (p==-2 && ((globalSim->elements[type].Properties & PROP_DRAWONCTYPE) || type==PT_CRAY))
			{
				parts[index].ctype = PT_SPRK;
				lastIndex = index;
			}
		}
		return lastIndex;
	}
	if (p==-2)//creating from brush
	{
		bool energyParticleFound = false;
		bool normalParticleFound = false;
		bool actionDone = false;
		FOR_PMAP_POSITION(globalSim, x, y, rcount, ri, rnext)
		{
			//If an element has the PROP_DRAWONCTYPE property, and the element being drawn to it does not have PROP_NOCTYPEDRAW (Also some special cases), set the element's ctype
			int drawOn = parts[ri].type;
			if (globalSim->elements[drawOn].Properties&TYPE_ENERGY)
				energyParticleFound = true;
			else
				normalParticleFound = true;

			if (((globalSim->elements[drawOn].Properties & PROP_DRAWONCTYPE) ||
				(drawOn==PT_STOR&&!(globalSim->elements[t].Properties&TYPE_SOLID))||
				(drawOn==PT_PCLN&&t!=PT_PSCN&&t!=PT_NSCN)||
				(drawOn==PT_PBCN&&t!=PT_PSCN&&t!=PT_NSCN)
				) && !(globalSim->elements[t].Properties & PROP_NOCTYPEDRAW)
			)
			{
				parts[ri].ctype = t;
				actionDone = true;
				if (t==PT_LIFE && v<NGOLALT && drawOn!=PT_STOR)
					parts[ri].tmp = v;
				else if (drawOn == PT_DRAY)
					parts[ri].ctype |= v<<8;
			}
			else if ((drawOn == PT_DTEC || (drawOn == PT_PSTN && t != PT_FRME) || drawOn == PT_DRAY) && drawOn != t)
			{
				parts[ri].ctype = t;
				actionDone = true;
				if (drawOn == PT_DTEC && t==PT_LIFE && v<NGOLALT)
					parts[ri].tmp = v;
			}
			else if (drawOn == PT_CRAY && drawOn != t)
			{
				parts[ri].ctype = t;
				if (t==PT_LIFE && v<NGOLALT)
					parts[ri].tmp2 = v;
				parts[ri].temp = globalSim->elements[t].DefaultProperties.temp;
			}
		}
		if (actionDone)
			return -1;
		if (globalSim->elements[t].Properties & TYPE_ENERGY)
		{
			if (energyParticleFound)
				return -1;
		}
		else
		{
			if (normalParticleFound)
				return -1;
		}
	}
	i = globalSim->part_create(p, x, y, t);
	if (t==PT_LIFE && i>=0)
	{
		if (v<NGOLALT)
		{
			parts[i].tmp = grule[v+1][9] - 1;
			parts[i].ctype = v;
		}
	}
	return i;
}

void delete_part(int x, int y, int flags)//calls part_kill with the particle located at x,y
{
	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return;

	if (!globalSim->pmap[y][x].count)
		return;

	if (!(flags&BRUSH_SPECIFIC_DELETE))
	{
		globalSim->part_kill(globalSim->pmap[y][x].first);
	}
	else
	{
		int count, i, next;
		FOR_PMAP_POSITION(globalSim, x, y, count, i, next)
		{
			if (parts[i].type==SLALT || SLALT==0)//specific deletion
			{
				globalSim->part_kill(i);
			}
			else if (globalSim->elements[parts[i].type].ui->MenuSection==SEC)//specific menu deletion
			{
				globalSim->part_kill(i);
			}
		}
	}
}

TPT_INLINE int is_wire(int x, int y)
{
	return bmap[y][x]==WL_DETECT || bmap[y][x]==WL_EWALL || bmap[y][x]==WL_ALLOWLIQUID || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_ALLOWALLELEC || bmap[y][x]==WL_EHOLE;
}

TPT_INLINE int is_wire_off(int x, int y)
{
	return (bmap[y][x]==WL_DETECT || bmap[y][x]==WL_EWALL || bmap[y][x]==WL_ALLOWLIQUID || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_ALLOWALLELEC || bmap[y][x]==WL_EHOLE) && emap[y][x]<8;
}

void set_emap(int x, int y)
{
	int x1, x2;

	if (!is_wire_off(x, y))
		return;

	// go left as far as possible
	x1 = x2 = x;
	while (x1>0)
	{
		if (!is_wire_off(x1-1, y))
			break;
		x1--;
	}
	while (x2<XRES/CELL-1)
	{
		if (!is_wire_off(x2+1, y))
			break;
		x2++;
	}

	// fill span
	for (x=x1; x<=x2; x++)
		emap[y][x] = 16;

	// fill children

	if (y>1 && x1==x2 &&
	        is_wire(x1-1, y-1) && is_wire(x1, y-1) && is_wire(x1+1, y-1) &&
	        !is_wire(x1-1, y-2) && is_wire(x1, y-2) && !is_wire(x1+1, y-2))
		set_emap(x1, y-2);
	else if (y>0)
		for (x=x1; x<=x2; x++)
			if (is_wire_off(x, y-1))
			{
				if (x==x1 || x==x2 || y>=YRES/CELL-1 ||
				        is_wire(x-1, y-1) || is_wire(x+1, y-1) ||
				        is_wire(x-1, y+1) || !is_wire(x, y+1) || is_wire(x+1, y+1))
					set_emap(x, y-1);
			}

	if (y<YRES/CELL-2 && x1==x2 &&
	        is_wire(x1-1, y+1) && is_wire(x1, y+1) && is_wire(x1+1, y+1) &&
	        !is_wire(x1-1, y+2) && is_wire(x1, y+2) && !is_wire(x1+1, y+2))
		set_emap(x1, y+2);
	else if (y<YRES/CELL-1)
		for (x=x1; x<=x2; x++)
			if (is_wire_off(x, y+1))
			{
				if (x==x1 || x==x2 || y<0 ||
				        is_wire(x-1, y+1) || is_wire(x+1, y+1) ||
				        is_wire(x-1, y-1) || !is_wire(x, y-1) || is_wire(x+1, y-1))
					set_emap(x, y+1);
			}
}

int nearest_part(int ci, int t, int max_d)
{
	int distance = (max_d!=-1)?max_d:MAX_DISTANCE;
	int ndistance = 0;
	int id = -1;
	int i = 0;
	int cx = (int)parts[ci].x;
	int cy = (int)parts[ci].y;
	for (i=0; i<=globalSim->parts_lastActiveIndex; i++)
	{
		if ((parts[i].type==t||(t==-1&&parts[i].type))&&!parts[i].life&&i!=ci)
		{
			ndistance = std::abs(cx-parts[i].x)+std::abs(cy-parts[i].y);// Faster but less accurate  Older: sqrt(pow(cx-parts[i].x, 2)+pow(cy-parts[i].y, 2));
			if (ndistance<distance)
			{
				distance = ndistance;
				id = i;
			}
		}
	}
	return id;
}

void create_arc(int sx, int sy, int dx, int dy, int midpoints, int variance, int type, int flags)
{
	int i;
	float xint, yint;
	int *xmid, *ymid;
	int voffset = variance/2;
	xmid = (int*)calloc(midpoints + 2, sizeof(int));
	ymid = (int*)calloc(midpoints + 2, sizeof(int));
	xint = (float)(dx-sx)/(float)(midpoints+1.0f);
	yint = (float)(dy-sy)/(float)(midpoints+1.0f);
	xmid[0] = sx;
	xmid[midpoints+1] = dx;
	ymid[0] = sy;
	ymid[midpoints+1] = dy;
	
	for(i = 1; i <= midpoints; i++)
	{
		ymid[i] = ymid[i-1]+yint;
		xmid[i] = xmid[i-1]+xint;
	}
	
	for(i = 0; i <= midpoints; i++)
	{
		if(i!=midpoints)
		{
			xmid[i+1] += (rand()%variance)-voffset;
			ymid[i+1] += (rand()%variance)-voffset;
		}	
		create_line(xmid[i], ymid[i], xmid[i+1], ymid[i+1], 0, 0, type, flags);
	}
	free(xmid);
	free(ymid);
}

void update_wallmaps()
{
	if (!sys_pause||framerender)
	{
		CellsData_subtract_sat(emap, emap, 1);
		globalSim->air.initBlockingData();
	}
}

void clear_area(int area_x, int area_y, int area_w, int area_h)
{
	int cx = 0;
	int cy = 0;
	int i;
	for (cy=0; cy<area_h; cy++)
	{
		for (cx=0; cx<area_w; cx++)
		{
			bmap[(cy+area_y)/CELL][(cx+area_x)/CELL] = 0;
			globalSim->delete_position(cx+area_x, cy+area_y);
		}
	}
	for (i=0; i<MAXSIGNS; i++)
	{
		if (signs[i].x>=area_x && signs[i].x<area_x+area_w && signs[i].y>=area_y && signs[i].y<area_y+area_h)
		{
			signs[i].text[0] = 0;
		}
	}
}

void create_box(int x1, int y1, int x2, int y2, int c, int flags)
{
	int i, j;
	if (c==SPC_PROP)
		return;
	if (x1>x2)
	{
		i = x2;
		x2 = x1;
		x1 = i;
	}
	if (y1>y2)
	{
		j = y2;
		y2 = y1;
		y1 = j;
	}
	for (j=y2; j>=y1; j--)
		for (i=x1; i<=x2; i++)
			create_parts(i, j, 0, 0, c, flags, 1);
}

int flood_prop(int x, int y, int parttype, size_t propoffset, void * propvalue, int proptype)
{
	Simulation *sim = globalSim;
	int x1, x2, dy = 1;
	int did_something = 0;
	int rcount, ri, rnext;
	char * bitmap = (char*)malloc(XRES*YRES); //Bitmap for checking
	if (!bitmap) return -1;
	memset(bitmap, 0, XRES*YRES);
	try
	{
		CoordStack cs;
		cs.push(x, y);
		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			x1 = x2 = x;
			while (x1>=CELL)
			{
				if (sim->pmap_find_one(x1-1, y, parttype)<0 || bitmap[(y*XRES)+x1-1])
					break;
				x1--;
			}
			while (x2<XRES-CELL)
			{
				if (sim->pmap_find_one(x2+1, y, parttype)<0 || bitmap[(y*XRES)+x2+1])
					break;
				x2++;
			}
			for (x=x1; x<=x2; x++)
			{
				FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)
				{
					if (parts[ri].type==parttype)
					{
						if(proptype==2){
							*((float*)(((char*)&parts[ri])+propoffset)) = *((float*)propvalue);
						} else if(proptype==0) {
							*((int*)(((char*)&parts[ri])+propoffset)) = *((int*)propvalue);
						} else if(proptype==1) {
							*((char*)(((char*)&parts[ri])+propoffset)) = *((char*)propvalue);
						}
						did_something = 1;
					}
				}
				bitmap[(y*XRES)+x] = 1;
			}
			if (y>=CELL+dy)
				for (x=x1; x<=x2; x++)
					if (sim->pmap_find_one(x, y-dy, parttype)>=0 && !bitmap[((y-dy)*XRES)+x])
						cs.push(x, y-dy);
			if (y<YRES-CELL-dy)
				for (x=x1; x<=x2; x++)
					if (sim->pmap_find_one(x, y+dy, parttype)>=0 && !bitmap[((y+dy)*XRES)+x])
						cs.push(x, y+dy);
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		return -1;
	}
	free(bitmap);
	return did_something;
}

int flood_parts(int x, int y, int fullc, int cm, int bm, int flags)
{
	Simulation *sim = globalSim;
	int c = fullc&0xFF;
	int x1, x2, dy = (c<PT_NUM)?1:CELL;
	int created_something = 0;

	if (c==SPC_PROP)
		return 0;
	if (cm==-1)
	{
		if (c==0)
		{
			if (sim->pmap[y][x].count==0)
				return 0;
			cm = sim->parts[sim->pmap[y][x].first].type;
			if ((flags&BRUSH_REPLACEMODE) && cm!=SLALT)
				return 0;
		}
		else
			cm = 0;
	}
	if (bm==-1)
	{
		if (c-UI_WALLSTART+UI_ACTUALSTART==WL_ERASE)
		{
			bm = bmap[y/CELL][x/CELL];
			if (!bm)
				return 0;
			if (bm==WL_WALL)
				cm = 0xFF;
		}
		else
			bm = 0;
	}

	if (((cm>0 && sim->pmap_find_one(x, y, cm)<0) || bmap[y/CELL][x/CELL]!=bm )||( (flags&BRUSH_SPECIFIC_DELETE) && cm!=SLALT))
		return 1;

	try
	{
		CoordStack cs;
		cs.push(x, y);

		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			// go left as far as possible
			while (x1>=CELL)
			{
				if ((cm==0 ? sim->pmap[y][x1-1].count>0 : sim->pmap_find_one(x1-1, y, cm)<0) || bmap[y/CELL][(x1-1)/CELL]!=bm)
					break;
				x1--;
			}
			// go right as far as possible
			while (x2<XRES-CELL)
			{
				if ((cm==0 ? sim->pmap[y][x2+1].count>0 : sim->pmap_find_one(x2+1, y, cm)<0) || bmap[y/CELL][(x2+1)/CELL]!=bm)
					break;
				x2++;
			}
			// fill span
			for (x=x1; x<=x2; x++)
			{
				if (create_parts(x, y, 0, 0, fullc, flags, 1))
					created_something = 1;
			}

			// add vertically adjacent pixels to stack
			if (y>=CELL+dy)
				for (x=x1; x<=x2; x++)
					if ((cm==0 ? sim->pmap[y-dy][x].count==0 : sim->pmap_find_one(x, y-dy, cm)>=0) && bmap[(y-dy)/CELL][x/CELL]==bm)
					{
						cs.push(x, y-dy);
					}
			if (y<YRES-CELL-dy)
				for (x=x1; x<=x2; x++)
					if ((cm==0 ? sim->pmap[y+dy][x].count==0 : sim->pmap_find_one(x, y+dy, cm)>=0) && bmap[(y+dy)/CELL][x/CELL]==bm)
					{
						cs.push(x, y+dy);
					}
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		return -1;
	}
	return created_something;
}

int flood_water(int x, int y, int i, int originaly, int check)
{
	Simulation *sim = globalSim;
	int x1, x2;
	int rcount, ri, rnext;

	try
	{
		CoordStack cs;
		cs.push(x, y);

		do
		{
			cs.pop(x, y);
			bool foundOne = false;
			FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
			{
				if (sim->elements[parts[ri].type].Falldown==2 && (parts[ri].flags & FLAG_WATEREQUAL) == check)
				{
					foundOne = true;
					break;
				}
			}
			if (!foundOne) continue;

			x1 = x2 = x;
			// go left as far as possible
			while (x1>=CELL)
			{
				bool foundOne = false;
				FOR_PMAP_POSITION_NOENERGY(sim, x1-1, y, rcount, ri, rnext)
				{
					if (sim->elements[parts[ri].type].Falldown==2)
					{
						foundOne = true;
						break;
					}
				}
				if (!foundOne) break;
				x1--;
			}
			// go right as far as possible
			while (x2<XRES-CELL)
			{
				bool foundOne = false;
				FOR_PMAP_POSITION_NOENERGY(sim, x2+1, y, rcount, ri, rnext)
				{
					if (sim->elements[parts[ri].type].Falldown==2)
					{
						foundOne = true;
						break;
					}
				}
				if (!foundOne) break;
				x2++;
			}

			// fill span
			for (x=x1; x<=x2; x++)
			{
				if (check)
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
					{
						parts[ri].flags &= ~FLAG_WATEREQUAL;//flag it as checked (different from the original particle's checked flag)
					}
				}
				else
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
					{
						parts[ri].flags |= FLAG_WATEREQUAL;//flag it as checked (different from the original particle's checked flag)
					}
				}
				//check above, maybe around other sides too?
				if ( ((y-1) > originaly) && !globalSim->pmap[y-1][x].count_notEnergy && globalSim->part_canMove(parts[i].type, x, y-1))
				{
					sim->part_set_pos(i, (int)(parts[i].x + 0.5f), (int)(parts[i].y + 0.5f), x, y-1);
					return 0;
				}
			}

			// add vertically adjacent pixels to stack
			if (y>=CELL+1)
			{
				bool foundPrev = false;
				for (x=x1; x<=x2; x++)
				{
					bool foundOne = false;
					FOR_PMAP_POSITION_NOENERGY(sim, x, y-1, rcount, ri, rnext)
					{
						if (sim->elements[parts[ri].type].Falldown==2 && (parts[ri].flags & FLAG_WATEREQUAL) == check)
						{
							foundOne = true;
							break;
						}
					}
					// If there's a long row, only add one because there will be a horizontal fill anyway
					if (foundOne && !foundPrev) cs.push(x, y-1);
					foundPrev = foundOne;
				}
			}
			if (y<YRES-CELL-1)
			{
				bool foundPrev = false;
				for (x=x1; x<=x2; x++)
				{
					bool foundOne = false;
					FOR_PMAP_POSITION_NOENERGY(sim, x, y+1, rcount, ri, rnext)
					{
						if (sim->elements[parts[ri].type].Falldown==2 && (parts[ri].flags & FLAG_WATEREQUAL) == check)
						{
							foundOne = true;
							break;
						}
					}
					// If there's a long row, only add one because there will be a horizontal fill anyway
					if (foundOne && !foundPrev) cs.push(x, y+1);
					foundPrev = foundOne;
				}
			}
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		return 1;
	}
	return 1;
}

//wrapper around create_part to create TESC with correct tmp value
int create_part_add_props(int p, int x, int y, int tv, int rx, int ry)
{
	p=create_part(p, x, y, tv);
	if (tv==PT_TESC)
	{
		parts[p].tmp=rx*4+ry*4+7;
		if (parts[p].tmp>300)
			parts[p].tmp=300;
	}
	return p;
}

//this creates particles from a brush, don't use if you want to create one particle
int create_parts(int x, int y, int rx, int ry, int c, int flags, int fill)
{
	int i, j, r, f = 0, u, v, oy, ox, b = 0, dw = 0, p, fn;

	int wall = c - 100;
	if (c==SPC_WIND || c==PT_FIGH)
		return 0;

	if(c==SPC_PROP){
		prop_edit_ui(vid_buf, x, y);
		return 0;
	}
	if (c == SPC_AIR || c == SPC_HEAT || c == SPC_COOL || c == SPC_VACUUM || c == SPC_PGRV || c == SPC_NGRV)
		fill = 1;
	for (r=UI_ACTUALSTART; r<=UI_ACTUALSTART+UI_WALLCOUNT; r++)
	{
		if (wall==r)
		{
			if (c == SPC_AIR || c == SPC_HEAT || c == SPC_COOL || c == SPC_VACUUM || c == SPC_PGRV || c == SPC_NGRV || wall == WL_SIGN)
				break;
			if (wall == WL_ERASE)
				b = 0;
			else
				b = wall;
			dw = 1;
		}
	}
	if (c == WL_FANHELPER)
	{
		b = WL_FANHELPER;
		dw = 1;
	}
	if (wall == WL_GRAV)
	{
		gravwl_timeout = 60;
	}
	if (c==PT_LIGH)
	{
	    if (lighting_recreate>0 && rx+ry>0)
            return 0;
        p=create_part(-2, x, y, c);
        if (p!=-1)
        {
            parts[p].life=rx+ry;
            if (parts[p].life>55)
                parts[p].life=55;
            parts[p].temp=parts[p].life*150; // temperature of the lighting shows the power of the lighting
            lighting_recreate+=parts[p].life/2+1;
            return 1;
        }
        else return 0;
	}
	if (c == PT_STKM || c == PT_STKM2 || c == PT_FIGH)
		rx = ry = 0;
	
	if (dw==1)
	{
		ry = ry/CELL;
		rx = rx/CELL;
		x = x/CELL;
		y = y/CELL;
		x -= rx/2;
		y -= ry/2;
		for (ox=x; ox<=x+rx; ox++)
		{
			for (oy=y; oy<=y+rx; oy++)
			{
				if (ox>=0&&ox<XRES/CELL&&oy>=0&&oy<YRES/CELL)
				{
					i = ox;
					j = oy;
					if ((flags&BRUSH_SPECIFIC_DELETE) && b!=WL_FANHELPER)
					{
						if (bmap[j][i]==SLALT-100)
						{
							b = 0;
							if (SLALT==WL_GRAV) gravwl_timeout = 60;
						}
						else
							continue;
					}
					if (b==WL_FAN)
					{
						fvx[j][i] = 0.0f;
						fvy[j][i] = 0.0f;
					}
					if (b==WL_STREAM)
					{
						i = x + rx/2;
						j = y + ry/2;
						for (v=-1; v<2; v++)
							for (u=-1; u<2; u++)
								if (i+u>=0 && i+u<XRES/CELL &&
								        j+v>=0 && j+v<YRES/CELL &&
								        bmap[j+v][i+u] == WL_STREAM)
									return 1;
						bmap[j][i] = WL_STREAM;
						continue;
					}
					if (b==0 && bmap[j][i]==WL_GRAV) gravwl_timeout = 60;
					bmap[j][i] = b;
				}
			}
		}
		return 1;
	}

	if (c == SPC_AIR || c == SPC_HEAT || c == SPC_COOL || c == SPC_VACUUM || c == SPC_PGRV || c == SPC_NGRV)
		fn = 3;
	else if (c == 0 && !(flags&BRUSH_REPLACEMODE))								// delete
		fn = 0;
	else if ((flags&BRUSH_SPECIFIC_DELETE) && !(flags&BRUSH_REPLACEMODE))	// specific delete
		fn = 1;
	else if (flags&BRUSH_REPLACEMODE)										// replace
		fn = 2;
	else																	// normal draw
		fn = 3;

	if (rx<=0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (j = y + ry; j >= y - ry; j--)
			if (create_parts2(fn,x,j,c,rx,ry,flags))
				f = 1;
	}
	else
	{
		int tempy = y, i, j, jmax, oldy;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush
		if (CURRENT_BRUSH == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++) {
			oldy = tempy;
			// Fix a problem with the triangle brush which occurs if the bottom corner (the first point tested) isn't recognised as being inside the brush
			if (!InCurrentBrush(i-x,tempy-y,rx,ry))
				continue;
			while (InCurrentBrush(i-x,tempy-y,rx,ry))
				tempy = tempy - 1;
			tempy = tempy + 1;
			if (fill)
			{
				jmax = 2*y - tempy;
				if (CURRENT_BRUSH == TRI_BRUSH)
					jmax = y + ry;
				for (j = jmax; j >= tempy; j--) {
					if (create_parts2(fn,i,j,c,rx,ry,flags))
						f = 1;
					if (i!=x && create_parts2(fn,2*x-i,j,c,rx,ry,flags))
						f = 1;
				}
			}
			else
			{
				if ((oldy != tempy && CURRENT_BRUSH != SQUARE_BRUSH) || i == x-rx)
					oldy--;
				//if (CURRENT_BRUSH == TRI_BRUSH)
				//	oldy = tempy;
				for (j = oldy+1; j >= tempy; j--) {
					int i2 = 2*x-i, j2 = 2*y-j;
					if (CURRENT_BRUSH == TRI_BRUSH)
						j2 = y+ry;
					if (create_parts2(fn,i,j,c,rx,ry,flags))
						f = 1;
					if (i2 != i && create_parts2(fn,i2,j,c,rx,ry,flags))
						f = 1;
					if (j2 != j && create_parts2(fn,i,j2,c,rx,ry,flags))
						f = 1;
					if (i2 != i && j2 != j && create_parts2(fn,i2,j2,c,rx,ry,flags))
						f = 1;
				}
			}
		}
	}
	return !f;
}

int create_parts2(int f, int x, int y, int c, int rx, int ry, int flags)
{
	if (f == 0)      //delete
		delete_part(x, y, 0);
	else if (f == 1) //specific delete
		delete_part(x, y, flags);
	else if (f == 2) //replace mode
	{
		if (x<0 || y<0 || x>=XRES || y>=YRES)
			return 0;
		if (SLALT!=0 && globalSim->pmap_find_one(x,y,SLALT)<0)
			return 0;
		if (globalSim->pmap[y][x].count)
		{
			delete_part(x, y, 0);
			if (c!=0)
				create_part_add_props(-2, x, y, c, rx, ry);
		}
	}
	else if (f == 3) //normal draw
		if (create_part_add_props(-2, x, y, c, rx, ry)==-1)
			return 1;
	return 0;
}

int InCurrentBrush(int i, int j, int rx, int ry)
{
	switch(CURRENT_BRUSH)
	{
		case CIRCLE_BRUSH:
			return (pow((double)i,2)*pow((double)ry,2)+pow((double)j,2)*pow((double)rx,2)<=pow((double)rx,2)*pow((double)ry,2));
			break;
		case SQUARE_BRUSH:
			return (abs(i) <= rx && abs(j) <= ry);
			break;
		case TRI_BRUSH:
			return ((abs((rx+2*i)*ry+rx*j) + abs(2*rx*(j-ry)) + abs((rx-2*i)*ry+rx*j))<=(4*rx*ry));
			break;
		default:
			return 0;
			break;
	}
}
int get_brush_flags()
{
	int flags = 0;
	if (REPLACE_MODE)
		flags |= BRUSH_REPLACEMODE;
	if (sdl_mod & (KMOD_CAPS))
		flags |= BRUSH_SPECIFIC_DELETE;
	if (sdl_mod & (KMOD_LALT) && sdl_mod & (KMOD_CTRL))
		flags |= BRUSH_SPECIFIC_DELETE;
	return flags;
}
void create_line(int x1, int y1, int x2, int y2, int rx, int ry, int c, int flags)
{
	int cp=abs(y2-y1)>abs(x2-x1), x, y, dx, dy, sy, fill = 1;
	float e, de;
	if (c==SPC_PROP)
		return;
	if (cp)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (cp)
			create_parts(y, x, rx, ry, c, flags, fill);
		else
			create_parts(x, y, rx, ry, c, flags, fill);
		fill = 0;
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((c==WL_EHOLE+100 || c==WL_ALLOWGAS+100 || c==WL_ALLOWENERGY+100 || c==WL_ALLOWALLELEC+100 || c==WL_ALLOWSOLID+100 || c==WL_ALLOWAIR+100 || c==WL_WALL+100 || c==WL_DESTROYALL+100 || c==WL_ALLOWLIQUID+100 || c==WL_FAN+100 || c==WL_STREAM+100 || c==WL_DETECT+100 || c==WL_EWALL+100 || c==WL_WALLELEC+100 || !(rx+ry))
			   && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (cp)
					create_parts(y, x, rx, ry, c, flags, fill);
				else
					create_parts(x, y, rx, ry, c, flags, fill);
			}
			e -= 1.0f;
		}
	}
}

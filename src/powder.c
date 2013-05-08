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
#include <air.h>
#include <misc.h>
#include "gravity.h"
#ifdef LUACONSOLE
#include <luaconsole.h>
#endif

#include "simulation/Simulation.h"
#include "simulation/CoordStack.h"
#include "simulation/ElementDataContainer.h"
#include "simulation/elements/PRTI.h"

part_type ptypes[PT_NUM];
part_transition ptransitions[PT_NUM];
unsigned int platent[PT_NUM];

int lighting_recreate = 0;
int force_stacking_check = 0;//whether to force a check for excessively stacked particles

playerst player;
playerst player2;

particle *parts;
particle *cb_parts;
const particle emptyparticle = {};

int airMode = 0;
int water_equal_test = 0;

unsigned char bmap[YRES/CELL][XRES/CELL];
unsigned char emap[YRES/CELL][XRES/CELL];

unsigned char cb_bmap[YRES/CELL][XRES/CELL];
unsigned char cb_emap[YRES/CELL][XRES/CELL];

unsigned pmap[YRES][XRES];
int pmap_count[YRES][XRES];
unsigned photons[YRES][XRES];
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
int ISGOL = 0;
int ISLOVE = 0;
int ISLOLZ = 0;
int ISGRAV = 0;
int ISWIRE = 0;
int VINE_MODE = 0;

int love[XRES/9][YRES/9];
int lolz[XRES/9][YRES/9];
unsigned char gol[YRES][XRES];
unsigned char gol2[YRES][XRES][NGOL+1];

wall_type wtypes[] =
{
	{PIXPACK(0xC0C0C0), PIXPACK(0x101010), 0, "Wall. Indestructible. Blocks everything. Conductive."},
	{PIXPACK(0x808080), PIXPACK(0x808080), 0, "E-Wall. Becomes transparent when electricity is connected."},
	{PIXPACK(0xFF8080), PIXPACK(0xFF2008), 1, "Detector. Generates electricity when a particle is inside."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Streamline. Set start point of a streamline."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Sign. Click on a sign to edit it or anywhere else to place a new one."},
	{PIXPACK(0x8080FF), PIXPACK(0x000000), 1, "Fan. Accelerates air. Use line tool to set direction and strength."},
	{PIXPACK(0xC0C0C0), PIXPACK(0x101010), 2, "Wall. Blocks most particles but lets liquids through. Conductive."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 1, "Wall. Absorbs particles but lets air currents through."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 0, "Erases walls."},
	{PIXPACK(0x808080), PIXPACK(0x000000), 3, "Wall. Indestructible. Blocks everything."},
	{PIXPACK(0x3C3C3C), PIXPACK(0x000000), 1, "Wall. Indestructible. Blocks particles, allows air"},
	{PIXPACK(0x575757), PIXPACK(0x000000), 1, "Wall. Indestructible. Blocks liquids and gases, allows powders"},
	{PIXPACK(0xFFFF22), PIXPACK(0x101010), 2, "Conductor, allows particles, conducts electricity"},
	{PIXPACK(0x242424), PIXPACK(0x101010), 0, "E-Hole, absorbs particles, release them when powered"},
	{PIXPACK(0xFFFFFF), PIXPACK(0x000000), -1, "Air, creates airflow and pressure"},
	{PIXPACK(0xFFBB00), PIXPACK(0x000000), -1, "Heats the targeted element."},
	{PIXPACK(0x00BBFF), PIXPACK(0x000000), -1, "Cools the targeted element."},
	{PIXPACK(0x303030), PIXPACK(0x000000), -1, "Vacuum, reduces air pressure."},
	{PIXPACK(0x579777), PIXPACK(0x000000), 1, "Wall. Indestructible. Blocks liquids and solids, allows gases"},
	{PIXPACK(0x000000), PIXPACK(0x000000), -1, "Drag tool"},
	{PIXPACK(0xFFEE00), PIXPACK(0xAA9900), 4, "Gravity wall"},
	{PIXPACK(0x0000BB), PIXPACK(0x000000), -1, "Positive gravity tool."},
	{PIXPACK(0x000099), PIXPACK(0x000000), -1, "Negative gravity tool."},
	{PIXPACK(0xFFAA00), PIXPACK(0xAA5500), 4, "Energy wall, allows only energy type particles to pass"},
	{PIXPACK(0xFFAA00), PIXPACK(0xAA5500), -1, "Property edit tool"},
};

void get_gravity_field(int x, int y, float particleGrav, float newtonGrav, float *pGravX, float *pGravY)
{
	*pGravX = newtonGrav*gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
	*pGravY = newtonGrav*gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
	switch (gravityMode)
	{
		default:
		case 0: //normal, vertical gravity
			*pGravY += particleGrav;
			break;
		case 1: //no gravity
			break;
		case 2: //radial gravity
			if (x-XCNTR != 0 || y-YCNTR != 0)
			{
				float pGravMult = particleGrav/sqrtf((x-XCNTR)*(x-XCNTR) + (y-YCNTR)*(y-YCNTR));
				*pGravX -= pGravMult * (float)(x - XCNTR);
				*pGravY -= pGravMult * (float)(y - YCNTR);
			}
	}
}

static int pn_junction_sprk(int x, int y, int pt)
{
	int rcount, ri, rnext;
	bool ptFound = false;
	FOR_PMAP_POSITION(globalSim, x, y, rcount, ri, rnext)// TODO: not energy parts
	{
		if (parts[ri].type==pt)
		{
			ptFound = true;
			break;
		}
	}
	if (!ptFound)
		return 0;

	return globalSim->spark_conductive_attempt(ri, x, y);
}

static void photoelectric_effect(int nx, int ny)//create sparks from PHOT when hitting PSCN and NSCN
{
	if (globalSim->pmap_find_one(nx,ny,PT_PSCN)>=0)
	{
		if (globalSim->pmap_find_one(nx-1,ny,PT_NSCN)>=0 ||
		        globalSim->pmap_find_one(nx+1,ny,PT_NSCN)>=0 ||
		        globalSim->pmap_find_one(nx,ny-1,PT_NSCN)>=0 ||
		        globalSim->pmap_find_one(nx,ny+1,PT_NSCN)>=0)
			pn_junction_sprk(nx, ny, PT_PSCN);
	}
}

unsigned char can_move[PT_NUM][PT_NUM];

void init_can_move()
{
	// can_move[moving type][type at destination]
	//  0 = No move/Bounce
	//  1 = Swap
	//  2 = Both particles occupy the same space.
	//  3 = Varies, go run some extra checks
	int t, rt, stkm_move;
	for (rt=0;rt<PT_NUM;rt++)
		can_move[0][rt] = 0; // particles that don't exist shouldn't move...
	for (t=1;t<PT_NUM;t++)
		can_move[t][0] = 2;
	for (t=1;t<PT_NUM;t++)
		for (rt=0;rt<PT_NUM;rt++)
			can_move[t][rt] = 1;
	for (rt=1;rt<PT_NUM;rt++)
	{
		can_move[PT_PHOT][rt] = 2;
	}
	for (t=1;t<PT_NUM;t++)
	{
		for (rt=1;rt<PT_NUM;rt++)
		{
			// weight check, also prevents particles of same type displacing each other
			if (ptypes[t].weight <= ptypes[rt].weight || rt==PT_GEL) can_move[t][rt] = 0;
			if (t==PT_NEUT && ptypes[rt].properties&PROP_NEUTPASS)
				can_move[t][rt] = 2;
			if (t==PT_NEUT && ptypes[rt].properties&PROP_NEUTABSORB)
				can_move[t][rt] = 1;
			if (t==PT_NEUT && ptypes[rt].properties&PROP_NEUTPENETRATE)
				can_move[t][rt] = 1;
			if (ptypes[t].properties&PROP_NEUTPENETRATE && rt==PT_NEUT)
				can_move[t][rt] = 0;
			if (ptypes[t].properties&TYPE_ENERGY && ptypes[rt].properties&TYPE_ENERGY)
				can_move[t][rt] = 2;
		}
	}
	can_move[PT_DEST][PT_DMND] = 0;
	can_move[PT_DEST][PT_CLNE] = 0;
	can_move[PT_DEST][PT_PCLN] = 0;
	can_move[PT_DEST][PT_BCLN] = 0;
	can_move[PT_DEST][PT_PBCN] = 0;
	can_move[PT_BIZR][PT_FILT] = 2;
	can_move[PT_BIZRG][PT_FILT] = 2;
	for (t=0;t<PT_NUM;t++)
	{
		//spark shouldn't move
		can_move[PT_SPRK][t] = 0;
		stkm_move = 0;
		if (ptypes[t].properties & (TYPE_LIQUID | TYPE_GAS))
			stkm_move = 2;
		if (!t || t==PT_PRTO || t==PT_SPAWN || t==PT_SPAWN2)
			stkm_move = 2;
		can_move[PT_STKM][t] = stkm_move;
		can_move[PT_STKM2][t] = stkm_move;
		can_move[PT_FIGH][t] = stkm_move;
	}
	for (t=1;t<PT_NUM;t++)
	{
		// make them eat things
		can_move[t][PT_BHOL] = 1;
		can_move[t][PT_NBHL] = 1;
		can_move[t][PT_STKM] = 0;
		can_move[t][PT_STKM2] = 0;
		can_move[t][PT_FIGH] = 0;
		//INVIS behaviour varies with pressure
		can_move[t][PT_INVIS] = 3;
		//stop CNCT being displaced by other particles
		can_move[t][PT_CNCT] = 0;
		//void behaviour varies with powered state and ctype
		can_move[t][PT_PVOD] = 3;
		can_move[t][PT_VOID] = 3;
		can_move[t][PT_EMBR] = 0;
		can_move[PT_EMBR][t] = 0;
		if (ptypes[t].properties&TYPE_ENERGY)
		{
			can_move[t][PT_VIBR] = 1;
			can_move[t][PT_BVBR] = 1;
		}
	}
	for (t=0;t<PT_NUM;t++)
	{
		if (t==PT_GLAS || t==PT_PHOT || t==PT_CLNE || t==PT_PCLN
			|| t==PT_GLOW || t==PT_WATR || t==PT_DSTW || t==PT_SLTW
			|| t==PT_ISOZ || t==PT_ISZS || t==PT_FILT || t==PT_INVIS
			|| t==PT_QRTZ || t==PT_PQRT)
			can_move[PT_PHOT][t] = 2;
	}
	can_move[PT_ELEC][PT_LCRY] = 2;
	can_move[PT_ELEC][PT_EXOT] = 2;
	can_move[PT_NEUT][PT_EXOT] = 2;
	can_move[PT_PHOT][PT_LCRY] = 3;//varies according to LCRY life
	
	can_move[PT_PHOT][PT_BIZR] = 2;
	can_move[PT_ELEC][PT_BIZR] = 2;
	can_move[PT_PHOT][PT_BIZRG] = 2;
	can_move[PT_ELEC][PT_BIZRG] = 2;
	can_move[PT_PHOT][PT_BIZRS] = 2;
	can_move[PT_ELEC][PT_BIZRS] = 2;
	
	can_move[PT_NEUT][PT_INVIS] = 2;
	//whol eats anar
	can_move[PT_ANAR][PT_WHOL] = 1;
	can_move[PT_ANAR][PT_NWHL] = 1;
	can_move[PT_ELEC][PT_DEUT] = 1;
	can_move[PT_THDR][PT_THDR] = 2;
	can_move[PT_EMBR][PT_EMBR] = 2;
	can_move[PT_TRON][PT_SWCH] = 3;
}

int eval_move_special(int pt, int nx, int ny, int ri, int result)
{
	int rt = parts[ri].type;
	if ((pt==PT_PHOT || pt==PT_ELEC) && rt==PT_LCRY)
		result = (parts[ri].life > 5)? 2 : 0;
	if (rt==PT_INVIS)
	{
		if (pv[ny/CELL][nx/CELL]>4.0f || pv[ny/CELL][nx/CELL]<-4.0f) result = 2;
		else result = 0;
	}
	else if (rt==PT_PVOD)
	{
		if (parts[ri].life == 10)
		{
			if(!parts[ri].ctype || (parts[ri].ctype==pt)!=(parts[ri].tmp&1))
				result = 1;
			else
				result = 0;
		}
		else result = 0;
	}
	else if (rt==PT_VOID)
	{
		if(!parts[ri].ctype || (parts[ri].ctype==pt)!=(parts[ri].tmp&1))
			result = 1;
		else
			result = 0;
	}
	else if (pt == PT_TRON && rt == PT_SWCH)
	{
		if (parts[ri].life >= 10)
			result = 2;
		else
			result = 0;
	}
	return result;
}

/*
   RETURN-value explanation
1 = Swap
0 = No move/Bounce
2 = Both particles occupy the same space. This is also returned for a particle moving into empty space.
 */
// TODO: add a return value for particle will be killed? Though for some materials, like PRTI, this won't be known until we try to do it.
int eval_move(int pt, int nx, int ny, unsigned *rr)
{
	int result = 2;
	int rcount, ri, rnext;

	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 0;

	if (globalSim->pmap[ny][nx].count)
	{
		FOR_PMAP_POSITION(globalSim, nx, ny, rcount, ri, rnext)// TODO: not energy parts
		{
			int tmpResult = can_move[pt][parts[ri].type];
			if (tmpResult==3)
				tmpResult = eval_move_special(pt, nx, ny, ri, tmpResult);
			// Find the particle which restricts movement the most
			if (tmpResult<result)
				result = tmpResult;
		}
	}

	if (rr)
	{
		// TODO: remove this
		if (globalSim->pmap[ny][nx].count)
			*rr = (globalSim->pmap[ny][nx].first<<8) | parts[globalSim->pmap[ny][nx].first].type;
	}

	if (bmap[ny/CELL][nx/CELL])
	{
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWGAS && !(ptypes[pt].properties&TYPE_GAS))// && ptypes[pt].falldown!=0 && pt!=PT_FIRE && pt!=PT_SMKE)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWENERGY && !(ptypes[pt].properties&TYPE_ENERGY))// && ptypes[pt].falldown!=0 && pt!=PT_FIRE && pt!=PT_SMKE)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWLIQUID && ptypes[pt].falldown!=2)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWSOLID && ptypes[pt].falldown!=1)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_ALLOWAIR || bmap[ny/CELL][nx/CELL]==WL_WALL || bmap[ny/CELL][nx/CELL]==WL_WALLELEC)
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_EWALL && !emap[ny/CELL][nx/CELL])
			return 0;
		if (bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL] && !(ptypes[pt].properties&TYPE_SOLID))
		{
			bool foundSolid = false;
			FOR_PMAP_POSITION(globalSim, nx, ny, rcount, ri, rnext)// TODO: not energy parts
			{
				if (ptypes[parts[ri].type].properties&TYPE_SOLID)
				{
					foundSolid = true;
					break;
				}
			}
			if (!foundSolid)
				return 2;
		}
	}
	return result;
}

/*
   return value
-1 = moving particle was killed
0 = No move/Bounce
1 = Swap
2 = Both particles occupy the same space. This is also returned for a particle moving into empty space.
 */
int try_move(int i, int x, int y, int nx, int ny)
{
	int e;
	int rcount, ri, rnext, rt;

	if (x==nx && y==ny)
		return 1;
	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return 1;

	int t = parts[i].type;
	e = eval_move(t, nx, ny, NULL);

	/* half-silvered mirror */
	if (!e && t==PT_PHOT &&
	        ((globalSim->pmap_find_one(nx,ny,PT_BMTL)>=0 && rand()<RAND_MAX/2) ||
	         globalSim->pmap_find_one(x,y,PT_BMTL)>=0))
		e = 2;

	if (!e) //if no movement
	{
		if (globalSim->elements[t].Properties & TYPE_ENERGY)
		{
			FOR_PMAP_POSITION(globalSim, nx, ny, rcount, ri, rnext)// TODO: not energy parts
			{
				rt = parts[ri].type;
				if (!legacy_enable && t==PT_PHOT)//PHOT heat conduction
				{
					if (rt == PT_COAL || rt == PT_BCOL)
						parts[ri].temp = parts[i].temp;

					if (rt < PT_NUM && ptypes[rt].hconduct && (rt!=PT_HSWC||parts[ri].life==10) && rt!=PT_FILT)
						parts[i].temp = parts[ri].temp = restrict_flt((parts[ri].temp+parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
				}
				if (rt==PT_CLNE || rt==PT_PCLN || rt==PT_BCLN || rt==PT_PBCN) {
					if (!parts[ri].ctype)
						parts[ri].ctype = t;
				}
				if (rt==PT_PRTI)
				{
					PortalChannel *channel = ((PRTI_ElementDataContainer*)globalSim->elementData[PT_PRTI])->GetParticleChannel(globalSim, ri);
					int slot = PRTI_ElementDataContainer::GetSlot(x-nx,y-ny);
					if (channel->StoreParticle(globalSim, i, slot))
						return -1;
				}
				if ((rt==PT_PIPE || rt == PT_PPIP) && !(parts[ri].tmp&0xFF))
				{
					parts[ri].tmp =  (parts[ri].tmp&~0xFF) | t;
					parts[ri].temp = parts[i].temp;
					parts[ri].tmp2 = parts[i].life;
					parts[ri].pavg[0] = parts[i].tmp;
					parts[ri].pavg[1] = parts[i].ctype;
					kill_part(i);
					return -1;
				}
				if (t==PT_PHOT)
					parts[i].ctype &= globalSim->elements[rt].PhotonReflectWavelengths;
			}
		}
		return 0;
	}

	if (e == 2) //if occupy same space
	{
		FOR_PMAP_POSITION(globalSim, nx, ny, rcount, ri, rnext)// TODO: not energy parts
		{
			rt = parts[ri].type;
			if (t==PT_PHOT)
			{
				parts[i].ctype &= globalSim->elements[rt].PhotonReflectWavelengths;
				if (rt==PT_GLOW && !parts[ri].life)
				{
					if (rand() < RAND_MAX/30)
					{
						parts[ri].life = 120;
						create_gain_photon(i);
					}
				}
				if (rt==PT_BIZR || rt==PT_BIZRG || rt==PT_BIZRS)
				{
					part_change_type(i, x, y, PT_ELEC);
					parts[i].ctype = 0;
				}
				if (rt==PT_INVIS && pv[ny/CELL][nx/CELL]<=4.0f && pv[ny/CELL][nx/CELL]>=-4.0f)
				{
					part_change_type(i,x,y,PT_NEUT);
					parts[i].ctype = 0;
				}
			}
			if ((t==PT_BIZR||t==PT_BIZRG||t==PT_PHOT) && rt==PT_FILT)
			{
				int temp_bin = (int)((parts[ri].temp-273.0f)*0.025f);
				if (temp_bin < 0) temp_bin = 0;
				if (temp_bin > 25) temp_bin = 25;
				if(!parts[ri].tmp){
					parts[i].ctype = 0x1F << temp_bin; //Assign Colour
				} else if(parts[ri].tmp==1){
					parts[i].ctype &= 0x1F << temp_bin; //Filter Colour
				} else if(parts[ri].tmp==2){
					parts[i].ctype |= 0x1F << temp_bin; //Add Colour
				} else if(parts[ri].tmp==3){
					parts[i].ctype &= ~(0x1F << temp_bin); //Subtract Colour
				}
			}
			if (t == PT_NEUT && rt==PT_GLAS)
			{
				if (rand() < RAND_MAX/10)
					create_cherenkov_photon(i);
			}
		}
		return 1;
	}
	//else e=1 , we are trying to swap the particles

	if (t==PT_CNCT && y<ny && globalSim->pmap_find_one(x, y+1, PT_CNCT)>=0)//check below CNCT for another CNCT
		return 0;
			
	int srcNeutPenetrate = -1;
	int destNeutPenetrate = -1;
	bool srcPartFound = false;

	// e=1 for neutron means that target material is NEUTPENETRATE, meaning it gets moved around when neutron passes
	// First, look for NEUTPENETRATE or empty space at x,y
	if (t==PT_NEUT && destNeutPenetrate>=0)
	{
		FOR_PMAP_POSITION(globalSim, x, y, rcount, ri, rnext)// TODO: not energy parts
		{
			if (globalSim->elements[parts[ri].type].Properties&PROP_NEUTPENETRATE)
			{
				srcNeutPenetrate = ri;
				break;
			}
			else if (!(globalSim->elements[parts[ri].type].Properties&TYPE_ENERGY))
			{
				srcPartFound = true;
				break;
			}
		}
	}

	FOR_PMAP_POSITION(globalSim, nx, ny, rcount, ri, rnext)// TODO: not energy parts
	{
		rt = parts[ri].type;
		if (t==PT_NEUT && (ptypes[rt].properties&PROP_NEUTABSORB))
		{
			kill_part(i);
			return -1;
		}
		if (rt==PT_VOID || rt==PT_PVOD) //this is where void eats particles
		{
			//void ctype already checked in eval_move
			kill_part(i);
			return -1;
		}
		if (rt==PT_BHOL || rt==PT_NBHL) //this is where blackhole eats particles
		{
			if (!legacy_enable)
			{
				parts[ri].temp = restrict_flt(parts[ri].temp+parts[i].temp/2, MIN_TEMP, MAX_TEMP);//3.0f;
			}
			kill_part(i);
			return -1;
		}
		if ((rt==PT_WHOL||rt==PT_NWHL) && t==PT_ANAR) //whitehole eats anar
		{
			if (!legacy_enable)
			{
				parts[ri].temp = restrict_flt(parts[ri].temp- (MAX_TEMP-parts[i].temp)/2, MIN_TEMP, MAX_TEMP);
			}
			kill_part(i);
			return -1;
		}
		if (rt==PT_DEUT && t==PT_ELEC)
		{
			if(parts[ri].life < 6000)
				parts[ri].life += 1;
			parts[ri].temp = 0;
			kill_part(i);
			return -1;
		}
		if ((rt==PT_VIBR || rt==PT_BVBR) && (ptypes[t].properties & TYPE_ENERGY))
		{
			parts[ri].tmp += 20;
			kill_part(i);
			return -1;
		}
		if ((bmap[y/CELL][x/CELL]==WL_EHOLE && !emap[y/CELL][x/CELL]) && !(bmap[ny/CELL][nx/CELL]==WL_EHOLE && !emap[ny/CELL][nx/CELL]))
			return 0;

		if(t==PT_GBMB&&parts[i].life>0)
			return 0;
		
		if (t==PT_NEUT && destNeutPenetrate<0 && (globalSim->elements[rt].Properties&PROP_NEUTPENETRATE))
			destNeutPenetrate = ri;

		// Move all particles at the destination position that can be displaced by this particle
		// For NEUTPENETRATE particles being displaced by a neutron, only move ones at nx,ny to x,y if there's currently a NEUTPENETRATE particle or empty space at x,y
		if (!(globalSim->elements[rt].Properties&TYPE_ENERGY) && (t!=PT_NEUT || srcNeutPenetrate>=0 || !srcPartFound))
		{
			int tmpResult = can_move[t][rt];
			if (tmpResult==3)
				tmpResult = eval_move_special(t, nx, ny, ri, tmpResult);
			if (tmpResult==1)
			{
				globalSim->pmap_remove(ri, nx, ny);
				parts[ri].x += x-nx;
				parts[ri].y += y-ny;
				globalSim->pmap_add(ri, (int)(parts[ri].x+0.5f), (int)(parts[ri].y+0.5f), rt);
			}
		}
	}
	

	if (t==PT_NEUT && srcNeutPenetrate>=0)
	{
		globalSim->pmap_remove(srcNeutPenetrate, x, y);
		globalSim->pmap_add(srcNeutPenetrate, nx, ny, parts[srcNeutPenetrate].type);
		parts[srcNeutPenetrate].x = nx;
		parts[srcNeutPenetrate].y = ny;
	}

	return 1;
}

// try to move particle, and if successful update pmap and parts[i].x,y
int do_move(int i, int x, int y, float nxf, float nyf)
{
	// volatile to hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
	volatile float tmpx = nxf, tmpy = nyf;
	int nx = (int)(tmpx+0.5f), ny = (int)(tmpy+0.5f), result;
	if (parts[i].type == PT_NONE)
		return 0;
	result = try_move(i, x, y, nx, ny);
	if (result>0)
	{
		int t = parts[i].type;
		if (ny!=y || nx!=x)
		{
			if (nx<CELL || nx>=XRES-CELL || ny<CELL || ny>=YRES-CELL)//kill_part if particle is out of bounds
			{
				kill_part(i);
				return -1;
			}
			globalSim->pmap_remove(i, x, y);
			globalSim->pmap_add(i, nx, ny, t);
		}
		// Assign coords after the out of bounds check which might kill the particle, because kill_part > pmap_remove uses the current particle coords
		parts[i].x = tmpx;
		parts[i].y = tmpy;
	}
	return result;
}


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

	return !eval_move(t, x, y, NULL);
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
		photoelectric_effect(x, y);

	return get_normal(pt, x, y, dx, dy, nx, ny);
}

//For soap only
void detach(int i)
{
	if ((parts[i].ctype&2) == 2)
	{
		if ((parts[parts[i].tmp].ctype&4) == 4)
			parts[parts[i].tmp].ctype ^= 4;
	}

	if ((parts[i].ctype&4) == 4)
	{
		if ((parts[parts[i].tmp2].ctype&2) == 2)
			parts[parts[i].tmp2].ctype ^= 2;
	}

	parts[i].ctype = 0;
}

void kill_part(int i)//kills particle number i
{
	globalSim->part_kill(i);
}

void part_change_type(int i, int x, int y, int t)//changes the type of particle number i, to t.  This also changes pmap at the same time.
{
	globalSim->part_change_type(i, x, y, t);
}

int create_part(int p, int x, int y, int tv)//the function for creating a particle, use p=-1 for creating a new particle, -2 is from a brush, or a particle number to replace a particle.
{
	int rcount, ri, rnext;
	int i;

	int t = tv & 0xFF;
	int v = (tv >> 8) & 0xFF;
	
	if (x<0 || y<0 || x>=XRES || y>=YRES || ((t<=0 || t>=PT_NUM)&&t!=SPC_HEAT&&t!=SPC_COOL&&t!=SPC_AIR&&t!=SPC_VACUUM&&t!=SPC_PGRV&&t!=SPC_NGRV))
		return -1;
	if (t>=0 && t<PT_NUM && !ptypes[t].enabled)
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
				
				parts[ri].temp = restrict_flt(parts[ri].temp + heatchange, MIN_TEMP, MAX_TEMP);
			}
			if (t==SPC_COOL&&parts[ri].temp>MIN_TEMP)
			{
				float heatchange;
				int fast = ((sdl_mod & (KMOD_SHIFT)) && (sdl_mod & (KMOD_CTRL)));
				if (parts[ri].type==PT_PUMP || parts[ri].type==PT_GPMP)
					heatchange = fast?1.0f:.1f;
				else
					heatchange = fast?50.0f:4.0f;
				
				parts[ri].temp = restrict_flt(parts[ri].temp - heatchange, MIN_TEMP, MAX_TEMP);
			}
		}
		return ri;
	}
	if (t==SPC_AIR)
	{
		pv[y/CELL][x/CELL] += 0.10f;
		return -1;
	}
	if (t==SPC_VACUUM)
	{
		pv[y/CELL][x/CELL] -= 0.10f;
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
		int index, type, lastSparkedIndex=-1;
		FOR_PMAP_POSITION(globalSim, x, y, rcount, index, rnext)
		{
			int type = parts[index].type;
			if (type == PT_WIRE)
			{
				parts[index].ctype = PT_DUST;
				lastSparkedIndex = index;
				continue;
			}
			if (!(type == PT_INST || (ptypes[type].properties&PROP_CONDUCTS)))
				continue;
			if (parts[index].life!=0)
				continue;
			if (p == -2 && type == PT_INST)
			{
				INST_flood_spark(globalSim, x, y);
				lastSparkedIndex = index;
				continue;
			}
			if (globalSim->spark_conductive_attempt(index, x, y))
				lastSparkedIndex = index;
		}
		return lastSparkedIndex;
	}
	if (p==-2)//creating from brush
	{
		bool energyParticleFound = false;
		bool normalParticleFound = false;
		bool actionDone = false;
		int pmaptype;
		FOR_PMAP_POSITION(globalSim, x, y, rcount, ri, rnext)
		{
			pmaptype = parts[ri].type;
			if (globalSim->elements[pmaptype].Properties&TYPE_ENERGY)
				energyParticleFound = true;
			else
				normalParticleFound = true;

			if ((
				(pmaptype==PT_STOR&&!(ptypes[t].properties&TYPE_SOLID))||
				pmaptype==PT_CLNE||
				pmaptype==PT_BCLN||
				pmaptype==PT_CONV||
				(pmaptype==PT_PCLN&&t!=PT_PSCN&&t!=PT_NSCN)||
				(pmaptype==PT_PBCN&&t!=PT_PSCN&&t!=PT_NSCN)
			)&&(
				t!=PT_CLNE&&t!=PT_PCLN&&
				t!=PT_BCLN&&t!=PT_STKM&&
				t!=PT_STKM2&&t!=PT_PBCN&&
				t!=PT_STOR&&t!=PT_FIGH&&
				t!=PT_CONV)
			)
			{
				parts[ri].ctype = t;
				actionDone = true;
				if (t==PT_LIFE && v<NGOLALT && pmaptype!=PT_STOR)
					parts[ri].tmp = v;
			}
			else if (pmaptype == PT_DTEC && pmaptype != t)
			{
				parts[ri].ctype = t;
				actionDone = true;
				if (t==PT_LIFE && v<NGOLALT)
					parts[ri].tmp = v;
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

void create_gain_photon(int pp)//photons from PHOT going through GLOW
{
	float xx, yy;
	int i, lr, temp_bin, nx, ny;

	lr = rand() % 2;

	if (lr) {
		xx = parts[pp].x - 0.3*parts[pp].vy;
		yy = parts[pp].y + 0.3*parts[pp].vx;
	} else {
		xx = parts[pp].x + 0.3*parts[pp].vy;
		yy = parts[pp].y - 0.3*parts[pp].vx;
	}

	nx = (int)(xx + 0.5f);
	ny = (int)(yy + 0.5f);

	if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
		return;

	int glow_i = globalSim->pmap_find_one(nx, ny, PT_GLOW);
	if (glow_i<0)
		return;

	i = globalSim->part_create(-1, nx, ny, PT_PHOT);
	if (i<0)
		return;

	parts[i].life = 680;
	parts[i].x = xx;
	parts[i].y = yy;
	parts[i].vx = parts[pp].vx;
	parts[i].vy = parts[pp].vy;
	parts[i].temp = parts[glow_i].temp;

	temp_bin = (int)((parts[i].temp-273.0f)*0.25f);
	if (temp_bin < 0) temp_bin = 0;
	if (temp_bin > 25) temp_bin = 25;
	parts[i].ctype = 0x1F << temp_bin;
}

void create_cherenkov_photon(int pp)//photons from NEUT going through GLAS
{
	int i, lr, nx, ny;
	float r, eff_ior;

	nx = (int)(parts[pp].x + 0.5f);
	ny = (int)(parts[pp].y + 0.5f);
	int glass_i = globalSim->pmap_find_one(nx, ny, PT_GLAS);
	if (glass_i<0)
		return;

	if (hypotf(parts[pp].vx, parts[pp].vy) < 1.44f)
		return;

	i = globalSim->part_create(-1, nx, ny, PT_PHOT);
	if (i<0)
		return;

	lr = rand() % 2;

	parts[i].ctype = 0x00000F80;
	parts[i].life = 680;
	parts[i].x = parts[pp].x;
	parts[i].y = parts[pp].y;
	parts[i].temp = parts[glass_i].temp;
	parts[i].pavg[0] = parts[i].pavg[1] = 0.0f;

	if (lr) {
		parts[i].vx = parts[pp].vx - 2.5f*parts[pp].vy;
		parts[i].vy = parts[pp].vy + 2.5f*parts[pp].vx;
	} else {
		parts[i].vx = parts[pp].vx + 2.5f*parts[pp].vy;
		parts[i].vy = parts[pp].vy - 2.5f*parts[pp].vx;
	}

	/* photons have speed of light. no discussion. */
	r = 1.269 / hypotf(parts[i].vx, parts[i].vy);
	parts[i].vx *= r;
	parts[i].vy *= r;
}

TPT_INLINE void delete_part(int x, int y, int flags)//calls kill_part with the particle located at x,y
{
	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return;

	if (!globalSim->pmap[y][x].count)
		return;

	if (!(flags&BRUSH_SPECIFIC_DELETE))
	{
		kill_part(globalSim->pmap[y][x].first);
	}
	else
	{
		int count, i, next;
		FOR_PMAP_POSITION(globalSim, x, y, count, i, next)
		{
			if (parts[i].type==SLALT || SLALT==0)//specific deletion
			{
				kill_part(i);
			}
			else if (ptypes[parts[i].type].menusection==SEC)//specific menu deletion
			{
				kill_part(i);
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

int get_wavelength_bin(int *wm)
{
	int i, w0=30, wM=0;

	if (!*wm)
		return -1;

	for (i=0; i<30; i++)
		if (*wm & (1<<i)) {
			if (i < w0)
				w0 = i;
			if (i > wM)
				wM = i;
		}

	if (wM-w0 < 5)
		return (wM+w0)/2;

	i = rand() % (wM-w0-3);
	i += w0;

	*wm &= 0x1F << i;
	return i + 2;
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

TPT_GNU_INLINE int parts_avg(int ci, int ni,int t)
{
	if (t==PT_INSL)//to keep electronics working
	{
		// TODO: not energy parts
		if (globalSim->pmap_find_one(((int)(parts[ci].x+0.5f) + (int)(parts[ni].x+0.5f))/2, ((int)(parts[ci].y+0.5f) + (int)(parts[ni].y+0.5f))/2, t)>=0)
			return t;
		else
			return PT_NONE;
	}
	else
	{
		if (globalSim->pmap_find_one((int)((parts[ci].x + parts[ni].x)/2+0.5f), (int)((parts[ci].y + parts[ni].y)/2+0.5f), t)>=0)
			return t;
		else
			return PT_NONE;
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
	for (i=0; i<=parts_lastActiveIndex; i++)
	{
		if ((parts[i].type==t||(t==-1&&parts[i].type))&&!parts[i].life&&i!=ci)
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

int parts_lastActiveIndex = NPART-1;
void update_wallmaps()
{
	int x, y;
	if (!sys_pause||framerender)
	{
		for (y=0; y<YRES/CELL; y++)
		{
			for (x=0; x<XRES/CELL; x++)
			{
				if (emap[y][x])
					emap[y][x] --;
				bmap_blockair[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || (bmap[y][x]==WL_EWALL && !emap[y][x]));
				bmap_blockairh[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_GRAV || (bmap[y][x]==WL_EWALL && !emap[y][x]));
			}
		}
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
			delete_part(cx+area_x, cy+area_y, 0);
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
	for (j=y1; j<=y2; j++)
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
				FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)// TODO: not energy parts
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
	int co = c;
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
			FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)
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
				FOR_PMAP_POSITION(sim, x1-1, y, rcount, ri, rnext)
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
				FOR_PMAP_POSITION(sim, x2+1, y, rcount, ri, rnext)
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
					FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)
					{
						parts[ri].flags &= ~FLAG_WATEREQUAL;//flag it as checked (different from the original particle's checked flag)
					}
				}
				else
				{
					FOR_PMAP_POSITION(sim, x, y, rcount, ri, rnext)
					{
						parts[ri].flags |= FLAG_WATEREQUAL;//flag it as checked (different from the original particle's checked flag)
					}
				}
				//check above, maybe around other sides too?
				if ( ((y-1) > originaly) && !pmap[y-1][x] && eval_move(parts[i].type, x, y-1, NULL))
				{
					sim->part_move(i, (int)(parts[i].x + 0.5f), (int)(parts[i].y + 0.5f), x, y-1);
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
					FOR_PMAP_POSITION(sim, x, y-1, rcount, ri, rnext)
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
					FOR_PMAP_POSITION(sim, x, y+1, rcount, ri, rnext)
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
	int i, j, r, f = 0, u, v, oy, ox, b = 0, dw = 0, stemp = 0, p, fn;

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
		for (j = y - ry; j <= y + ry; j++)
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
				for (j = tempy; j <= jmax; j++) {
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
				for (j = tempy; j <= oldy+1; j++) {
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
		if ((pmap[y][x]&0xFF)!=SLALT&&SLALT!=0)
			return 0;
		if ((pmap[y][x]))
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

TPT_GNU_INLINE void orbitalparts_get(int block1, int block2, int resblock1[], int resblock2[])
{
	resblock1[0] = (block1&0x000000FF);
	resblock1[1] = (block1&0x0000FF00)>>8;
	resblock1[2] = (block1&0x00FF0000)>>16;
	resblock1[3] = (block1&0xFF000000)>>24;

	resblock2[0] = (block2&0x000000FF);
	resblock2[1] = (block2&0x0000FF00)>>8;
	resblock2[2] = (block2&0x00FF0000)>>16;
	resblock2[3] = (block2&0xFF000000)>>24;
}

TPT_GNU_INLINE void orbitalparts_set(int *block1, int *block2, int resblock1[], int resblock2[])
{
	int block1tmp = 0;
	int block2tmp = 0;

	block1tmp = (resblock1[0]&0xFF);
	block1tmp |= (resblock1[1]&0xFF)<<8;
	block1tmp |= (resblock1[2]&0xFF)<<16;
	block1tmp |= (resblock1[3]&0xFF)<<24;

	block2tmp = (resblock2[0]&0xFF);
	block2tmp |= (resblock2[1]&0xFF)<<8;
	block2tmp |= (resblock2[2]&0xFF)<<16;
	block2tmp |= (resblock2[3]&0xFF)<<24;

	*block1 = block1tmp;
	*block2 = block2tmp;
}

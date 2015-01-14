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
#include "simulation/elements/FIGH.h"
#include "simulation/elements/PRTI.h"

#define INBOND(x, y) ((x)>=0 && (y)>=0 && (x)<XRES && (y)<YRES)

STKM_ElemDataSim::STKM_ElemDataSim(Simulation *s, int t)
	: STK_common_ElemDataSim(s, t),
	  obs_simCleared(sim->hook_cleared, &player, &Stickman_data::reset)
{}

Stickman_data::Stickman_data() :isStored(false), part(NULL), comm(0), pcomm(0), elem(PT_DUST), elemDelayFrames(0), rocketBoots(false)
{}

void Stickman_data::reset()
{
	set_particle(NULL);
	isStored = false;
	elem = PT_DUST;
	elemDelayFrames = 0;
	rocketBoots = false;
}

void Stickman_data::set_legs_pos(int x, int y)
{
	legs[0] = x-1;
	legs[1] = y+6;
	legs[2] = x-1;
	legs[3] = y+6;

	legs[4] = x-3;
	legs[5] = y+12;
	legs[6] = x-3;
	legs[7] = y+12;

	legs[8] = x+1;
	legs[9] = y+6;
	legs[10] = x+1;
	legs[11] = y+6;

	legs[12] = x+3;
	legs[13] = y+12;
	legs[14] = x+3;
	legs[15] = y+12;
}
void Stickman_data::set_particle(particle * p)
{
	part = p;
	if (part) set_legs_pos(part->x, part->y);
}

void STKM_ElemDataSim::on_part_create(particle &p)
{
	player.set_particle(&p);
	if (storageActionPending)
	{
		// being retrieved from portal
		storageActionPending = false;
		player.isStored = false;
	}
	else
	{
		player.elem = PT_DUST;
		player.elemDelayFrames = 0;
		player.rocketBoots = false;
	}
}

void STKM_ElemDataSim::on_part_kill(particle &p)
{
	if (&p!=player.part)
		return;
	player.set_particle(NULL);
	player.isStored = storageActionPending;
	storageActionPending = false;
}

Stickman_data * Stickman_data::get(Simulation * sim, const particle &p)
{
	if (p.type==PT_STKM || p.type==PT_STKM2)
		return &sim->elemData<STKM_ElemDataSim>(p.type)->player;
	else if (p.type==PT_FIGH)
		return sim->elemData<FIGH_ElemDataSim>(p.type)->GetFighterData(p.tmp);
	return NULL;
}

Stickman_data * Stickman_data::get(Simulation * sim, int elementId, int whichFighter)
{
	if (elementId==PT_STKM || elementId==PT_STKM2)
		return &sim->elemData<STKM_ElemDataSim>(elementId)->player;
	else if (elementId==PT_FIGH)
		return sim->elemData<FIGH_ElemDataSim>(elementId)->GetFighterData(whichFighter);
	return NULL;
}

int STKM_update(UPDATE_FUNC_ARGS)
{
	return run_stickman(&sim->elemData<STKM_ElemDataSim>(PT_STKM)->player, UPDATE_FUNC_SUBCALL_ARGS);
}

int STKM_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = *colg = *colb = *cola = 0;
	*pixel_mode = PSPEC_STICKMAN;
	return 0;
}

int run_stickman(Stickman_data* playerp, UPDATE_FUNC_ARGS)
{
	if (playerp->part != &sim->parts[i])
	{
		sim->part_kill(i);
		return 1;
	}

	int rx, ry, rt;
	int rcount, ri, rnext;
	int t = sim->parts[i].type;
	float pp, d;
	float dt = 0.9;///(FPSB*FPSB);  //Delta time in square
	float gvx, gvy;
	float gx, gy, dl, dr;
	float rocketBootsHeadEffect = 0.35f;
	float rocketBootsFeetEffect = 0.15f;
	float rocketBootsHeadEffectV = 0.3f;// stronger acceleration vertically, to counteract gravity
	float rocketBootsFeetEffectV = 0.45f;

	if ((sim->parts[i].ctype>0 && sim->parts[i].ctype<PT_NUM && sim->elements[parts[i].ctype].Enabled && sim->elements[parts[i].ctype].Falldown>0) || sim->parts[i].ctype==SPC_AIR || sim->parts[i].ctype == PT_NEUT || sim->parts[i].ctype == PT_PHOT || sim->parts[i].ctype == PT_LIGH)
		playerp->elem = parts[i].ctype;
	sim->parts[i].ctype = playerp->elem;

	if (playerp->elem != PT_LIGH)
		playerp->elemDelayFrames = 0;
	else if (playerp->elemDelayFrames>0)
		playerp->elemDelayFrames--;

	//Temperature handling
	if (parts[i].temp<243)
		parts[i].life -= 1;
	if ((parts[i].temp<309.6f) && (parts[i].temp>=243))
		parts[i].temp += 1;

	//Death
	if (parts[i].life<1 || (pv[y/CELL][x/CELL]>=4.5f && playerp->elem != SPC_AIR) ) //If his HP is less that 0 or there is very big wind...
	{
		int r;
		for (r=-2; r<=1; r++)
		{
			sim->part_create(-1, x+r, y-2, playerp->elem);
			sim->part_create(-1, x+r+1, y+2, playerp->elem);
			sim->part_create(-1, x-2, y+r+1, playerp->elem);
			sim->part_create(-1, x+2, y+r, playerp->elem);
		}
		sim->part_kill(i);  //Kill him
		return 1;
	}

	//Follow gravity
	sim->GetGravityAccel(parts[i].x, parts[i].y, 1.0f, 1.0f, gvx, gvy);

	float rbx = gvx;
	float rby = gvy;
	bool rbLowGrav = false;
	float tmp = fmaxf(fabsf(rbx), fabsf(rby));
	if (tmp < 0.001f)
	{
		rbLowGrav = true;
		rbx = -parts[i].vx;
		rby = -parts[i].vy;
		tmp = fmaxf(fabsf(rbx), fabsf(rby));
	}
	if (tmp < 0.001f)
	{
		rbx = 0;
		rby = 1.0f;
		tmp = 1.0f;
	}
	float rbx1 = rbx/tmp, rby1 = rby/tmp;// scale so that the largest is 1.0
	tmp = 1.0f/sqrtf(rbx*rbx+rby*rby);
	rbx *= tmp;// scale to a unit vector
	rby *= tmp;
	if (rbLowGrav)
	{
		rocketBootsHeadEffectV = rocketBootsHeadEffect;
		rocketBootsFeetEffectV = rocketBootsFeetEffect;
	}

	parts[i].vx -= gvx*dt;  //Head up!
	parts[i].vy -= gvy*dt;

	//Verlet integration
	pp = 2*playerp->legs[0]-playerp->legs[2]+playerp->accs[0]*dt*dt;
	playerp->legs[2] = playerp->legs[0];
	playerp->legs[0] = pp;
	pp = 2*playerp->legs[1]-playerp->legs[3]+playerp->accs[1]*dt*dt;
	playerp->legs[3] = playerp->legs[1];
	playerp->legs[1] = pp;

	pp = 2*playerp->legs[4]-playerp->legs[6]+(playerp->accs[2]+gvx)*dt*dt;
	playerp->legs[6] = playerp->legs[4];
	playerp->legs[4] = pp;
	pp = 2*playerp->legs[5]-playerp->legs[7]+(playerp->accs[3]+gvy)*dt*dt;
	playerp->legs[7] = playerp->legs[5];
	playerp->legs[5] = pp;

	pp = 2*playerp->legs[8]-playerp->legs[10]+playerp->accs[4]*dt*dt;
	playerp->legs[10] = playerp->legs[8];
	playerp->legs[8] = pp;
	pp = 2*playerp->legs[9]-playerp->legs[11]+playerp->accs[5]*dt*dt;
	playerp->legs[11] = playerp->legs[9];
	playerp->legs[9] = pp;

	pp = 2*playerp->legs[12]-playerp->legs[14]+(playerp->accs[6]+gvx)*dt*dt;
	playerp->legs[14] = playerp->legs[12];
	playerp->legs[12] = pp;
	pp = 2*playerp->legs[13]-playerp->legs[15]+(playerp->accs[7]+gvy)*dt*dt;
	playerp->legs[15] = playerp->legs[13];
	playerp->legs[13] = pp;

	//Setting acceleration to 0
	playerp->accs[0] = 0;
	playerp->accs[1] = 0;

	playerp->accs[2] = 0;
	playerp->accs[3] = 0;

	playerp->accs[4] = 0;
	playerp->accs[5] = 0;

	playerp->accs[6] = 0;
	playerp->accs[7] = 0;

	gx = (playerp->legs[4] + playerp->legs[12])/2 - gvy;
	gy = (playerp->legs[5] + playerp->legs[13])/2 + gvx;
	dl = pow(gx - playerp->legs[4], 2) + pow(gy - playerp->legs[5], 2);
	dr = pow(gx - playerp->legs[12], 2) + pow(gy - playerp->legs[13], 2);
	
	//Go left
	if (((int)(playerp->comm)&0x01) == 0x01)
	{
		bool moved = false;
		if (dl>dr)
		{
			if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[4], playerp->legs[5])))
			{
				playerp->accs[2] = -3*gvy-3*gvx;
				playerp->accs[3] = 3*gvx-3*gvy;
				playerp->accs[0] = -gvy;
				playerp->accs[1] = gvx;
				moved = true;
			}
		}
		else
		{
			if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[12], playerp->legs[13])))
			{
				playerp->accs[6] = -3*gvy-3*gvx;
				playerp->accs[7] = 3*gvx-3*gvy;
				playerp->accs[0] = -gvy;
				playerp->accs[1] = gvx;
				moved = true;
			}
		}
		if (!moved && playerp->rocketBoots)
		{
			parts[i].vx -= rocketBootsHeadEffect*rby;
			parts[i].vy += rocketBootsHeadEffect*rbx;
			playerp->accs[2] -= rocketBootsFeetEffect*rby;
			playerp->accs[6] -= rocketBootsFeetEffect*rby;
			playerp->accs[3] += rocketBootsFeetEffect*rbx;
			playerp->accs[7] += rocketBootsFeetEffect*rbx;
			for (int leg=0; leg<2; leg++)
			{
				if (leg==1 && (((int)(playerp->comm)&0x02) == 0x02))
					continue;
				int footX = playerp->legs[leg*8+4], footY = playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY, PT_PLSM);
				if (np>=0)
				{
					parts[np].vx = parts[i].vx+rby*25;
					parts[np].vy = parts[i].vy-rbx*25;
					parts[np].life += 30;
				}
 			}
 		}
	}

	//Go right
	if (((int)(playerp->comm)&0x02) == 0x02)
	{
		bool moved = false;
		if (dl<dr)
		{
			if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[4], playerp->legs[5])))
			{
				playerp->accs[2] = 3*gvy-3*gvx;
				playerp->accs[3] = -3*gvx-3*gvy;
				playerp->accs[0] = gvy;
				playerp->accs[1] = -gvx;
				moved = true;
			}
		}
		else
		{
			if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[12], playerp->legs[13])))
			{
				playerp->accs[6] = 3*gvy-3*gvx;
				playerp->accs[7] = -3*gvx-3*gvy;
				playerp->accs[0] = gvy;
				playerp->accs[1] = -gvx;
				moved = true;
			}
		}
		if (!moved && playerp->rocketBoots)
		{
			parts[i].vx += rocketBootsHeadEffect*rby;
			parts[i].vy -= rocketBootsHeadEffect*rbx;
			playerp->accs[2] += rocketBootsFeetEffect*rby;
			playerp->accs[6] += rocketBootsFeetEffect*rby;
			playerp->accs[3] -= rocketBootsFeetEffect*rbx;
			playerp->accs[7] -= rocketBootsFeetEffect*rbx;
			for (int leg=0; leg<2; leg++)
			{
				if (leg==0 && (((int)(playerp->comm)&0x01) == 0x01))
					continue;
				int footX = playerp->legs[leg*8+4], footY = playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY, PT_PLSM);
				if (np>=0)
				{
					parts[np].vx = parts[i].vx-rby*25;
					parts[np].vy = parts[i].vy+rbx*25;
					parts[np].life += 30;
				}
 			}
 		}
	}

	if (playerp->rocketBoots && ((int)(playerp->comm)&0x03) == 0x03)
	{
		// Pressing left and right simultaneously with rocket boots on slows the stickman down
		// Particularly useful in zero gravity
		parts[i].vx *= 0.5f;
		parts[i].vy *= 0.5f;
		playerp->accs[2] = playerp->accs[6] = 0;
		playerp->accs[3] = playerp->accs[7] = 0;
	}

	//Jump
	if (((int)(playerp->comm)&0x04) == 0x04)
 	{
		if (playerp->rocketBoots)
		{
			parts[i].vx -= rocketBootsHeadEffectV*rbx;
			parts[i].vy -= rocketBootsHeadEffectV*rby;
			playerp->accs[2] -= rocketBootsFeetEffectV*rbx;
			playerp->accs[6] -= rocketBootsFeetEffectV*rbx;
			playerp->accs[3] -= rocketBootsFeetEffectV*rby;
			playerp->accs[7] -= rocketBootsFeetEffectV*rby;
			for (int leg=0; leg<2; leg++)
			{
				int footX = playerp->legs[leg*8+4], footY = playerp->legs[leg*8+5];
				int np = sim->part_create(-1, footX, footY+1, PT_PLSM);
				if (np>=0)
				{
					parts[np].vx = parts[i].vx+rbx*30;
					parts[np].vy = parts[i].vy+rby*30;
					parts[np].life += 10;
				}
			}
		}
		else if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[4], playerp->legs[5])) || MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[12], playerp->legs[13])))
		{
			parts[i].vx -= 4*gvx;
			parts[i].vy -= 4*gvy;
			playerp->accs[2] -= gvx;
			playerp->accs[6] -= gvx;
			playerp->accs[3] -= gvy;
			playerp->accs[7] -= gvy;
		}
 	}

	//Charge detector wall if foot inside
	if (bmap[(int)(playerp->legs[5]+0.5)/CELL][(int)(playerp->legs[4]+0.5)/CELL]==WL_DETECT)
		set_emap((int)playerp->legs[4]/CELL, (int)playerp->legs[5]/CELL);
	if (bmap[(int)(playerp->legs[13]+0.5)/CELL][(int)(playerp->legs[12]+0.5)/CELL]==WL_DETECT)
		set_emap((int)(playerp->legs[12]+0.5)/CELL, (int)(playerp->legs[13]+0.5)/CELL);

	//Searching for particles near head
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				if (bmap[(ry+y)/CELL][(rx+x)/CELL]==WL_FAN)
					playerp->elem = SPC_AIR;
				else if (bmap[(ry+y)/CELL][(rx+x)/CELL]==WL_EHOLE)
					playerp->rocketBoots = false;
				else if (bmap[(ry+y)/CELL][(rx+x)/CELL]==WL_GRAV)
					playerp->rocketBoots = true;
				FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if (sim->elements[rt].Falldown!=0 || sim->elements[rt].State == ST_GAS
							|| (sim->elements[rt].Properties&TYPE_GAS)
							|| (sim->elements[rt].Properties&TYPE_LIQUID)
							|| rt == PT_NEUT || rt == PT_PHOT)
					{
						if (!playerp->rocketBoots || rt!=PT_PLSM)
							playerp->elem = rt;  //Current element
					}
					if (rt==PT_TESC || rt==PT_LIGH)
						playerp->elem = PT_LIGH;
					if (rt == PT_PLNT && parts[i].life<100) //Plant gives him 5 HP
					{
						if (parts[i].life<=95)
							parts[i].life += 5;
						else
							parts[i].life = 100;
						sim->part_kill(ri);
					}

					if (rt == PT_NEUT)
					{
						if (parts[i].life<=100) parts[i].life -= (102-parts[i].life)/2;
						else parts[i].life *= 0.9f;
						sim->part_kill(ri);
					}
					if (rt==PT_PRTI)
						STKM_interact(sim, playerp, i, rx, ry);
					if (!parts[i].type)//STKM_interact may kill STKM
						return 1;
				}
			}

	//Head position
	rx = x + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01);
	ry = y - 3*(playerp->pcomm == 0);

	//Spawn
	if (((int)(playerp->comm)&0x08) == 0x08)
	{
		ry -= 2*sim->rng.randInt<0,1>()+1;
		bool solidFound = false;
		FOR_PMAP_POSITION_NOENERGY(sim, rx, ry, rcount, ri, rnext)
		{
			if (sim->elements[parts[ri].type].State == ST_SOLID)
			{
				solidFound = true;
				break;
			}
		}
		if (solidFound)
		{
			sim->spark_position_conductiveOnly(rx, ry);
		}
		else
		{
			int np = -1;
			if (playerp->elem == SPC_AIR)
				create_parts(rx + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01), ry, 4, 4, SPC_AIR, 0, 1);
			else if (playerp->elem==PT_LIGH && playerp->elemDelayFrames>0)//limit lightning creation rate
				np = -1;
			else if (playerp->elem == PT_PHOT && sim->rng.chance<1,3>())
				np = -1;
			else
				np = sim->part_create(-1, rx, ry, playerp->elem);
			if (np>=0)
			{
				if (playerp->elem == PT_PHOT)
				{
					parts[np].vy = 0;
					if (((int)playerp->pcomm)&(0x01|0x02))
						parts[np].vx = (((((int)playerp->pcomm)&0x02) == 0x02) - (((int)(playerp->pcomm)&0x01) == 0x01))*3;
					else
						parts[np].vx = 3;
				}
				else if (playerp->elem == PT_LIGH)
				{
					float angle;
					int power = 100;
					if (gvx!=0 || gvy!=0)
						angle = atan2(gvx, gvy)*180.0f/M_PI;
					else
						angle = sim->rng.randInt<0,359>();
					if (((int)playerp->pcomm)&0x01)
						angle += 180;
					if (angle>360)
						angle-=360;
					if (angle<0)
						angle+=360;
					parts[np].tmp = angle;
					parts[np].life=sim->rng.randInt(0,1+power/15)+power/7;
					parts[np].temp=parts[np].life*power/2.5;
					parts[np].tmp2=1;
					playerp->elemDelayFrames = 30;
				}
				else if (playerp->elem != SPC_AIR)
				{
					parts[np].vx -= -gvy*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[np].vy -= gvx*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01));
					parts[i].vx -= (sim->elements[(int)playerp->elem].Weight*parts[np].vx)/1000;
				}
			}

		}
	}

	//Simulation of joints
	d = 25/(pow((playerp->legs[0]-playerp->legs[4]), 2) + pow((playerp->legs[1]-playerp->legs[5]), 2)+25) - 0.5;  //Fast distance
	playerp->legs[4] -= (playerp->legs[0]-playerp->legs[4])*d;
	playerp->legs[5] -= (playerp->legs[1]-playerp->legs[5])*d;
	playerp->legs[0] += (playerp->legs[0]-playerp->legs[4])*d;
	playerp->legs[1] += (playerp->legs[1]-playerp->legs[5])*d;

	d = 25/(pow((playerp->legs[8]-playerp->legs[12]), 2) + pow((playerp->legs[9]-playerp->legs[13]), 2)+25) - 0.5;
	playerp->legs[12] -= (playerp->legs[8]-playerp->legs[12])*d;
	playerp->legs[13] -= (playerp->legs[9]-playerp->legs[13])*d;
	playerp->legs[8] += (playerp->legs[8]-playerp->legs[12])*d;
	playerp->legs[9] += (playerp->legs[9]-playerp->legs[13])*d;

	d = 36/(pow((playerp->legs[0]-parts[i].x), 2) + pow((playerp->legs[1]-parts[i].y), 2)+36) - 0.5;
	parts[i].vx -= (playerp->legs[0]-parts[i].x)*d;
	parts[i].vy -= (playerp->legs[1]-parts[i].y)*d;
	playerp->legs[0] += (playerp->legs[0]-parts[i].x)*d;
	playerp->legs[1] += (playerp->legs[1]-parts[i].y)*d;

	d = 36/(pow((playerp->legs[8]-parts[i].x), 2) + pow((playerp->legs[9]-parts[i].y), 2)+36) - 0.5;
	parts[i].vx -= (playerp->legs[8]-parts[i].x)*d;
	parts[i].vy -= (playerp->legs[9]-parts[i].y)*d;
	playerp->legs[8] += (playerp->legs[8]-parts[i].x)*d;
	playerp->legs[9] += (playerp->legs[9]-parts[i].y)*d;

	if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[4], playerp->legs[5])))
	{
		playerp->legs[4] = playerp->legs[6];
		playerp->legs[5] = playerp->legs[7];
	}

	if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[12], playerp->legs[13])))
	{
		playerp->legs[12] = playerp->legs[14];
		playerp->legs[13] = playerp->legs[15];
	}

	//This makes stick man "pop" from obstacles
	if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[4], playerp->legs[5])))
	{
		float t;
		t = playerp->legs[4]; playerp->legs[4] = playerp->legs[6]; playerp->legs[6] = t;
		t = playerp->legs[5]; playerp->legs[5] = playerp->legs[7]; playerp->legs[7] = t;
	}

	if (MoveResult::WillBlock(sim->part_canMove(t, playerp->legs[12], playerp->legs[13])))
	{
		float t;
		t = playerp->legs[12]; playerp->legs[12] = playerp->legs[14]; playerp->legs[14] = t;
		t = playerp->legs[13]; playerp->legs[13] = playerp->legs[15]; playerp->legs[15] = t;
	}

	//Keeping legs distance
	if ((pow((playerp->legs[4] - playerp->legs[12]), 2) + pow((playerp->legs[5]-playerp->legs[13]), 2))<16)
	{
		float tvx, tvy;
		tvx = -gvy;
		tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[2] -= 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[3] -= 0.2*tvy/hypot(tvx, tvy);

			playerp->accs[6] += 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[7] += 0.2*tvy/hypot(tvx, tvy);
		}
	}

	if ((pow((playerp->legs[0] - playerp->legs[8]), 2) + pow((playerp->legs[1]-playerp->legs[9]), 2))<16)
	{
		float tvx, tvy;
		tvx = -gvy;
		tvy = gvx;

		if (tvx || tvy)
		{
			playerp->accs[0] -= 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[1] -= 0.2*tvy/hypot(tvx, tvy);

			playerp->accs[4] += 0.2*tvx/hypot(tvx, tvy);
			playerp->accs[5] += 0.2*tvy/hypot(tvx, tvy);
		}
	}

	//If legs touch something
	STKM_interact(sim, playerp, i, (int)(playerp->legs[4]+0.5), (int)(playerp->legs[5]+0.5));
	STKM_interact(sim, playerp, i, (int)(playerp->legs[12]+0.5), (int)(playerp->legs[13]+0.5));
	STKM_interact(sim, playerp, i, (int)(playerp->legs[4]+0.5), (int)playerp->legs[5]);
	STKM_interact(sim, playerp, i, (int)(playerp->legs[12]+0.5), (int)playerp->legs[13]);
	if (!parts[i].type)
		return 1;

	parts[i].ctype = playerp->elem;
	return 0;
}

void STKM_interact(Simulation *sim, Stickman_data* playerp, int i, int x, int y)
{
	int rt;
	int rcount, ri, rnext;
	if (x<0 || y<0 || x>=XRES || y>=YRES || !parts[i].type)
		return;
	FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
	{
		rt = parts[ri].type;

		if (rt==PT_SPRK && playerp->elem!=PT_LIGH) //If on charge
		{
			parts[i].life -= sim->rng.randInt<32,32+19>();
		}

		if (sim->elements[rt].HeatConduct && (rt!=PT_HSWC||sim->parts[ri].life==10) && ((playerp->elem!=PT_LIGH && parts[ri].temp>=323) || parts[ri].temp<=243) && (!playerp->rocketBoots || rt!=PT_PLSM))
		{
			parts[i].life -= 2;
			playerp->accs[3] -= 1;
		}
			
		if (sim->elements[rt].Properties&PROP_DEADLY)
			switch (rt)
			{
				case PT_ACID:
					parts[i].life -= 5;
					break;
				default:
					parts[i].life -= 1;
			}

		if (sim->elements[rt].Properties&PROP_RADIOACTIVE)
			parts[i].life -= 1;

		if (rt==PT_PRTI && parts[i].type)
		{
			int t = parts[i].type;
			int tmp = parts[i].tmp;
			PortalChannel *channel = sim->elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(sim->parts[ri]);
			if (channel->StoreParticle(sim, i, 1))//slot=1 gives rx=0, ry=1 in PRTO_update
				return;
		}

		if ((rt==PT_BHOL || rt==PT_NBHL) && parts[i].type)
		{
			if (!legacy_enable)
			{
				sim->part_add_temp(parts[ri], parts[i].temp/2);
			}
			sim->part_kill(i);
			return;
		}
		if ((rt==PT_VOID || (rt==PT_PVOD && parts[ri].life==10)) && (!parts[ri].ctype || (parts[ri].ctype==parts[i].type)!=(parts[ri].tmp&1)) && parts[i].type)
		{
			sim->part_kill(i);
			return;
		}
	}
}

bool STKM_create_allowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)
{
	return sim->elemData<STKM_ElemDataSim>(PT_STKM)->create_allowed();
}

void STKM_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->part_create(-3, x, y, PT_SPAWN);
}

void STKM_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	STKM_ElemDataSim *ed = sim->elemData<STKM_ElemDataSim>(PT_STKM);
	if (to==PT_STKM)
		ed->on_part_create(parts[i]);
	else
		ed->on_part_kill(parts[i]);
}

void STKM_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_STKM";
	elem->ui->Name = "STKM";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->DefaultProperties.temp = R_TEMP+14.6f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Stickman. Don't kill him! Control with the arrow keys.";

	elem->State = ST_NONE;
	elem->Properties = PROP_NOCTYPEDRAW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->DefaultProperties.life = 100;

	elem->Update = &STKM_update;
	elem->Graphics = &STKM_graphics;
	elem->Func_Create_Allowed = &STKM_create_allowed;
	elem->Func_Create = &STKM_create;
	elem->Func_ChangeType = &STKM_ChangeType;
	elem->Func_SimInit = &SimInit_createElemData<STKM_ElemDataSim>;
}


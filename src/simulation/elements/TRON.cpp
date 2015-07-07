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

/* TRON element is meant to resemble a tron bike (or worm) moving around and trying to avoid obstacles itself.
 * It has four direction each turn to choose from, 0 (left) 1 (up) 2 (right) 3 (down).
 * Each turn has a small random chance to randomly turn one way (so it doesn't do the exact same thing in a large room)
 * If the place it wants to move isn't a barrier, it will try and 'see' infront of itself to determine its safety.
 * For now the tron can only see its own body length in pixels ahead of itself (and around corners)
 *  - - - - - - - - - -
 *  - - - - + - - - - -
 *  - - - + + + - - - -
 *  - - +<--+-->+ - - -
 *  - +<----+---->+ - -
 *  - - - - H - - - - -
 * Where H is the head with tail length 4, it checks the + area to see if it can hit any of the edges, then it is called safe, or picks the biggest area if none safe.
 * .tmp bit values: 1st head, 2nd no tail growth, 3rd wait flag, 4th Nodie, 5th Dying, 6th & 7th is direction, 8th - 16th hue
 * .tmp2 is tail length (gets longer every few hundred frames)
 * .life is the timer that kills the end of the tail (the head uses life for how often it grows longer)
 * .ctype Contains the colour, lost on save, regenerated using hue tmp (bits 7 - 16)
 */
 
#define TRON_HEAD 1
#define TRON_NOGROW 2
#define TRON_WAIT 4 //it was just created, so WAIT a frame
#define TRON_NODIE 8
#define TRON_DEATH 16 //Crashed, now dying
#define TRON_NORANDOM 65536

const int tron_rx[4] = {-1, 0, 1, 0};
const int tron_ry[4] = { 0,-1, 0, 1};
unsigned int tron_colours[32];

bool canmovetron(Simulation * sim, int x, int y, int len)
{
	MoveResult::Code result=MoveResult::ALLOW, tmpResult;
	int rt;
	int rcount, ri, rnext;
	
	//TODO: maybe TRON should be able to cross some walls? E.g. detector wall
	if (sim->walls.type(SimPosI(x,y)))
		return false;

	FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
	{
		rt = parts[ri].type;
		if (rt==PT_SWCH && parts[ri].life>=10)
			tmpResult = MoveResult::ALLOW;
		else if (rt==PT_INVIS && parts[ri].tmp==1)
			tmpResult = MoveResult::ALLOW;
		else if ((((sim->elements[rt].Properties & PROP_LIFE_KILL_DEC) && sim->parts[ri].life > 0) || (sim->elements[rt].Properties & (PROP_LIFE_KILL|PROP_LIFE_DEC))==(PROP_LIFE_KILL|PROP_LIFE_DEC)) && parts[ri].life < len)
			tmpResult = MoveResult::ALLOW;
		else
		{
			tmpResult = sim->can_move[PT_TRON][parts[ri].type];
			if (tmpResult==MoveResult::DYNAMIC)
				tmpResult = sim->part_canMove_dynamic(PT_TRON, x, y, ri, tmpResult);
		}
		// Find the particle which restricts movement the most
		if (tmpResult<result)
			result = tmpResult;
	}
	return (result==MoveResult::ALLOW || result==MoveResult::ALLOW_BUT_SLOW);
}

int new_tronhead(Simulation *sim, int x, int y, int i, int direction)
{
	int np = sim->part_create(-1, x , y ,PT_TRON);
	if (np==-1)
		return -1;
	if (parts[i].life >= 100) // increase tail length
	{
		if (!(parts[i].tmp&TRON_NOGROW))
			parts[i].tmp2++;
		parts[i].life = 5;
	}
	//give new head our properties
	parts[np].tmp = 1 | (direction<<5) | (parts[i].tmp&(TRON_NOGROW|TRON_NODIE|TRON_NORANDOM)) | (parts[i].tmp&0xF800);
	if (np > i)
		parts[np].tmp |= TRON_WAIT;
	
	parts[np].ctype = parts[i].ctype;
	parts[np].tmp2 = parts[i].tmp2;
	parts[np].life = parts[i].life + 2;
	return 1;
}

int trymovetron(Simulation *sim, int x, int y, int dir, int i, int len)
{
	int k,j,rx,ry,tx,ty,count;
	count = 0;
	rx = x;
	ry = y;
	for (k = 1; k <= len; k ++)
	{
		rx += tron_rx[dir];
		ry += tron_ry[dir];
		if (ry > CELL && rx > CELL && ry < YRES-CELL && rx < XRES-CELL && canmovetron(sim, rx, ry, k-1))
		{
			count++;
			for (tx = rx - tron_ry[dir] , ty = ry - tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx-=tron_ry[dir],ty-=tron_rx[dir],j++)
			{
				if (ty > CELL && tx > CELL && ty < YRES-CELL && tx < XRES-CELL && canmovetron(sim, tx, ty, j+k-1))
				{
					if (j == (len-k))//there is a safe path, so we can break out
						return len+1;
					count++;
				}
				else //we hit a block so no need to check farther here
					break;
			}
			for (tx = rx + tron_ry[dir] , ty = ry + tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx+=tron_ry[dir],ty+=tron_rx[dir],j++)
			{
				if (ty > CELL && tx > CELL && ty < YRES-CELL && tx < XRES-CELL && canmovetron(sim, tx, ty, j+k-1))
				{
					if (j == (len-k))
						return len+1;
					count++;
				}
				else
					break;
			}
		}
		else //a block infront, no need to continue
			break;
	}
	return count;
}

int TRON_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp&TRON_WAIT)
	{
		parts[i].tmp &= ~TRON_WAIT;
		return 0;
	}
	if (parts[i].tmp&TRON_HEAD)
	{
		int firstdircheck = 0,seconddir=0,seconddircheck = 0,lastdir=0,lastdircheck = 0;
		int direction = (parts[i].tmp>>5 & 0x3);
		int originaldir = direction;

		//random turn
		int random = sim->rng.randInt<0,339>();
		if ((random==1 || random==3) && !(parts[i].tmp & TRON_NORANDOM))
		{
			//randomly turn left(3) or right(1)
			direction = (direction + random)%4;
		}
		
		//check infront
		//do sight check
		firstdircheck = trymovetron(sim, x,y,direction,i,parts[i].tmp2);
		if (firstdircheck < parts[i].tmp2)
		{
			if (parts[i].tmp & TRON_NORANDOM)
			{
				seconddir = (direction + 1)%4;
				lastdir = (direction + 3)%4;
			}
			else if (originaldir != direction) //if we just tried a random turn, don't pick random again
			{
				seconddir = originaldir;
				lastdir = (direction + 2)%4;
			}
			else
			{
				seconddir = (direction + (sim->rng.randInt<0,1>()*2)+1)% 4;
				lastdir = (seconddir + 2)%4;
			}
			seconddircheck = trymovetron(sim, x,y,seconddir,i,parts[i].tmp2);
			lastdircheck = trymovetron(sim, x,y,lastdir,i,parts[i].tmp2);
		}
		//find the best move
		if (seconddircheck > firstdircheck)
			direction = seconddir;
		if (lastdircheck > seconddircheck && lastdircheck > firstdircheck)
			direction = lastdir;
		//now try making new head, even if it fails
		if (new_tronhead(sim, x + tron_rx[direction],y + tron_ry[direction],i,direction) == -1)
		{
			//ohgod crash
			parts[i].tmp |= TRON_DEATH;
			//trigger tail death for TRON_NODIE, or is that mode even needed? just set a high tail length(but it still won't start dying when it crashes)
		}

		//set own life and clear .tmp (it dies if it can't move anyway)
		parts[i].life = parts[i].tmp2;
		parts[i].tmp &= parts[i].tmp&0xF818;
	}
	else // fade tail deco, or prevent tail from dying
	{
		if (parts[i].tmp&TRON_NODIE)
			parts[i].life++;
		//parts[i].dcolour =  tptmath::scale_to_clamped_byte((float)parts[i].life/(float)parts[i].tmp2,0,1.0f) << 24 |  parts[i].dcolour&0x00FFFFFF;
	}
	return 0;
}

int TRON_graphics(GRAPHICS_FUNC_ARGS)
{
	unsigned int col = tron_colours[(cpart->tmp&0xF800)>>11];
	if(cpart->tmp & TRON_HEAD)
		*pixel_mode |= PMODE_GLOW;
	*colr = (col & 0xFF0000)>>16;
	*colg = (col & 0x00FF00)>>8;
	*colb = (col & 0x0000FF);
	if(cpart->tmp & TRON_DEATH)
	{
		*pixel_mode |= FIRE_ADD | PMODE_FLARE;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*firea = 255;
	}
	if(cpart->life < cpart->tmp2 && !(cpart->tmp & TRON_HEAD))
	{
		*pixel_mode |= PMODE_BLEND;
		*pixel_mode &= ~PMODE_FLAT;
		*cola = (int)((((float)cpart->life)/((float)cpart->tmp2))*255.0f);
	}
	return 0;
}

// TODO:
void TRON_init_graphics()
{
	int i;
	int r, g, b;
	for (i=0; i<32; i++)
	{
		HSV_to_RGB(i<<4,255,255,&r,&g,&b);
		tron_colours[i] = r<<16 | g<<8 | b;
	}
}

void TRON_create(ELEMENT_CREATE_FUNC_ARGS)
{
	int randhue = sim->rng.randInt<0,359>();
	int randomdir = sim->rng.randInt<0,3>();
	sim->parts[i].tmp = 1|(randomdir<<5)|(randhue<<7);//set as a head and a direction
	sim->parts[i].tmp2 = 4;//tail
	sim->parts[i].life = 5;
}

void TRON_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_TRON";
	elem->ui->Name = "TRON";
	elem->Colour = COLPACK(0xA9FF00);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_SPECIAL;
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
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 0.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->ui->Description = "Smart particles, Travels in straight lines and avoids obstacles. Grows with time.";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_LIFE_KILL;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &TRON_update;
	elem->Graphics = &TRON_graphics;
	elem->Func_Create = &TRON_create;
}


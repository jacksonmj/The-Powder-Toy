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

#define PISTON_INACTIVE		0x00
#define PISTON_RETRACT		0x01
#define PISTON_EXTEND		0x02
#define MAX_FRAME			0x0F
#define DEFAULT_LIMIT		0x1F
#define DEFAULT_ARM_LIMIT	0xFF

// Returns the number of pixels by which a piston can move a stack of particles
// (the amount by which the PSTN moves, which if extending is not necessarily the amount by which all other particles move, since spaces are collapsed).
// Stack starts at (stackX, stackY) and moves in direction (directionX, directionY)
// Particles of type (block) block further movement
int PSTN_CanMoveStack(Simulation * sim, int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block, int *spacesRet=NULL)
{
	int posX, posY, spaces = 0, currentPos = 0;
	if (amount <= 0)
	{
		if (spacesRet)
			*spacesRet = 0;
		return 0;
	}
	for(posX = stackX, posY = stackY; currentPos < maxSize + amount && currentPos < XRES-1; posX += directionX, posY += directionY) {
		if (!(posX < XRES && posY < YRES && posX >= 0 && posY >= 0)) {
			break;
		}
		if (sim->IsWallBlocking(posX, posY, 0) || (block && sim->pmap_find_one(posX, posY, block)>=0))
		{
			if (spacesRet)
				*spacesRet = spaces;
			return spaces;
		}
		if(!sim->pmap[posY][posX].count_notEnergy) {
			spaces++;
			currentPos++;
			if(spaces >= amount)
				break;
		} else {
			if(spaces < maxSize && currentPos < maxSize && (!retract || (sim->pmap_find_one(posX, posY, PT_FRME)>=0 && posX == stackX && posY == stackY)))
				currentPos++;
			else
			{
				if (spacesRet)
					*spacesRet = spaces;
				return spaces;
			}
		}
	}
	if (spacesRet)
		*spacesRet = spaces;
	if (spaces)
		return currentPos;
	else
		return 0;
}

int PSTN_MoveStack(Simulation * sim, int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block, bool sticky, int callDepth = 0)
{
	int posX, posY, currentPos = 0;
	if(!callDepth && sim->pmap_find_one(stackX, stackY, PT_FRME)>=0) {
		int newY = !!directionX, newX = !!directionY;
		int realDirectionX = retract?-directionX:directionX;
		int realDirectionY = retract?-directionY:directionY;
		int maxRight = MAX_FRAME, maxLeft = MAX_FRAME;

		//check if we can push all the FRME
		for(int c = retract; c < MAX_FRAME; c++) {
			posY = stackY + (c*newY);
			posX = stackX + (c*newX);
			if (posX < XRES && posY < YRES && posX >= 0 && posY >= 0 && sim->pmap_find_one(posX, posY, PT_FRME)>=0) {
				int val = PSTN_CanMoveStack(sim, posX, posY, realDirectionX, realDirectionY, maxSize, amount, retract, block);
				if(val < amount)
					amount = val;
			} else {
				maxRight = c;
				break;
			}
		}
		for(int c = 1; c < MAX_FRAME; c++) {
			posY = stackY - (c*newY);
			posX = stackX - (c*newX);
			if (posX < XRES && posY < YRES && posX >= 0 && posY >= 0 && sim->pmap_find_one(posX, posY, PT_FRME)>=0) {
				int val = PSTN_CanMoveStack(sim, posX, posY, realDirectionX, realDirectionY, maxSize, amount, retract, block);
				if(val < amount)
					amount = val;
			} else {
				maxLeft = c;
				break;
			}
		}

		//If the piston is pushing frame, iterate out from the centre to the edge and push everything resting on frame
		for(int c = 1; c < maxRight; c++) {
			posY = stackY + (c*newY);
			posX = stackX + (c*newX);
			PSTN_MoveStack(sim, posX, posY, directionX, directionY, maxSize, amount, retract, block, !sim->parts[sim->pmap_find_one(posX, posY, PT_FRME)].tmp, 1);
		}
		for(int c = 1; c < maxLeft; c++) {
			posY = stackY - (c*newY);
			posX = stackX - (c*newX);
			PSTN_MoveStack(sim, posX, posY, directionX, directionY, maxSize, amount, retract, block, !sim->parts[sim->pmap_find_one(posX, posY, PT_FRME)].tmp, 1);
		}

		//Remove arm section if retracting with FRME
		if (retract)
			for(int j = 1; j <= amount; j++)
				sim->delete_position_notEnergy(stackX+(directionX*-j), stackY+(directionY*-j), PT_PSTN);
		return PSTN_MoveStack(sim, stackX, stackY, directionX, directionY, maxSize, amount, retract, block, !sim->parts[sim->pmap_find_one(stackX, stackY, PT_FRME)].tmp, 1);
	}
	if(retract){
		//Remove arm section if retracting without FRME
		if (!callDepth)
			for(int j = 1; j <= amount; j++)
				sim->delete_position_notEnergy(stackX+(directionX*-j), stackY+(directionY*-j), PT_PSTN);
		for(posX = stackX, posY = stackY; currentPos < maxSize && currentPos < XRES-1; posX += directionX, posY += directionY) {
			if (!(posX < XRES && posY < YRES && posX >= 0 && posY >= 0)) {
				break;
			}
			if(!sim->pmap[posY][posX].count_notEnergy || (block && sim->pmap_find_one(posX, posY, block)>=0) || (!sticky && sim->pmap_find_one(posX, posY, PT_FRME)<0)) {
				break;
			}
			int rcount, ri, rnext;
			int destPosX = posX-directionX*amount, destPosY = posY-directionY*amount;
			FOR_PMAP_POSITION_NOENERGY(sim, posX, posY, rcount, ri, rnext)
			{
				sim->part_set_pos(ri, posX, posY, destPosX, destPosY);
			}
		}
		return amount;
	} else {
		int spaces;
		currentPos = PSTN_CanMoveStack(sim, stackX, stackY, directionX, directionY, maxSize, amount, retract, block, &spaces);
		if (currentPos)
		{
			int destPos = currentPos-1;
			int srcPos = destPos-1;
			int srcPosX, srcPosY;
			int destPosX, destPosY;
			int rcount, ri, rnext;
			for (; srcPos>=0; srcPos--)
			{
				srcPosX = stackX + directionX*srcPos;
				srcPosY = stackY + directionY*srcPos;
				if (sim->pmap[srcPosY][srcPosX].count_notEnergy)
				{
					destPosX = stackX + directionX*destPos;
					destPosY = stackY + directionY*destPos;
					FOR_PMAP_POSITION_NOENERGY(sim, srcPosX, srcPosY, rcount, ri, rnext)
					{
						sim->part_set_pos(ri, srcPosX, srcPosY, destPosX, destPosY);
					}
					destPos--;
				}
			}
			return spaces;
		}
	}
	return 0;
}

int PSTN_update(UPDATE_FUNC_ARGS)
{
	if(parts[i].life)
		return 0;
	int maxSize = parts[i].tmp ? parts[i].tmp : DEFAULT_LIMIT;
	int armLimit = parts[i].tmp2 ? parts[i].tmp2 : DEFAULT_ARM_LIMIT;
	int state = PISTON_INACTIVE;
	int nxx, nyy, nxi, nyi, rx, ry;
	int directionX = 0, directionY = 0;
	if (state == PISTON_INACTIVE) {
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry) && (!rx || !ry))
				{
					int rcount, ri, rnext;
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type==PT_SPRK && parts[ri].life==3) {
							if(parts[ri].ctype == PT_PSCN)
								state = PISTON_EXTEND;
							else
								state = PISTON_RETRACT;
						}
					}
				}
	}
	if(state == PISTON_EXTEND || state == PISTON_RETRACT) {
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry) && (!rx || !ry))
				{
					if (sim->pmap_find_one(x+rx, y+ry, PT_PSTN) < 0)
						continue;

					bool movedPiston = false;
					bool foundEnd = false;
					int pistonEndX, pistonEndY;
					int pistonCount = -1;// number of PSTN particles minus 1
					int newSpace = 0;
					int armCount = 0;
					directionX = rx;
					directionY = ry;
					for (nxx = 0, nyy = 0, nxi = directionX, nyi = directionY; ; nyy += nyi, nxx += nxi) {
						if (!(x+nxx<XRES && y+nyy<YRES && x+nxx >= 0 && y+nyy >= 0)) {
							break;
						}
						int rcount, ri, rnext;
						bool foundPSTN = false;
						FOR_PMAP_POSITION_NOENERGY(sim, x+nxx, y+nyy, rcount, ri, rnext)
						{
							if (parts[ri].type != PT_PSTN)
								continue;
							foundPSTN = true;
							if(parts[ri].life)
								armCount++;
							else if (armCount)
							{
								pistonEndX = x+nxx;
								pistonEndY = y+nyy;
								foundEnd = true;
								break;
							}
							else
							{
								if (parts[ri].temp>283.15)
									pistonCount += (int)((parts[ri].temp-268.15)/10);// how many tens of degrees above 0 C, rounded to nearest 10
								else
									pistonCount++;
							}
						}
						if (foundEnd)
							break;
						if (!foundPSTN)
						{
							pistonEndX = x+nxx;
							pistonEndY = y+nyy;
							foundEnd = true;
							break;
						}
					}
					if(foundEnd) {
						if(state == PISTON_EXTEND) {
							if(armCount+pistonCount > armLimit)
								pistonCount = armLimit-armCount;
							if(pistonCount > 0) {
								newSpace = PSTN_MoveStack(sim, pistonEndX, pistonEndY, directionX, directionY, maxSize, pistonCount, false, parts[i].ctype, true);
								if(newSpace) {
									//Create new piston section
									for(int j = 0; j < newSpace; j++) {
										int nr = sim->part_create(-3, pistonEndX+(nxi*j), pistonEndY+(nyi*j), PT_PSTN);
										if (nr >= 0) {
											parts[nr].life = 1;
											if (parts[i].dcolour)
											{
												int colour=parts[i].dcolour;
												parts[nr].dcolour=(colour&0xFF000000)|std::max((colour&0xFF0000)-0x3C0000,0)|std::max((colour&0xFF00)-0x3C00,0)|std::max((colour&0xFF)-0x3C,0);
											}
										}
									}
									movedPiston =  true;
								}
							}
						} else if(state == PISTON_RETRACT) {
							if(pistonCount > armCount)
								pistonCount = armCount;
							if(armCount) {
								PSTN_MoveStack(sim, pistonEndX, pistonEndY, directionX, directionY, maxSize, pistonCount, true, parts[i].ctype, true);
								movedPiston = true;
							}
						}
					}
					if (movedPiston)
						return 0;
				}

	}
	return 0;
}

int PSTN_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life)
	{
		*colr -= 60;
		*colg -= 60;
	}
	return 0;
}

void PSTN_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_PSTN";
	elem->ui->Name = "PSTN";
	elem->Colour = COLPACK(0xAA9999);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_FORCE;
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

	elem->DefaultProperties.temp = 283.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Piston, extends and pushes particles.";

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

	elem->Update = &PSTN_update;
	elem->Graphics = &PSTN_graphics;
}

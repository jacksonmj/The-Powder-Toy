/**
 * Powder Toy - saving and loading functions
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

#include <bzlib.h>
#include <math.h>
#include "defines.h"
#include "powder.h"
#include "save.h"
#include "gravity.h"
#include "BSON.h"
#include "hmap.h"

#include "common/tptmath.h"
#include "simulation/Simulation.h"
#include "simulation/elements/FIGH.h"
#include "simulation/elements/LIFE.hpp"
#include "simulation/elements/STKM.h"
#include "simulation/SimulationSharedData.h"
#include "simulation/walls/WallTypes.hpp"

unsigned pmap_save[YRES][XRES];// TODO: remove

//Pop
pixel *prerender_save(void *save, int size, int *width, int *height)
{
	unsigned char * saveData = (unsigned char*)save;
	if (size<16)
	{
		return NULL;
	}
	if(saveData[0] == 'O' && saveData[1] == 'P' && saveData[2] == 'S')
	{
		return prerender_save_OPS(save, size, width, height);
	}
	else if((saveData[0]==0x66 && saveData[1]==0x75 && saveData[2]==0x43) || (saveData[0]==0x50 && saveData[1]==0x53 && saveData[2]==0x76))
	{
		return prerender_save_PSv(save, size, width, height);
	}
	return NULL;
}

void *build_save(int *size, int orig_x0, int orig_y0, int orig_w, int orig_h, const_WallsDataP wallsData, Signs &signs, void* partsptr)
{
	return build_save_OPS(size, orig_x0, orig_y0, orig_w, orig_h, wallsData, signs, partsptr);
}

int parse_save(void *save, int size, int replace, int x0, int y0, WallsDataP wallsData, Signs &signs, void* partsptr, unsigned pmap[YRES][XRES])
{
	unsigned char * saveData = (unsigned char*)save;
	if (!pmap)
		pmap = pmap_save;
	int result = 1;
	if (size<16)
	{
		return 1;
	}
	ppip_changed = 1;
	if(saveData[0] == 'O' && saveData[1] == 'P' && saveData[2] == 'S')
	{
		result = parse_save_OPS(save, size, replace, x0, y0, wallsData, signs, partsptr, pmap);
	}
	else if((saveData[0]==0x66 && saveData[1]==0x75 && saveData[2]==0x43) || (saveData[0]==0x50 && saveData[1]==0x53 && saveData[2]==0x76))
	{
		result = parse_save_PSv(save, size, replace, x0, y0, wallsData, signs, partsptr, pmap);
	}
	globalSim->recalc_pmap();
	globalSim->recalc_elementCount();
	globalSim->recalc_freeParticles();
	globalSim->queueStackingCheck();
	return result;
}

pixel *prerender_save_OPS(void *save, int size, int *width, int *height)
{
	unsigned char * inputData = (unsigned char*)save, *bsonData = NULL, *partsData = NULL, *partsPosData = NULL, *wallData = NULL;
	int inputDataLen = size, bsonDataLen = 0, partsDataLen, partsPosDataLen, wallDataLen;
	int i, x, y, j;
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int bsonInitialised = 0;
	pixel * vidBuf = NULL;
	bson b;
	bson_iterator iter;

	//Block sizes
	blockX = 0;
	blockY = 0;
	blockW = inputData[6];
	blockH = inputData[7];
	
	//Full size, normalised
	fullX = 0;
	fullY = 0;
	fullW = blockW*CELL;
	fullH = blockH*CELL;
	
	//
	*width = fullW;
	*height = fullH;
	
	//From newer version
	if(inputData[4] > SAVE_VERSION)
	{
		fprintf(stderr, "Save from newer version\n");
		goto fail;
	}
		
	//Incompatible cell size
	if(inputData[5] > CELL)
	{
		fprintf(stderr, "Cell size mismatch\n");
		goto fail;
	}
		
	//Too large/off screen
	if(blockX+blockW > XRES/CELL || blockY+blockH > YRES/CELL)
	{
		fprintf(stderr, "Save too large\n");
		goto fail;
	}
	
	bsonDataLen = ((unsigned)inputData[8]);
	bsonDataLen |= ((unsigned)inputData[9]) << 8;
	bsonDataLen |= ((unsigned)inputData[10]) << 16;
	bsonDataLen |= ((unsigned)inputData[11]) << 24;
	
	bsonData = (unsigned char*)malloc(bsonDataLen+1);
	if(!bsonData)
	{
		fprintf(stderr, "Internal error while parsing save: could not allocate buffer\n");
		goto fail;
	}
	//Make sure bsonData is null terminated, since all string functions need null terminated strings
	//(bson_iterator_key returns a pointer into bsonData, which is then used with strcmp)
	bsonData[bsonDataLen] = 0;
	
	if (BZ2_bzBuffToBuffDecompress((char*)bsonData, (unsigned int*)(&bsonDataLen), (char*)inputData+12, inputDataLen-12, 0, 0))
	{
		fprintf(stderr, "Unable to decompress\n");
		goto fail;
	}
	
	bson_init_data(&b, (char*)bsonData);
	bsonInitialised = 1;
	bson_iterator_init(&iter, &b);
	while(bson_iterator_next(&iter))
	{
		if(strcmp(bson_iterator_key(&iter), "parts")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		if(strcmp(bson_iterator_key(&iter), "partsPos")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsPosDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsPosData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle position data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "wallMap")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (wallDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				wallData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of wall data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
	}
	
	vidBuf = (pixel*)calloc(fullW*fullH, PIXELSIZE);
	
	//Read wall and fan data
	if(wallData)
	{
		if(blockW * blockH > wallDataLen)
		{
			fprintf(stderr, "Not enough wall data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				if(wallData[y*blockW+x])
				{
					// Not bothering to draw walls accurately because prerender_save will be removed at some point during the rewrite
					for (i=0; i<CELL; i++)
						for (j=0; j<CELL; j++)
							vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = PIXPACK(0x808080);
				}
			}
		}
	}
	
	//Read particle data
	if(partsData && partsPosData)
	{
		int fieldDescriptor;
		int posCount, posTotal, partsPosDataIndex = 0;
		int saved_x, saved_y;
		if(fullW * fullH * 3 > partsPosDataLen)
		{
			fprintf(stderr, "Not enough particle position data\n");
			goto fail;
		}
		i = 0;
		for (saved_y=0; saved_y<fullH; saved_y++)
		{
			for (saved_x=0; saved_x<fullW; saved_x++)
			{
				//Read total number of particles at this position
				posTotal = 0;
				posTotal |= partsPosData[partsPosDataIndex++]<<16;
				posTotal |= partsPosData[partsPosDataIndex++]<<8;
				posTotal |= partsPosData[partsPosDataIndex++];
				//Put the next posTotal particles at this position
				for (posCount=0; posCount<posTotal; posCount++)
				{
					//i+3 because we have 4 bytes of required fields (type (1), descriptor (2), temp (1))
					if (i+3 >= partsDataLen)
						goto fail;
					x = saved_x + fullX;
					y = saved_y + fullY;
					fieldDescriptor = partsData[i+1];
					fieldDescriptor |= partsData[i+2] << 8;
					if(x >= XRES || x < 0 || y >= YRES || y < 0)
					{
						fprintf(stderr, "Out of range [%d]: %d %d, [%d, %d], [%d, %d]\n", i, x, y, (unsigned)partsData[i+1], (unsigned)partsData[i+2], (unsigned)partsData[i+3], (unsigned)partsData[i+4]);
						goto fail;
					}
					if(!globalSim->element_isValid(partsData[i]))
						partsData[i] = PT_DMND;	//Replace all invalid elements with diamond
					
					//Draw type
					if (partsData[i]==PT_STKM || partsData[i]==PT_STKM2 || partsData[i]==PT_FIGH)
					{
						pixel lc, hc=PIXRGB(255, 224, 178);
						if (partsData[i]==PT_STKM || partsData[i]==PT_FIGH) lc = PIXRGB(255, 255, 255);
						else lc = PIXRGB(100, 100, 255);
						//only need to check upper bound of y coord - lower bounds and x<w are checked in draw_line
						if (partsData[i]==PT_STKM || partsData[i]==PT_STKM2)
						{
							draw_line(vidBuf, x-2, y-2, x+2, y-2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							if (y+2<*height)
							{
								draw_line(vidBuf, x-2, y+2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
								draw_line(vidBuf, x-2, y-2, x-2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
								draw_line(vidBuf, x+2, y-2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							}
						}
						else if (y+2<*height)
						{
							draw_line(vidBuf, x-2, y, x, y-2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x-2, y, x, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x, y-2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x, y+2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), *width);
						}
						if (y+6<*height)
						{
							draw_line(vidBuf, x, y+3, x-1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), *width);
							draw_line(vidBuf, x, y+3, x+1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), *width);
						}
						if (y+12<*height)
						{
							draw_line(vidBuf, x-1, y+6, x-3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), *width);
							draw_line(vidBuf, x+1, y+6, x+3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), *width);
						}
					}
					else
						vidBuf[(fullY+y)*fullW+(fullX+x)] = ARGBColour_TO_PIXEL(globalSim->elements[partsData[i]].Colour);
					i+=3; //Skip Type and Descriptor

					//Skip temp
					if(fieldDescriptor & 0x01)
					{
						i+=2;
					}
					else
					{
						i++;
					}
					
					//Skip life
					if(fieldDescriptor & 0x02)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x04)
						{
							if(i++ >= partsDataLen) goto fail;
						}
					}
					
					//Skip tmp
					if(fieldDescriptor & 0x08)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x10)
						{
							if(i++ >= partsDataLen) goto fail;
							if(fieldDescriptor & 0x1000)
							{
								if(i+1 >= partsDataLen) goto fail;
								i += 2;
							}
						}
					}
					
					//Skip ctype
					if(fieldDescriptor & 0x20)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x200)
						{
							if(i+2 >= partsDataLen) goto fail;
							i+=3;
						}
					}
					
					//Read dcolour
					if(fieldDescriptor & 0x40)
					{
						unsigned char r, g, b;
						if(i+3 >= partsDataLen) goto fail;
						i++;//Skip alpha
						r = partsData[i++];
						g = partsData[i++];
						b = partsData[i++];
						vidBuf[(fullY+y)*fullW+(fullX+x)] = PIXRGB(r, g, b);
					}
					
					//Skip vx
					if(fieldDescriptor & 0x80)
					{
						if(i++ >= partsDataLen) goto fail;
					}
					
					//Skip vy
					if(fieldDescriptor & 0x100)
					{
						if(i++ >= partsDataLen) goto fail;
					}

					//Skip tmp2
					if(fieldDescriptor & 0x400)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x800)
						{
							if(i++ >= partsDataLen) goto fail;
						}
					}

					//Skip pavg
					if(fieldDescriptor & 0x2000)
					{
						i += 4;
						if(i > partsDataLen) goto fail;
					}
				}
			}
		}
	}
	goto fin;
fail:
	if(vidBuf)
	{
		free(vidBuf);
		vidBuf = NULL;
	}
fin:
	//Don't call bson_destroy if bson_init wasn't called, or an uninitialised pointer (b.data) will be freed and the game will crash
	if (bsonInitialised)
		bson_destroy(&b);
	return vidBuf;
}

void *build_save_OPS(int *size, int orig_x0, int orig_y0, int orig_w, int orig_h, const_WallsDataP wallsData, Signs &signs, void* o_partsptr)
{
	particle *partsptr = (particle*)o_partsptr;
	unsigned char *partsData = NULL, *partsPosData = NULL, *fanData = NULL, *wallData = NULL, *finalData = NULL, *outputData = NULL, *soapLinkData = NULL;
	unsigned *partsPosLink = NULL, *partsPosFirstMap = NULL, *partsPosCount = NULL, *partsPosLastMap = NULL;
	unsigned partsCount = 0, *partsSaveIndex = NULL;
	unsigned *elementCount = (unsigned*)calloc(PT_NUM, sizeof(unsigned));
	int partsDataLen, partsPosDataLen, fanDataLen, wallDataLen, finalDataLen, outputDataLen, soapLinkDataLen;
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int x, y, i, wallDataFound = 0;
	int posCount, signsCount;
	bson b;

	//Get coords in blocks
	blockX = orig_x0/CELL;
	blockY = orig_y0/CELL;

	//Snap full coords to block size
	fullX = blockX*CELL;
	fullY = blockY*CELL;

	//Original size + offset of original corner from snapped corner, rounded up by adding CELL-1
	blockW = (orig_w+orig_x0-fullX+CELL-1)/CELL;
	blockH = (orig_h+orig_y0-fullY+CELL-1)/CELL;
	fullW = blockW*CELL;
	fullH = blockH*CELL;

	SimAreaI origArea(SimPosI(orig_x0, orig_y0), SimPosDI(orig_w, orig_h));

	//Copy fan and wall data
	wallData = (unsigned char*)malloc(blockW*blockH);
	wallDataLen = blockW*blockH;
	fanData = (unsigned char*)malloc((blockW*blockH)*2);
	fanDataLen = 0;
	if (!wallData || !fanData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for(x = blockX; x < blockX+blockW; x++)
	{
		for(y = blockY; y < blockY+blockH; y++)
		{
			wallData[(y-blockY)*blockW+(x-blockX)] = wallsData.wallType[y][x];
			if(wallsData.wallType[y][x] && !wallDataFound)
				wallDataFound = 1;
			if(wallsData.wallType[y][x]==WL_FAN)
			{
				i = (int)(wallsData.fanVX[y][x]*64.0f+127.5f);
				if (i<0) i=0;
				if (i>255) i=255;
				fanData[fanDataLen++] = i;
				i = (int)(wallsData.fanVY[y][x]*64.0f+127.5f);
				if (i<0) i=0;
				if (i>255) i=255;
				fanData[fanDataLen++] = i;
			}
		}
	}
	if(!fanDataLen)
	{
		free(fanData);
		fanData = NULL;
	}
	if(!wallDataFound)
	{
		free(wallData);
		wallData = NULL;
	}
	
	//Index positions of all particles, using linked lists
	//partsPosFirstMap is pmap for the first particle in each position
	//partsPosLastMap is pmap for the last particle in each position
	//partsPosCount is the number of particles in each position
	//partsPosLink contains, for each particle, (i<<8)|1 of the next particle in the same position
	partsPosFirstMap = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosLastMap = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosCount = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosLink = (unsigned*)calloc(NPART, sizeof(unsigned));
	if (!partsPosFirstMap || !partsPosLastMap || !partsPosCount || !partsPosLink)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for(i = 0; i < NPART; i++)
	{
		if(partsptr[i].type)
		{
			x = (int)(partsptr[i].x+0.5f);
			y = (int)(partsptr[i].y+0.5f);
			if (x>=orig_x0 && x<orig_x0+orig_w && y>=orig_y0 && y<orig_y0+orig_h)
			{
				//Coordinates relative to top left corner of saved area
				x -= fullX;
				y -= fullY;
				if (!partsPosFirstMap[y*fullW + x])
				{
					//First entry in list
					partsPosFirstMap[y*fullW + x] = (i<<8)|1;
					partsPosLastMap[y*fullW + x] = (i<<8)|1;
				}
				else
				{
					//Add to end of list
					partsPosLink[partsPosLastMap[y*fullW + x]>>8] = (i<<8)|1;//link to current end of list
					partsPosLastMap[y*fullW + x] = (i<<8)|1;//set as new end of list
				}
				partsPosCount[y*fullW + x]++;
			}
		}
	}

	//Store number of particles in each position
	partsPosData = (unsigned char*)malloc(fullW*fullH*3);
	partsPosDataLen = 0;
	if (!partsPosData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for (y=0;y<fullH;y++)
	{
		for (x=0;x<fullW;x++)
		{
			posCount = partsPosCount[y*fullW + x];
			partsPosData[partsPosDataLen++] = (posCount&0x00FF0000)>>16;
			partsPosData[partsPosDataLen++] = (posCount&0x0000FF00)>>8;
			partsPosData[partsPosDataLen++] = (posCount&0x000000FF);
		}
	}

	//Copy parts data
	/* Field descriptor format:
	|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|
									|		pavg	|	tmp[3+4]	|		tmp2[2]	|		tmp2[1]	|	ctype[2]	|		vy		|		vx		|	dcololour	|	ctype[1]	|		tmp[2]	|		tmp[1]	|		life[2]	|		life[1]	|	temp dbl len|
	life[2] means a second byte (for a 16 bit field) if life[1] is present
	*/
	partsData = (unsigned char*)malloc(NPART * (sizeof(particle)+1));
	partsDataLen = 0;
	partsSaveIndex = (unsigned*)calloc(NPART, sizeof(unsigned));
	partsCount = 0;
	if (!partsData || !partsSaveIndex)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for (y=0;y<fullH;y++)
	{
		for (x=0;x<fullW;x++)
		{
			//Find the first particle in this position
			i = partsPosFirstMap[y*fullW + x];

			//Loop while there is a pmap entry
			while (i)
			{
				unsigned short fieldDesc = 0;
				int fieldDescLoc = 0, tempTemp, vTemp;
				
				//Turn pmap entry into a partsptr index
				i = i>>8;

				//Store saved particle index+1 for this partsptr index (0 means not saved)
				partsSaveIndex[i] = (partsCount++) + 1;

				//Type (required)
				partsData[partsDataLen++] = partsptr[i].type;
				elementCount[partsptr[i].type]++;
				
				//Location of the field descriptor
				fieldDescLoc = partsDataLen++;
				partsDataLen++;
				
				//Extra Temperature (2nd byte optional, 1st required), 1 to 2 bytes
				//Store temperature as an offset of 21C(294.15K) or go into a 16byte int and store the whole thing
				if(fabs(partsptr[i].temp-294.15f)<127)
				{
					tempTemp = floor(partsptr[i].temp-294.15f+0.5f);
					partsData[partsDataLen++] = tempTemp;
				}
				else
				{
					fieldDesc |= 1;
					tempTemp = (int)(partsptr[i].temp+0.5f);
					partsData[partsDataLen++] = tempTemp;
					partsData[partsDataLen++] = tempTemp >> 8;
				}
				
				//Life (optional), 1 to 2 bytes
				if(partsptr[i].life)
				{
					int life = partsptr[i].life;
					if (life > 0xFFFF)
						life = 0xFFFF;
					else if (life < 0)
						life = 0;
					fieldDesc |= 1 << 1;
					partsData[partsDataLen++] = life;
					if(life & 0xFF00)
					{
						fieldDesc |= 1 << 2;
						partsData[partsDataLen++] = life >> 8;
					}
				}
				
				//Tmp (optional), 1, 2 or 4 bytes
				if(partsptr[i].tmp)
				{
					fieldDesc |= 1 << 3;
					partsData[partsDataLen++] = partsptr[i].tmp;
					if(partsptr[i].tmp & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 4;
						partsData[partsDataLen++] = partsptr[i].tmp >> 8;
						if(partsptr[i].tmp & 0xFFFF0000)
						{
							fieldDesc |= 1 << 12;
							partsData[partsDataLen++] = (partsptr[i].tmp&0xFF000000)>>24;
							partsData[partsDataLen++] = (partsptr[i].tmp&0x00FF0000)>>16;
						}
					}
				}
				
				//Ctype (optional), 1 or 4 bytes
				if(partsptr[i].ctype)
				{
					fieldDesc |= 1 << 5;
					partsData[partsDataLen++] = partsptr[i].ctype;
					if(partsptr[i].ctype & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 9;
						partsData[partsDataLen++] = (partsptr[i].ctype&0xFF000000)>>24;
						partsData[partsDataLen++] = (partsptr[i].ctype&0x00FF0000)>>16;
						partsData[partsDataLen++] = (partsptr[i].ctype&0x0000FF00)>>8;
					}
				}
				
				//Dcolour (optional), 4 bytes
				if(partsptr[i].dcolour && (partsptr[i].dcolour & 0xFF000000))
				{
					fieldDesc |= 1 << 6;
					partsData[partsDataLen++] = (partsptr[i].dcolour&0xFF000000)>>24;
					partsData[partsDataLen++] = (partsptr[i].dcolour&0x00FF0000)>>16;
					partsData[partsDataLen++] = (partsptr[i].dcolour&0x0000FF00)>>8;
					partsData[partsDataLen++] = (partsptr[i].dcolour&0x000000FF);
				}
				
				//VX (optional), 1 byte
				if(fabs(partsptr[i].vx) > 0.001f)
				{
					fieldDesc |= 1 << 7;
					vTemp = (int)(partsptr[i].vx*16.0f+127.5f);
					if (vTemp<0) vTemp=0;
					if (vTemp>255) vTemp=255;
					partsData[partsDataLen++] = vTemp;
				}
				
				//VY (optional), 1 byte
				if(fabs(partsptr[i].vy) > 0.001f)
				{
					fieldDesc |= 1 << 8;
					vTemp = (int)(partsptr[i].vy*16.0f+127.5f);
					if (vTemp<0) vTemp=0;
					if (vTemp>255) vTemp=255;
					partsData[partsDataLen++] = vTemp;
				}

				//Tmp2 (optional), 1 or 2 bytes
				if(partsptr[i].tmp2)
				{
					fieldDesc |= 1 << 10;
					partsData[partsDataLen++] = partsptr[i].tmp2;
					if(partsptr[i].tmp2 & 0xFF00)
					{
						fieldDesc |= 1 << 11;
						partsData[partsDataLen++] = partsptr[i].tmp2 >> 8;
					}
				}

				//Pavg, 4 bytes
 				//Don't save pavg for things that break under pressure, because then they will break when the save is loaded, since pressure isn't also loaded
				if ((partsptr[i].pavg[0] || partsptr[i].pavg[1]) && !(partsptr[i].type == PT_QRTZ || partsptr[i].type == PT_GLAS || partsptr[i].type == PT_TUNG))
				{
					fieldDesc |= 1 << 13;
					partsData[partsDataLen++] = (int)partsptr[i].pavg[0];
					partsData[partsDataLen++] = ((int)partsptr[i].pavg[0])>>8;
					partsData[partsDataLen++] = (int)partsptr[i].pavg[1];
					partsData[partsDataLen++] = ((int)partsptr[i].pavg[1])>>8;
				}

				//Write the field descriptor
				partsData[fieldDescLoc] = fieldDesc;
				partsData[fieldDescLoc+1] = fieldDesc>>8;

				//Get the pmap entry for the next particle in the same position
				i = partsPosLink[i];
			}
		}
	}

	soapLinkData = (unsigned char*)malloc(3*elementCount[PT_SOAP]);
	soapLinkDataLen = 0;
	if (!soapLinkData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	//Iterate through particles in the same order that they were saved
	for (y=0;y<fullH;y++)
	{
		for (x=0;x<fullW;x++)
		{
			//Find the first particle in this position
			i = partsPosFirstMap[y*fullW + x];

			//Loop while there is a pmap entry
			while (i)
			{
				//Turn pmap entry into a partsptr index
				i = i>>8;

				if (partsptr[i].type==PT_SOAP)
				{
					//Only save forward link for each particle, back links can be deduced from other forward links
					//linkedIndex is index within saved particles + 1, 0 means not saved or no link
					unsigned linkedIndex = 0;
					if ((partsptr[i].ctype&2) && partsptr[i].tmp>=0 && partsptr[i].tmp<NPART)
					{
						linkedIndex = partsSaveIndex[partsptr[i].tmp];
					}
					soapLinkData[soapLinkDataLen++] = (linkedIndex&0xFF0000)>>16;
					soapLinkData[soapLinkDataLen++] = (linkedIndex&0x00FF00)>>8;
					soapLinkData[soapLinkDataLen++] = (linkedIndex&0x0000FF);
				}

				//Get the pmap entry for the next particle in the same position
				i = partsPosLink[i];
			}
		}
	}
	if(!soapLinkDataLen)
	{
		free(soapLinkData);
		soapLinkData = NULL;
	}
	if(!partsDataLen)
	{
		free(partsData);
		partsData = NULL;
	}
	
	bson_init(&b);
	bson_append_start_object(&b, "origin");
	bson_append_int(&b, "majorVersion", SAVE_VERSION);
	bson_append_int(&b, "minorVersion", MINOR_VERSION);
	bson_append_int(&b, "buildNum", BUILD_NUM);
	// TODO: 
	//bson_append_int(&b, "snapshotId", 0);
	//bson_append_string(&b, "releaseType", IDENT_RELTYPE);
	//bson_append_string(&b, "platform", IDENT_PLATFORM);
	//bson_append_string(&b, "builtType", IDENT_BUILD);
	bson_append_finish_object(&b);

	bson_append_bool(&b, "waterEEnabled", water_equal_test);
	bson_append_bool(&b, "legacyEnable", globalSim->option_heatMode()==HeatMode::Legacy ? 1 : 0);
	bson_append_bool(&b, "gravityEnable", ngrav_enable);
	bson_append_bool(&b, "aheat_enable", globalSim->ambientHeatEnabled);
	bson_append_bool(&b, "paused", sys_pause);
	bson_append_int(&b, "gravityMode", gravityMode);
	bson_append_int(&b, "airMode", globalSim->airMode);
	bson_append_int(&b, "edgeMode", globalSim->option_edgeMode());
	
	//bson_append_int(&b, "leftSelectedElement", sl);
	//bson_append_int(&b, "rightSelectedElement", sr);
	bson_append_int(&b, "activeMenu", active_menu);
	if(partsData)
		bson_append_binary(&b, "parts", BSON_BIN_USER, (char*)partsData, partsDataLen);
	if(partsPosData)
		bson_append_binary(&b, "partsPos", BSON_BIN_USER, (char*)partsPosData, partsPosDataLen);
	if(wallData)
		bson_append_binary(&b, "wallMap", BSON_BIN_USER, (char*)wallData, wallDataLen);
	if(fanData)
		bson_append_binary(&b, "fanMap", BSON_BIN_USER, (char*)fanData, fanDataLen);
	if(soapLinkData)
		bson_append_binary(&b, "soapLinks", BSON_BIN_USER, (char*)soapLinkData, soapLinkDataLen);
	signsCount = 0;
	for (const Sign &sign: globalSim->signs)
	{
		if (origArea.contains(sign.pos))
		{
			signsCount++;
		}
	}
	if(signsCount)
	{
		bson_append_start_array(&b, "signs");
		for (const Sign &sign: globalSim->signs)
		{
			std::string text = sign.getRawText();
			if(text.length() && origArea.contains(sign.pos))
			{
				bson_append_start_object(&b, "sign");
				bson_append_string(&b, "text", text.c_str());
				bson_append_int(&b, "justification", static_cast<int>(sign.justification));
				bson_append_int(&b, "x", sign.pos.x-fullX);
				bson_append_int(&b, "y", sign.pos.y-fullY);
				bson_append_finish_object(&b);
			}
		}
		bson_append_finish_array(&b);
	}
	bson_finish(&b);
	bson_print(&b);
	
	finalData = (unsigned char*)bson_data(&b);
	finalDataLen = bson_size(&b);
	outputDataLen = finalDataLen*2+12;
	outputData = (unsigned char*)malloc(outputDataLen);
	if (!outputData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}

	outputData[0] = 'O';
	outputData[1] = 'P';
	outputData[2] = 'S';
	outputData[3] = '1';
	outputData[4] = SAVE_VERSION;
	outputData[5] = CELL;
	outputData[6] = blockW;
	outputData[7] = blockH;
	outputData[8] = finalDataLen;
	outputData[9] = finalDataLen >> 8;
	outputData[10] = finalDataLen >> 16;
	outputData[11] = finalDataLen >> 24;
	
	if (BZ2_bzBuffToBuffCompress((char*)outputData+12, (unsigned*)(&outputDataLen), (char*)finalData, bson_size(&b), 9, 0, 0) != BZ_OK)
	{
		puts("Save Error\n");
		free(outputData);
		*size = 0;
		outputData = NULL;
		goto fin;
	}
	
	printf("compressed data: %d\n", outputDataLen);
	*size = outputDataLen + 12;
	
fin:
	bson_destroy(&b);
	if(partsData)
		free(partsData);
	if(wallData)
		free(wallData);
	if(fanData)
		free(fanData);
	if (elementCount)
		free(elementCount);
	if (partsSaveIndex)
		free(partsSaveIndex);
	if (soapLinkData)
		free(soapLinkData);
	
	return outputData;
}

int parse_save_OPS(void *save, int size, int replace, int x0, int y0, WallsDataP wallsData, Signs &signs, void* o_partsptr, unsigned pmap[YRES][XRES])
{
	particle *partsptr = (particle*)o_partsptr;
	unsigned char * inputData = (unsigned char*)save, *bsonData = NULL, *partsData = NULL, *partsPosData = NULL, *fanData = NULL, *wallData = NULL, *soapLinkData = NULL;
	int inputDataLen = size, bsonDataLen = 0, partsDataLen, partsPosDataLen, fanDataLen, wallDataLen, soapLinkDataLen;
	unsigned partsCount = 0, *partsSimIndex = NULL;
	int i, freeIndicesCount, x, y, returnCode = 0, j;
	int *freeIndices = NULL;
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int saved_version = inputData[4];
	bson b;
	bson_iterator iter;

	//Block sizes
	blockX = x0/CELL;
	blockY = y0/CELL;
	blockW = inputData[6];
	blockH = inputData[7];
	
	//Full size, normalised
	fullX = blockX*CELL;
	fullY = blockY*CELL;
	fullW = blockW*CELL;
	fullH = blockH*CELL;
	
	//From newer version
	if(saved_version > SAVE_VERSION)
	{
		fprintf(stderr, "Save from newer version\n");
		return 2;
	}
		
	//Incompatible cell size
	if(inputData[5] > CELL)
	{
		fprintf(stderr, "Cell size mismatch\n");
		return 1;
	}
		
	//Too large/off screen
	if(blockX+blockW > XRES/CELL || blockY+blockH > YRES/CELL)
	{
		fprintf(stderr, "Save too large\n");
		return 1;
	}
	
	bsonDataLen = ((unsigned)inputData[8]);
	bsonDataLen |= ((unsigned)inputData[9]) << 8;
	bsonDataLen |= ((unsigned)inputData[10]) << 16;
	bsonDataLen |= ((unsigned)inputData[11]) << 24;

	if (bsonDataLen<1 || bsonDataLen > 209715200-1)
	{
		fprintf(stderr, "Save data too large, refusing\n");
		return 3;
	}

	bsonData = (unsigned char*)malloc(bsonDataLen+1);
	if(!bsonData)
	{
		fprintf(stderr, "Internal error while parsing save: could not allocate buffer\n");
		return 3;
	}
	//Make sure bsonData is null terminated, since all string functions need null terminated strings
	//(bson_iterator_key returns a pointer into bsonData, which is then used with strcmp)
	bsonData[bsonDataLen] = 0;
	
	if (BZ2_bzBuffToBuffDecompress((char*)bsonData, (unsigned*)(&bsonDataLen), (char*)inputData+12, inputDataLen-12, 0, 0))
	{
		fprintf(stderr, "Unable to decompress\n");
		return 1;
	}
	
	if(replace)
	{
		//Remove everything
		clear_sim();
	}
	
	bson_init_data(&b, (char*)bsonData);
	bson_iterator_init(&iter, &b);
	while(bson_iterator_next(&iter))
	{
		if(strcmp(bson_iterator_key(&iter), "signs")==0)
		{
			if(bson_iterator_type(&iter)==BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while(bson_iterator_next(&subiter))
				{
					if(strcmp(bson_iterator_key(&subiter), "sign")==0)
					{
						if(bson_iterator_type(&subiter)==BSON_OBJECT)
						{
							bson_iterator signiter;
							bson_iterator_subiterator(&subiter, &signiter);
							//Find a free sign ID
							Sign *sign = globalSim->signs.add();
							//Stop reading signs if we have no free spaces
							if (!sign)
								break;
							while(bson_iterator_next(&signiter))
							{
								if(strcmp(bson_iterator_key(&signiter), "text")==0 && bson_iterator_type(&signiter)==BSON_STRING)
								{
									sign->setRawText(bson_iterator_string(&signiter));
								}
								else if(strcmp(bson_iterator_key(&signiter), "justification")==0 && bson_iterator_type(&signiter)==BSON_INT)
								{
									sign->justification = static_cast<Sign::Justification>(bson_iterator_int(&signiter));
								}
								else if(strcmp(bson_iterator_key(&signiter), "x")==0 && bson_iterator_type(&signiter)==BSON_INT)
								{
									sign->pos.x = bson_iterator_int(&signiter)+fullX;
								}
								else if(strcmp(bson_iterator_key(&signiter), "y")==0 && bson_iterator_type(&signiter)==BSON_INT)
								{
									sign->pos.y = bson_iterator_int(&signiter)+fullY;
								}
								else
								{
									fprintf(stderr, "Unknown sign property %s\n", bson_iterator_key(&signiter));
								}
							}
						}
						else
						{
							fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&subiter));
						}
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "parts")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		if(strcmp(bson_iterator_key(&iter), "partsPos")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsPosDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsPosData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle position data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "wallMap")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (wallDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				wallData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of wall data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "fanMap")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (fanDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				fanData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of fan data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "soapLinks")==0)
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (soapLinkDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				soapLinkData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of soap data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "legacyEnable")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				globalSim->option_heatMode(bson_iterator_bool(&iter) ? HeatMode::Legacy : HeatMode::Normal);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "gravityEnable")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				int tempGrav = ngrav_enable;
				tempGrav = ((int)bson_iterator_bool(&iter))?1:0;
#ifndef RENDERER
				//Change the gravity state
				if(ngrav_enable != tempGrav)
				{
					if(tempGrav)
						start_grav_async();
					else
						stop_grav_async();
				}
#endif
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "waterEEnabled")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				water_equal_test = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "aheat_enable")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				globalSim->ambientHeatEnabled = (bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "paused")==0 && !sys_pause)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				sys_pause = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "gravityMode")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				gravityMode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "airMode")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				globalSim->airMode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if(strcmp(bson_iterator_key(&iter), "edgeMode")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				globalSim->option_edgeMode(bson_iterator_int(&iter));
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		/*else if((strcmp(bson_iterator_key(&iter), "leftSelectedElement")==0 || strcmp(bson_iterator_key(&iter), "rightSelectedElement")) && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT && bson_iterator_int(&iter) > 0 && bson_iterator_int(&iter) < PT_NUM)
			{
				if(bson_iterator_key(&iter)[0] == 'l')
				{
					sl = bson_iterator_int(&iter);
				}
				else
				{
					sr = bson_iterator_int(&iter);
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}*/
		else if(strcmp(bson_iterator_key(&iter), "activeMenu")==0 && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT && bson_iterator_int(&iter) >= 0 && bson_iterator_int(&iter) < SC_TOTAL && msections[bson_iterator_int(&iter)].doshow)
			{
				active_menu = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong value for %s\n", bson_iterator_key(&iter));
			}
		}
	}
	
	//Read wall and fan data
	if(wallData)
	{
		j = 0;
		if(blockW * blockH > wallDataLen)
		{
			fprintf(stderr, "Not enough wall data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				if (wallData[y*blockW+x])
					wallsData.wallType[blockY+y][blockX+x] = globalSim->simSD->wallTypes.convertLegacyId(wallData[y*blockW+x]);
				if (wallsData.wallType[blockY+y][blockX+x] == WL_FAN && fanData)
				{
					if(j+1 >= fanDataLen)
					{
						fprintf(stderr, "Not enough fan data\n");
					}
					wallsData.fanVX[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
					wallsData.fanVY[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
				}
			}
		}
		gravity_mask();
	}
	
	//Read particle data
	if(partsData && partsPosData)
	{
		int newIndex = 0, fieldDescriptor, tempTemp;
		int posCount, posTotal, partsPosDataIndex = 0;
		int saved_x, saved_y;
		int freeIndicesIndex = 0;
		if(fullW * fullH * 3 > partsPosDataLen)
		{
			fprintf(stderr, "Not enough particle position data\n");
			goto fail;
		}
		globalSim->parts_lastActiveIndex = NPART-1;
		freeIndicesCount = 0;
		freeIndices = (int*)calloc(sizeof(int), NPART);
		partsSimIndex = (unsigned*)calloc(NPART, sizeof(unsigned));
		partsCount = 0;
		for (i = 0; i<NPART; i++)
		{
			//Ensure ALL parts (even photons) are in the pmap so we can overwrite, keep a track of indices we can use
			if (partsptr[i].type)
			{
				x = (int)(partsptr[i].x+0.5f);
				y = (int)(partsptr[i].y+0.5f);
				pmap[y][x] = (i<<8)|1;
			}
			else
				freeIndices[freeIndicesCount++] = i;
		}
		i = 0;
		for (saved_y=0; saved_y<fullH; saved_y++)
		{
			for (saved_x=0; saved_x<fullW; saved_x++)
			{
				//Read total number of particles at this position
				posTotal = 0;
				posTotal |= partsPosData[partsPosDataIndex++]<<16;
				posTotal |= partsPosData[partsPosDataIndex++]<<8;
				posTotal |= partsPosData[partsPosDataIndex++];
				//Put the next posTotal particles at this position
				for (posCount=0; posCount<posTotal; posCount++)
				{
					//i+3 because we have 4 bytes of required fields (type (1), descriptor (2), temp (1))
					if (i+3 >= partsDataLen)
						goto fail;
					x = saved_x + fullX;
					y = saved_y + fullY;
					fieldDescriptor = partsData[i+1];
					fieldDescriptor |= partsData[i+2] << 8;
					if(x >= XRES || x < 0 || y >= YRES || y < 0)
					{
						fprintf(stderr, "Out of range [%d]: %d %d, [%d, %d], [%d, %d]\n", i, x, y, (unsigned)partsData[i+1], (unsigned)partsData[i+2], (unsigned)partsData[i+3], (unsigned)partsData[i+4]);
						goto fail;
					}
					if(!globalSim->element_isValid(partsData[i]))
						partsData[i] = PT_DMND;	//Replace all invalid elements with diamond
					if(pmap[y][x] && posCount==0) // Check posCount to make sure an existing particle is not replaced twice if two particles are saved in that position
					{
						//Replace existing particle or allocated block
						newIndex = pmap[y][x]>>8;
					}
					else if(freeIndicesIndex<freeIndicesCount)
					{
						//Create new particle
						newIndex = freeIndices[freeIndicesIndex++];
					}
					else
					{
						//Nowhere to put new particle, tpt is sad :(
						break;
					}
					if(newIndex < 0 || newIndex >= NPART)
						goto fail;

					//Store partsptr index+1 for this saved particle index (0 means not loaded)
					partsSimIndex[partsCount++] = newIndex+1;

					//Clear the particle, ready for our new properties
					memset(&(partsptr[newIndex]), 0, sizeof(particle));
					
					//Required fields
					partsptr[newIndex].type = partsData[i];
					partsptr[newIndex].x = x;
					partsptr[newIndex].y = y;
					i+=3;
					
					//Read temp
					if(fieldDescriptor & 0x01)
					{
						//Full 16bit int
						tempTemp = partsData[i++];
						tempTemp |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].temp = tempTemp;
					}
					else
					{
						//1 Byte room temp offset
						tempTemp = (signed char)partsData[i++];
						partsptr[newIndex].temp = tempTemp+294.15f;
					}
					
					//Read life
					if(fieldDescriptor & 0x02)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].life = partsData[i++];
						//Read 2nd byte
						if(fieldDescriptor & 0x04)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].life |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					//Read tmp
					if(fieldDescriptor & 0x08)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].tmp = partsData[i++];
						//Read 2nd byte
						if(fieldDescriptor & 0x10)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 8);
							//Read 3rd and 4th bytes
							if(fieldDescriptor & 0x1000)
							{
								if(i+1 >= partsDataLen) goto fail;
								partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 24);
								partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 16);
							}
						}
					}
					
					//Read ctype
					if(fieldDescriptor & 0x20)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].ctype = partsData[i++];
						//Read additional bytes
						if(fieldDescriptor & 0x200)
						{
							if(i+2 >= partsDataLen) goto fail;
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 24);
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 16);
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					//Read dcolour
					if(fieldDescriptor & 0x40)
					{
						if(i+3 >= partsDataLen) goto fail;
						partsptr[newIndex].dcolour = (((unsigned)partsData[i++]) << 24);
						partsptr[newIndex].dcolour |= (((unsigned)partsData[i++]) << 16);
						partsptr[newIndex].dcolour |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].dcolour |= ((unsigned)partsData[i++]);
					}
					
					//Read vx
					if(fieldDescriptor & 0x80)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].vx = (partsData[i++]-127.0f)/16.0f;
					}
					
					//Read vy
					if(fieldDescriptor & 0x100)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].vy = (partsData[i++]-127.0f)/16.0f;
					}

					//Read tmp2
					if(fieldDescriptor & 0x400)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].tmp2 = partsData[i++];
						//Read 2nd byte
						if(fieldDescriptor & 0x800)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].tmp2 |= (((unsigned)partsData[i++]) << 8);
						}
					}

					//Read pavg
					if(fieldDescriptor & 0x2000)
					{
						if(i+3 >= partsDataLen) goto fail;
						int pavg;
						pavg = partsData[i++];
						pavg |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].pavg[0] = (float)pavg;
						pavg = partsData[i++];
						pavg |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].pavg[1] = (float)pavg;
					}

#ifdef OGLR
					partsptr[newIndex].lastX = partsptr[newIndex].x - partsptr[newIndex].vx;
					partsptr[newIndex].lastY = partsptr[newIndex].y - partsptr[newIndex].vy;
#endif

					if (partsptr==globalSim->parts)
					{
						if ((Stickman_data::get(globalSim, PT_STKM)->exists() && partsptr[newIndex].type==PT_STKM) || (Stickman_data::get(globalSim, PT_STKM2)->exists() && partsptr[newIndex].type==PT_STKM2))
						{
							partsptr[newIndex].type = PT_NONE;
						}
						else if (partsptr[newIndex].type == PT_STKM || partsptr[newIndex].type == PT_STKM2 || partsptr[newIndex].type == PT_FIGH)
						{
							globalSim->elemData<STK_common_ElemDataSim>(partsptr[newIndex].type)->on_part_create(partsptr[newIndex]);
						}
					}

					if (partsptr[newIndex].type == PT_SOAP)
					{
						//Clear soap links, links will be added back in if soapLinkData is present
						partsptr[newIndex].ctype &= ~6;
					}
					if (!globalSim->element_isValid(partsptr[newIndex].type))
						partsptr[newIndex].type = PT_NONE;
					
					
					if (saved_version<81)
					{
						if (partsptr[newIndex].type==PT_BOMB && partsptr[newIndex].tmp!=0)
						{
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].ctype = 0;
							if (partsptr[newIndex].tmp==1)
								partsptr[newIndex].tmp = 0;
						}
						if (partsptr[newIndex].type==PT_DUST && partsptr[newIndex].life>0)
						{
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].ctype = (partsptr[newIndex].tmp2<<16) | (partsptr[newIndex].tmp<<8) | partsptr[newIndex].ctype;
							partsptr[newIndex].tmp = 1;
						}
						if (partsptr[newIndex].type==PT_FIRW && partsptr[newIndex].tmp>=2)
						{
							int caddress = tptmath::clamp_int(partsptr[newIndex].tmp-4, 0, 200-1)*3;
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].tmp = 1;
							partsptr[newIndex].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
						}
					}
					switch(partsptr[newIndex].type)
					{
					case PT_PSTN:
						if (saved_version < 87 && partsptr[newIndex].ctype)
							partsptr[newIndex].life = 1;
						if (saved_version < 91)
							partsptr[newIndex].temp = 283.15;
						break;
					case PT_FILT:
						if (saved_version < 89)
						{
							if (partsptr[newIndex].tmp<0 || partsptr[newIndex].tmp>3)
								partsptr[newIndex].tmp = 6;
							partsptr[newIndex].ctype = 0;
						}
						if (saved_version < 91)
						{
							if (partsptr[newIndex].tmp==4 || partsptr[newIndex].tmp==5)
								partsptr[newIndex].ctype = 0;
						}
						break;
					case PT_QRTZ:
					case PT_PQRT:
						if (saved_version < 89)
						{
							partsptr[newIndex].tmp2 = partsptr[newIndex].tmp;
							partsptr[newIndex].tmp = partsptr[newIndex].ctype;
							partsptr[newIndex].ctype = 0;
						}
						break;
					case PT_PHOT:
						if (saved_version < 90)
						{
							partsptr[newIndex].flags |= FLAG_PHOTDECO;
						}
						break;
					case PT_VINE:
						if (saved_version < 91)
						{
							partsptr[newIndex].tmp = 1;
						}
						break;
					case PT_DLAY:
						// correct DLAY temperature in older saves
						// due to either the +.5f now done in DLAY (higher temps), or rounding errors in the old DLAY code (room temperature temps),
						// the delay in all DLAY from older versions will always be one greater than it should
						if (saved_version < 91)
						{
							partsptr[newIndex].temp = partsptr[newIndex].temp - 1.0f;
						}
						break;
					}
				}
			}
		}
		if (soapLinkData)
		{
			int soapLinkDataPos = 0;
			for (size_t i=0; i<partsCount; i++)
			{
				if (partsSimIndex[i] && partsptr[partsSimIndex[i]-1].type == PT_SOAP)
				{
					// Get the index of the particle forward linked from this one, if present in the save data
					size_t linkedIndex = 0;
					if (soapLinkDataPos+3 > soapLinkDataLen) break;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<16;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<8;
					linkedIndex |= soapLinkData[soapLinkDataPos++];
					// All indexes in soapLinkData and partsSimIndex have 1 added to them (0 means not saved/loaded)
					if (!linkedIndex || linkedIndex-1>=partsCount || !partsSimIndex[linkedIndex-1])
						continue;
					linkedIndex = partsSimIndex[linkedIndex-1]-1;
					newIndex = partsSimIndex[i]-1;

					//Attach the two particles
					partsptr[newIndex].ctype |= 2;
					partsptr[newIndex].tmp = linkedIndex;
					partsptr[linkedIndex].ctype |= 4;
					partsptr[linkedIndex].tmp2 = newIndex;
				}
			}
		}
	}
	goto fin;
fail:
	//Clean up everything
	returnCode = 1;
fin:
	bson_destroy(&b);
	if(freeIndices)
		free(freeIndices);
	if(partsSimIndex)
		free(partsSimIndex);
	return returnCode;
}

//Old saving
pixel *prerender_save_PSv(void *save, int size, int *width, int *height)
{
	unsigned char *d,*c=(unsigned char*)save;
	int i,j,k,x,y,rx,ry,p=0, pc;
	int bw,bh,w,h,new_format = 0;
	pixel *fb;

	if (size<16)
		return NULL;
	if (!(c[2]==0x43 && c[1]==0x75 && c[0]==0x66) && !(c[2]==0x76 && c[1]==0x53 && c[0]==0x50))
		return NULL;
	if (c[2]==0x43 && c[1]==0x75 && c[0]==0x66) {
		new_format = 1;
	}
	if (c[4]>SAVE_VERSION)
		return NULL;

	bw = c[6];
	bh = c[7];
	w = bw*CELL;
	h = bh*CELL;

	if (c[5]!=CELL)
		return NULL;

	i = (unsigned)c[8];
	i |= ((unsigned)c[9])<<8;
	i |= ((unsigned)c[10])<<16;
	i |= ((unsigned)c[11])<<24;
	d = (unsigned char*)malloc(i);
	if (!d)
		return NULL;
	fb = (pixel*)calloc(w*h, PIXELSIZE);
	if (!fb)
	{
		free(d);
		return NULL;
	}

	if (BZ2_bzBuffToBuffDecompress((char *)d, (unsigned *)&i, (char *)(c+12), size-12, 0, 0))
		goto corrupt;
	size = i;

	if (size < bw*bh)
		goto corrupt;

	k = 0;
	for (y=0; y<bh; y++)
		for (x=0; x<bw; x++)
		{
			int wt = globalSim->simSD->wallTypes.convertV44Id(d[p]);
			rx = x*CELL;
			ry = y*CELL;
			if (wt)
			{
				// Not bothering to draw walls accurately because prerender_save will be removed at some point during the rewrite
				for (i=0; i<CELL; i++)
					for (j=0; j<CELL; j++)
						fb[(i+ry)*w+(j+rx)] = pc;
			}

			if (wt==WL_FAN)
				k++;
			p++;
		}
	p += 2*k;
	if (p>=size)
		goto corrupt;

	for (y=0; y<h; y++)
		for (x=0; x<w; x++)
		{
			if (p >= size)
				goto corrupt;
			j=d[p++];
			if (j<PT_NUM && j>0)
			{
				if (j==PT_STKM || j==PT_STKM2 || j==PT_FIGH)
				{
					pixel lc, hc=PIXRGB(255, 224, 178);
					if (j==PT_STKM || j==PT_FIGH) lc = PIXRGB(255, 255, 255);
					else lc = PIXRGB(100, 100, 255);
					//only need to check upper bound of y coord - lower bounds and x<w are checked in draw_line
					if (j==PT_STKM || j==PT_STKM2)
					{
						draw_line(fb , x-2, y-2, x+2, y-2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						if (y+2<h)
						{
							draw_line(fb , x-2, y+2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
							draw_line(fb , x-2, y-2, x-2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
							draw_line(fb , x+2, y-2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						}
					}
					else if (y+2<h)
					{
						draw_line(fb, x-2, y, x, y-2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x-2, y, x, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x, y-2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x, y+2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), w);
					}
					if (y+6<h)
					{
						draw_line(fb , x, y+3, x-1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), w);
						draw_line(fb , x, y+3, x+1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), w);
					}
					if (y+12<h)
					{
						draw_line(fb , x-1, y+6, x-3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), w);
						draw_line(fb , x+1, y+6, x+3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), w);
					}
				}
				else
					fb[y*w+x] = ARGBColour_TO_PIXEL(globalSim->elements[j].Colour);
			}
		}

	free(d);
	*width = w;
	*height = h;
	return fb;

corrupt:
	free(d);
	free(fb);
	return NULL;
}

int parse_save_PSv(void *save, int size, int replace, int x0, int y0, WallsDataP wallsData, Signs &signs, void* partsptr, unsigned pmap[YRES][XRES])
{
	unsigned char *d=NULL,*c=(unsigned char*)save;
	int i,j,k,x,y,p=0,*m=NULL, ver, pty, ty, legacy_beta=0, tempGrav = 0;
	int bx0=x0/CELL, by0=y0/CELL, bw, bh, w, h;
	int nf=0, new_format = 0, ttv = 0;
	particle *parts = (particle*)partsptr;
	int *fp = (int*)malloc(NPART*sizeof(int));

	//New file header uses PSv, replacing fuC. This is to detect if the client uses a new save format for temperatures
	//This creates a problem for old clients, that display and "corrupt" error instead of a "newer version" error

	if (size<16)
		return 1;
	if (!(c[2]==0x43 && c[1]==0x75 && c[0]==0x66) && !(c[2]==0x76 && c[1]==0x53 && c[0]==0x50))
		return 1;
	if (c[2]==0x76 && c[1]==0x53 && c[0]==0x50) {
		new_format = 1;
	}
	if (c[4]>SAVE_VERSION)
		return 2;
	ver = c[4];

	if (ver<34)
	{
		globalSim->option_heatMode(HeatMode::Legacy);
	}
	else
	{
		if (ver>=44) {
			globalSim->option_heatMode((c[3]&0x01) ? HeatMode::Legacy : HeatMode::Normal);
			if (!sys_pause) {
				sys_pause = (c[3]>>1)&0x01;
			}
			if (ver>=46 && replace) {
				gravityMode = ((c[3]>>2)&0x03);// | ((c[3]>>2)&0x01);
				globalSim->airMode = ((c[3]>>4)&0x07);// | ((c[3]>>4)&0x02) | ((c[3]>>4)&0x01);
			}
			if (ver>=49 && replace) {
				tempGrav = ((c[3]>>7)&0x01);		
			}
		} else {
			if (c[3]==1||c[3]==0) {
				globalSim->option_heatMode(c[3] ? HeatMode::Legacy : HeatMode::Normal);
			} else {
				legacy_beta = 1;
			}
		}
	}

	bw = c[6];
	bh = c[7];
	if (bx0+bw > XRES/CELL)
		bx0 = XRES/CELL - bw;
	if (by0+bh > YRES/CELL)
		by0 = YRES/CELL - bh;
	if (bx0 < 0)
		bx0 = 0;
	if (by0 < 0)
		by0 = 0;

	if (c[5]!=CELL || bx0+bw>XRES/CELL || by0+bh>YRES/CELL)
		return 3;
	i = (unsigned)c[8];
	i |= ((unsigned)c[9])<<8;
	i |= ((unsigned)c[10])<<16;
	i |= ((unsigned)c[11])<<24;
	if (i<0 || i > 209715200)
	{
		fprintf(stderr, "Save data too large, refusing\n");
		return 3;
	}
	d = (unsigned char*)malloc(i);
	if (!d)
		return 1;

	if (BZ2_bzBuffToBuffDecompress((char *)d, (unsigned *)&i, (char *)(c+12), size-12, 0, 0))
		return 1;
	size = i;

	if (size < bw*bh)
		return 1;

	// normalize coordinates
	x0 = bx0*CELL;
	y0 = by0*CELL;
	w  = bw *CELL;
	h  = bh *CELL;

	if (replace)
	{
		if (ver<46) {
			gravityMode = 0;
			globalSim->airMode = 0;
		}
		clear_sim();
	}
	globalSim->parts_lastActiveIndex = NPART-1;
	m = (int*)calloc(XRES*YRES, sizeof(int));

	// make a catalog of free parts
	//memset(pmap, 0, sizeof(pmap)); "Using sizeof for array given as function argument returns the size of pointer."
	memset(pmap, 0, sizeof(unsigned)*(XRES*YRES));
	for (i=0; i<NPART; i++)
		if (parts[i].type)
		{
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			pmap[y][x] = (i<<8)|1;
		}
		else
			fp[nf++] = i;

	// load the required air state
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
		{
			if (d[p])
			{
				//In old saves, ignore walls created by sign tool bug
				//Not ignoring other invalid walls or invalid walls in new saves, so that any other bugs causing them are easier to notice, find and fix
				if (ver>=44 && ver<71 && d[p]==WL_SIGN)
				{
					p++;
					continue;
				}
				if (ver>=44)
				{
					/* The numbers used to save walls were changed, starting in v44.
					 * The new numbers are ignored for older versions due to some corruption of bmap in saves from older versions. 
					 */
					wallsData.wallType[y][x] = globalSim->simSD->wallTypes.convertLegacyId(d[p]);
				}
				else
				{
					wallsData.wallType[y][x] = globalSim->simSD->wallTypes.convertV44Id(d[p]);
					if (wallsData.wallType[y][x] > 13) // 13 = max wall ID from v44 numbering system
						wallsData.wallType[y][x] = 0;
				}
				if (wallsData.wallType[y][x] >= WL_NUM)
					wallsData.wallType[y][x] = 0;
			}

			p++;
		}
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
			if (d[(y-by0)*bw+(x-bx0)]==4||(ver>=44 && d[(y-by0)*bw+(x-bx0)]==WL_FAN))
			{
				if (p >= size)
					goto corrupt;
				wallsData.fanVX[y][x] = (d[p++]-127.0f)/64.0f;
			}
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
			if (d[(y-by0)*bw+(x-bx0)]==4||(ver>=44 && d[(y-by0)*bw+(x-bx0)]==WL_FAN))
			{
				if (p >= size)
					goto corrupt;
				wallsData.fanVY[y][x] = (d[p++]-127.0f)/64.0f;
			}

	// load the particle map
	i = 0;
	pty = p;
	for (y=y0; y<y0+h; y++)
		for (x=x0; x<x0+w; x++)
		{
			if (p >= size)
				goto corrupt;
			j=d[p++];
			if (j >= PT_NUM) {
				//TODO: Possibly some server side translation
				j = PT_DUST;//goto corrupt;
			}
			if (j)
			{
				if (pmap[y][x])
				{
					k = pmap[y][x]>>8;
				}
				else if (i<nf)
				{
					k = fp[i];
					i++;
				}
				else
				{
					m[(x-x0)+(y-y0)*w] = NPART+1;
					continue;
				}
				memset(parts+k, 0, sizeof(particle));
				parts[k].type = j;
				if (j == PT_COAL)
					parts[k].tmp = 50;
				if (j == PT_FUSE)
					parts[k].tmp = 50;
				if (j == PT_PHOT)
					parts[k].ctype = 0x3fffffff;
				if (j == PT_SOAP)
					parts[k].ctype = 0;
				if (j==PT_BIZR || j==PT_BIZRG || j==PT_BIZRS)
					parts[k].ctype = 0x47FFFF;
				parts[k].x = (float)x;
				parts[k].y = (float)y;
				m[(x-x0)+(y-y0)*w] = k+1;
			}
		}

	// load particle properties
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			i--;
			if (p+1 >= size)
				goto corrupt;
			if (i < NPART)
			{
				parts[i].vx = (d[p++]-127.0f)/16.0f;
				parts[i].vy = (d[p++]-127.0f)/16.0f;
#ifdef OGLR
				parts[i].lastX = parts[i].x - parts[i].vx;
				parts[i].lastY = parts[i].y - parts[i].vy;
#endif
			}
			else
				p += 2;
		}
	}
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=44) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					ttv = (d[p++])<<8;
					ttv |= (d[p++]);
					parts[i-1].life = ttv;
				} else {
					p+=2;
				}
			} else {
				if (p >= size)
					goto corrupt;
				if (i <= NPART)
					parts[i-1].life = d[p++]*4;
				else
					p++;
			}
		}
	}
	if (ver>=44) {
		for (j=0; j<w*h; j++)
		{
			i = m[j];
			if (i)
			{
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					ttv = (d[p++])<<8;
					ttv |= (d[p++]);
					parts[i-1].tmp = ttv;
					if (ver<53 && !parts[i-1].tmp)
						for (unsigned q = 0; q<oldgolTypes.size(); q++) {
							if (parts[i-1].type==oldgolTypes[q])
							{
								LIFE_Rule &rule = globalSim->simSD->elemData<LIFE_ElemDataShared>(PT_LIFE)->rules[q];
								if (rule.liveStates()==1)
									parts[i-1].tmp = rule.liveStates();
							}
						}
					if (ver>=51 && ver<53 && parts[i-1].type==PT_PBCN)
					{
						parts[i-1].tmp2 = parts[i-1].tmp;
						parts[i-1].tmp = 0;
					}
				} else {
					p+=2;
				}
			}
		}
	}
	if (ver>=53) {
		for (j=0; j<w*h; j++)
		{
			i = m[j];
			ty = d[pty+j];
			if (i && (ty==PT_PBCN || (ty==PT_TRON && ver>=77)))
			{
				if (p >= size)
					goto corrupt;
				if (i <= NPART)
					parts[i-1].tmp2 = d[p++];
				else
					p++;
			}
		}
	}
	//Read ALPHA component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour = d[p++]<<24;
				} else {
					p++;
				}
			}
		}
	}
	//Read RED component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= d[p++]<<16;
				} else {
					p++;
				}
			}
		}
	}
	//Read GREEN component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= d[p++]<<8;
				} else {
					p++;
				}
			}
		}
	}
	//Read BLUE component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= d[p++];
				} else {
					p++;
				}
			}
		}
	}
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		ty = d[pty+j];
		if (i)
		{
			if (ver>=34&&legacy_beta==0)
			{
				if (p >= size)
				{
					goto corrupt;
				}
				if (i <= NPART)
				{
					if (ver>=42) {
						if (new_format) {
							ttv = (d[p++])<<8;
							ttv |= (d[p++]);
							if (parts[i-1].type==PT_PUMP) {
								parts[i-1].temp = ttv + 0.15;//fix PUMP saved at 0, so that it loads at 0.
							} else {
								parts[i-1].temp = ttv;
							}
						} else {
							parts[i-1].temp = (d[p++]*((MAX_TEMP+(-MIN_TEMP))/255))+MIN_TEMP;
						}
					} else {
						parts[i-1].temp = ((d[p++]*((O_MAX_TEMP+(-O_MIN_TEMP))/255))+O_MIN_TEMP)+273;
					}
				}
				else
				{
					p++;
					if (new_format) {
						p++;
					}
				}
			}
			else
			{
				parts[i-1].temp = globalSim->elements[parts[i-1].type].DefaultProperties.temp;
			}
		}
	} 
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		ty = d[pty+j];
		if (i && (ty==PT_CLNE || (ty==PT_PCLN && ver>=43) || (ty==PT_BCLN && ver>=44) || (ty==PT_SPRK && ver>=21) || (ty==PT_LAVA && ver>=34) || (ty==PT_PIPE && ver>=43) || (ty==PT_LIFE && ver>=51) || (ty==PT_PBCN && ver>=52) || (ty==PT_WIRE && ver>=55) || (ty==PT_STOR && ver>=59) || (ty==PT_CONV && ver>=60)))
		{
			if (p >= size)
				goto corrupt;
			if (i <= NPART)
				parts[i-1].ctype = d[p++];
			else
				p++;
		}
		// no more particle properties to load, so we can change type here without messing up loading
		if (i && i<=NPART)
		{
			if (parts==globalSim->parts)
			{
				if ((Stickman_data::get(globalSim, PT_STKM)->exists() && ty==PT_STKM) || (Stickman_data::get(globalSim, PT_STKM2)->exists() && ty==PT_STKM2))
				{
					parts[i-1].type = PT_NONE;
				}
				else if (parts[i-1].type == PT_STKM || parts[i-1].type == PT_STKM2 || parts[i-1].type == PT_FIGH)
				{
					globalSim->elemData<STK_common_ElemDataSim>(parts[i-1].type)->on_part_create(parts[i-1]);
				}
			}

			if (ver<79 && parts[i-1].type == PT_SPNG)
			{
				if (fabs(parts[i-1].vx)>0.0f || fabs(parts[i-1].vy)>0.0f)
					parts[i-1].flags |= FLAG_MOVABLE;
			}

			if (ver<48 && (ty==OLD_PT_WIND || (ty==PT_BRAY&&parts[i-1].life==0)))
			{
				// Replace invisible particles with something sensible and add decoration to hide it
				x = (int)(parts[i-1].x+0.5f);
				y = (int)(parts[i-1].y+0.5f);
				parts[i-1].dcolour = 0xFF000000;
				parts[i-1].type = PT_DMND;
			}
			if(ver<51 && ((ty>=78 && ty<=89) || (ty>=134 && ty<=146 && ty!=141))){
				//Replace old GOL
				parts[i-1].type = PT_LIFE;
				for (unsigned gnum = 0; gnum<oldgolTypes.size(); gnum++){
					if (ty==oldgolTypes[gnum])
						parts[i-1].ctype = gnum;
				}
				ty = PT_LIFE;
			}
			if(ver<52 && (ty==PT_CLNE || ty==PT_PCLN || ty==PT_BCLN)){
				//Replace old GOL ctypes in clone
				for (unsigned gnum = 0; gnum<oldgolTypes.size(); gnum++){
					if (parts[i-1].ctype==oldgolTypes[gnum])
					{
						parts[i-1].ctype = PT_LIFE;
						parts[i-1].tmp = gnum;
					}
				}
			}
			if(ty==PT_LCRY){
				if(ver<67)
				{
					//New LCRY uses TMP not life
					if(parts[i-1].life>=10)
					{
						parts[i-1].life = 10;
						parts[i-1].tmp2 = 10;
						parts[i-1].tmp = 3;
					}
					else if(parts[i-1].life<=0)
					{
						parts[i-1].life = 0;
						parts[i-1].tmp2 = 0;
						parts[i-1].tmp = 0;
					}
					else if(parts[i-1].life < 10 && parts[i-1].life > 0)
					{
						parts[i-1].tmp = 1;
					}
				}
				else
				{
					parts[i-1].tmp2 = parts[i-1].life;
				}
			}
			if (!globalSim->element_isValid(parts[i-1].type))
				parts[i-1].type = PT_NONE;
			
			if (ver<81)
			{
				if (parts[i-1].type==PT_BOMB && parts[i-1].tmp!=0)
				{
					parts[i-1].type = PT_EMBR;
					parts[i-1].ctype = 0;
					if (parts[i-1].tmp==1)
						parts[i-1].tmp = 0;
				}
				if (parts[i-1].type==PT_DUST && parts[i-1].life>0)
				{
					parts[i-1].type = PT_EMBR;
					parts[i-1].ctype = (parts[i-1].tmp2<<16) | (parts[i-1].tmp<<8) | parts[i-1].ctype;
					parts[i-1].tmp = 1;
				}
				if (parts[i-1].type==PT_FIRW && parts[i-1].tmp>=2)
				{
					int caddress = tptmath::clamp_int(parts[i-1].tmp-4, 0, 200-1);
					parts[i-1].type = PT_EMBR;
					parts[i-1].tmp = 1;
					parts[i-1].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
				}
			}
			switch(parts[i-1].type)
			{
			case PT_FILT:
				if (ver<89)
				{
					if (parts[i-1].tmp<0 || parts[i-1].tmp>3)
						parts[i-1].tmp = 6;
					parts[i-1].ctype = 0;
				}
				if (ver < 91)
				{
					if (parts[i-1].tmp==4 || parts[i-1].tmp==5)
						parts[i-1].ctype = 0;
				}
				break;
			case PT_QRTZ:
			case PT_PQRT:
				if (ver<89)
				{
					parts[i-1].tmp2 = parts[i-1].tmp;
					parts[i-1].tmp = parts[i-1].ctype;
					parts[i-1].ctype = 0;
				}
				break;
			case PT_PHOT:
				if (ver<90)
				{
					parts[i-1].flags |= FLAG_PHOTDECO;
				}
				break;
			case PT_VINE:
				if (ver<91)
				{
					parts[i-1].tmp = 1;
				}
				break;
			}
		}
	}

	#ifndef RENDERER
	//Change the gravity state
	if(ngrav_enable != tempGrav && replace)
	{
		if(tempGrav)
			start_grav_async();
		else
			stop_grav_async();
	}
	#endif
	
	gravity_mask();

	if (p >= size)
		goto version1;
	j = d[p++];
	for (i=0; i<j; i++)
	{
		if (p+6 > size)
			goto corrupt;
		Sign *sign = globalSim->signs.add();
		x = d[p++];
		x |= ((unsigned)d[p++])<<8;
		if (sign)
			sign->pos.x = x+x0;
		x = d[p++];
		x |= ((unsigned)d[p++])<<8;
		if (sign)
			sign->pos.y = x+y0;
		x = d[p++];
		if (sign)
			sign->justification = static_cast<Sign::Justification>(x);
		x = d[p++];
		if (p+x > size)
			goto corrupt;
		if (sign)
		{
			std::string text;
			text.assign((char*)d+p, x);
			sign->setRawText(text);
		}
		p += x;
	}

version1:
	if (m) free(m);
	if (d) free(d);
	if (fp) free(fp);

	return 0;

corrupt:
	if (m) free(m);
	if (d) free(d);
	if (fp) free(fp);
	if (replace)
	{
		clear_sim();
	}
	return 1;
}

void *build_thumb(int *size, int bzip2)
{
	unsigned char *d=(unsigned char*)calloc(1,XRES*YRES), *c;
	int i,j,x,y;
	for (i=0; i<NPART; i++)
		if (parts[i].type)
		{
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				d[x+y*XRES] = parts[i].type;
		}
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
			if (globalSim->walls.type(SimPosCell(x,y)))
				for (j=0; j<CELL; j++)
					for (i=0; i<CELL; i++)
						d[x*CELL+i+(y*CELL+j)*XRES] = 0xFF;
	j = XRES*YRES;

	if (bzip2)
	{
		i = (j*101+99)/100 + 608;
		c = (unsigned char*)malloc(i);

		c[0] = 0x53;
		c[1] = 0x68;
		c[2] = 0x49;
		c[3] = 0x74;
		c[4] = PT_NUM;
		c[5] = CELL;
		c[6] = XRES/CELL;
		c[7] = YRES/CELL;

		i -= 8;

		if (BZ2_bzBuffToBuffCompress((char *)(c+8), (unsigned *)&i, (char *)d, j, 9, 0, 0) != BZ_OK)
		{
			free(d);
			free(c);
			return NULL;
		}
		free(d);
		*size = i+8;
		return c;
	}

	*size = j;
	return d;
}

void *transform_save(void *odata, int *size, matrix2d transform, vector2d translate)
{
	void *ndata;
	WallsData wallso, wallsn;
	particle *partst = (particle*)calloc(sizeof(particle), NPART);
	Signs signst;
	unsigned (*pmapt)[XRES] = (unsigned(*)[XRES])calloc(YRES*XRES, sizeof(unsigned));
	int i, nx, ny, w, h, nw, nh;
	vector2d pos, tmp, ctl, cbr;
	vector2d vel;
	vector2d cornerso[4];
	unsigned char *odatac = (unsigned char*)odata;
	if (parse_save(odata, *size, 0, 0, 0, wallso, signst, partst, pmapt))
	{
		free(partst);
		free(pmapt);
		return odata;
	}
	w = odatac[6]*CELL;
	h = odatac[7]*CELL;
	// undo any translation caused by rotation
	cornerso[0] = v2d_new(0,0);
	cornerso[1] = v2d_new(w-1,0);
	cornerso[2] = v2d_new(0,h-1);
	cornerso[3] = v2d_new(w-1,h-1);
	for (i=0; i<4; i++)
	{
		tmp = m2d_multiply_v2d(transform,cornerso[i]);
		if (i==0) ctl = cbr = tmp; // top left, bottom right corner
		if (tmp.x<ctl.x) ctl.x = tmp.x;
		if (tmp.y<ctl.y) ctl.y = tmp.y;
		if (tmp.x>cbr.x) cbr.x = tmp.x;
		if (tmp.y>cbr.y) cbr.y = tmp.y;
	}
	// casting as int doesn't quite do what we want with negative numbers, so use floor()
	tmp = v2d_new(floor(ctl.x+0.5f),floor(ctl.y+0.5f));
	translate = v2d_sub(translate,tmp);
	nw = floor(cbr.x+0.5f)-floor(ctl.x+0.5f)+1;
	nh = floor(cbr.y+0.5f)-floor(ctl.y+0.5f)+1;
	if (nw>XRES) nw = XRES;
	if (nh>YRES) nh = YRES;
	// rotate and translate signs, parts, walls
	for (auto it=signst.begin(); it!=signst.end(); )
	{
		pos = v2d_new(it->pos.x, it->pos.y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
		nx = floor(pos.x+0.5f);
		ny = floor(pos.y+0.5f);
		it->pos.x = nx;
		it->pos.y = ny;
		if (nx<0 || nx>=nw || ny<0 || ny>=nh)
		{
			it = signst.erase(it);
		}
		else
		{
			++it;
		}
	}
	for (i=0; i<NPART; i++)
	{
		if (!partst[i].type) continue;
		pos = v2d_new(partst[i].x, partst[i].y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
		nx = floor(pos.x+0.5f);
		ny = floor(pos.y+0.5f);
		if (nx<0 || nx>=nw || ny<0 || ny>=nh)
		{
			partst[i].type = PT_NONE;
			continue;
		}
		partst[i].x = nx;
		partst[i].y = ny;
		vel = v2d_new(partst[i].vx, partst[i].vy);
		vel = m2d_multiply_v2d(transform, vel);
		partst[i].vx = vel.x;
		partst[i].vy = vel.y;
	}
	for (int y=0; y<YRES/CELL; y++)
		for (int x=0; x<XRES/CELL; x++)
		{
			pos = v2d_new(x*CELL+CELL*0.4f, y*CELL+CELL*0.4f);
			pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
			nx = pos.x/CELL;
			ny = pos.y/CELL;
			if (nx<0 || nx>=nw/CELL || ny<0 || ny>=nh/CELL)
				continue;
			if (wallso.wallType[y][x])
			{
				wallsn.wallType[ny][nx] = wallso.wallType[y][x];
				if (wallso.wallType[y][x]==WL_FAN)
				{
					vel = v2d_new(wallso.fanVX[y][x], wallso.fanVY[y][x]);
					vel = m2d_multiply_v2d(transform, vel);
					wallsn.fanVX[ny][nx] = vel.x;
					wallsn.fanVY[ny][nx] = vel.y;
				}
			}
			/* TODO:
			vel = v2d_new(vxo[y][x], vyo[y][x]);
			vel = m2d_multiply_v2d(transform, vel);
			vxn[ny][nx] = vel.x;
			vyn[ny][nx] = vel.y;
			pvn[ny][nx] = pvo[y][x];
			*/
		}
	ndata = build_save(size,0,0,nw,nh,wallso,signst,partst);
	free(partst);
	free(pmapt);
	return ndata;
}

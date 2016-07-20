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
#include "simulation/elements/PIPE.h"
#include "simulation/elements/PRTI.h"
#include "simulation/elements/SOAP.h"

#include <algorithm>
#include <locale>

#define PFLAG_NORMALSPEED 0x00010000

// parts[].tmp flags
// trigger flags to be processed this frame (trigger flags for next frame are shifted 3 bits to the left):
#define PPIP_TMPFLAG_TRIGGER_ON 0x10000000
#define PPIP_TMPFLAG_TRIGGER_OFF 0x08000000
#define PPIP_TMPFLAG_TRIGGER_REVERSE 0x04000000
#define PPIP_TMPFLAG_TRIGGERS 0x1C000000
// current status of the pipe
#define PPIP_TMPFLAG_PAUSED 0x02000000
#define PPIP_TMPFLAG_REVERSED 0x01000000
// 0x000000FF element
// 0x00000100 is single pixel pipe
// 0x00000200 will transfer like a single pixel pipe when in forward mode
// 0x00001C00 forward single pixel pipe direction
// 0x00002000 will transfer like a single pixel pipe when in reverse mode
// 0x0001C000 reverse single pixel pipe direction

const signed char pos_1_rx[] = {-1,-1,-1, 0, 0, 1, 1, 1};
const signed char pos_1_ry[] = {-1, 0, 1,-1, 1,-1, 0, 1};

int ppip_changed = 0;

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy)
{
	int coord_stack_limit = XRES*YRES;
	unsigned short (*coord_stack)[2];
	int coord_stack_size = 0;
	int x1, x2;
	int rcount, ri, rnext;

	// Separate flags for on and off in case PPIP is sparked by PSCN and NSCN on the same frame
	// - then PSCN can override NSCN and behaviour is not dependent on particle order
	int prop = 0;
	if (sparkedBy==PT_PSCN) prop = PPIP_TMPFLAG_TRIGGER_ON << 3;
	else if (sparkedBy==PT_NSCN) prop = PPIP_TMPFLAG_TRIGGER_OFF << 3;
	else if (sparkedBy==PT_INST) prop = PPIP_TMPFLAG_TRIGGER_REVERSE << 3;

	ri = sim->pmap_find_one(x,y,PT_PPIP);
	if (prop==0 || ri<0 || (parts[ri].tmp & prop))
		return;

	coord_stack = (unsigned short(*)[2])malloc(sizeof(unsigned short)*2*coord_stack_limit);
	coord_stack[coord_stack_size][0] = x;
	coord_stack[coord_stack_size][1] = y;
	coord_stack_size++;

	do
	{
		coord_stack_size--;
		x = coord_stack[coord_stack_size][0];
		y = coord_stack[coord_stack_size][1];

		bool found = false;
		FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
		{
			if (parts[ri].type==PT_PPIP)
			{
				if (!(parts[ri].tmp & prop))
				{
					ppip_changed = 1;
					found = true;
				}
				parts[ri].tmp |= prop;
			}
		}
		if (!found) continue;

		x1 = x2 = x;
		// go left as far as possible
		while (x1>=CELL)
		{
			bool found = false;
			FOR_PMAP_POSITION_NOENERGY(sim, x1-1, y, rcount, ri, rnext)
			{
				if (parts[ri].type==PT_PPIP)
				{
					if (!(parts[ri].tmp & prop))
					{
						ppip_changed = 1;
						found = true;
					}
					parts[ri].tmp |= prop;
				}
			}
			if (!found) break;
			x1--;
		}
		// go right as far as possible
		while (x2<XRES-CELL)
		{
			bool found = false;
			FOR_PMAP_POSITION_NOENERGY(sim, x2+1, y, rcount, ri, rnext)
			{
				if (parts[ri].type==PT_PPIP)
				{
					if (!(parts[ri].tmp & prop))
						ppip_changed = 1;
					parts[ri].tmp |= prop;
					found = true;
				}
			}
			if (!found) break;
			x2++;
		}

		// add adjacent pixels to stack
		// +-1 to x limits to include diagonally adjacent pixels
		// Don't need to check x bounds here, because already limited to [CELL, XRES-CELL]
		if (y>=CELL+1)
			for (x=x1-1; x<=x2+1; x++)
			{
				coord_stack[coord_stack_size][0] = x;
				coord_stack[coord_stack_size][1] = y-1;
				coord_stack_size++;
				if (coord_stack_size>=coord_stack_limit)
				{
					free(coord_stack);
					return;
				}
			}
		if (y<YRES-CELL-1)
			for (x=x1-1; x<=x2+1; x++)
			{
				coord_stack[coord_stack_size][0] = x;
				coord_stack[coord_stack_size][1] = y+1;
				coord_stack_size++;
				if (coord_stack_size>=coord_stack_limit)
				{
					free(coord_stack);
					return;
				}
			}
	} while (coord_stack_size>0);
	free(coord_stack);
}

void PIPE_transfer_pipe_to_part(Simulation *sim, particle *pipe, particle *part)
{
	part->type = (pipe->tmp & 0xFF);
	part->temp = pipe->temp;
	part->life = pipe->tmp2;
	part->tmp = pipe->pavg[0];
	part->ctype = pipe->pavg[1];
	pipe->tmp &= ~0xFF;

	if (!(sim->elements[part->type].Properties & TYPE_ENERGY))
	{
		part->vx = 0.0f;
		part->vy = 0.0f;
	}
	else if (part->type == PT_PHOT && part->ctype == 0x40000000)
		part->ctype = 0x3FFFFFFF;
	part->tmp2 = 0;
	part->flags = 0;
	part->dcolour = 0;
}

void PIPE_transfer_part_to_pipe(Simulation *sim, particle *part, particle *pipe)
{
	pipe->tmp = (pipe->tmp&~0xFF) | part->type;
	pipe->temp = part->temp;
	pipe->tmp2 = part->life;
	pipe->pavg[0] = part->tmp;
	pipe->pavg[1] = part->ctype;
}

void PIPE_transfer_pipe_to_pipe(Simulation *sim, particle *src, particle *dest)
{
	dest->tmp = (dest->tmp&~0xFF) | (src->tmp&0xFF);
	dest->temp = src->temp;
	dest->tmp2 = src->tmp2;
	dest->pavg[0] = src->pavg[0];
	dest->pavg[1] = src->pavg[1];
	src->tmp &= ~0xFF;
}

void pushParticle(Simulation *sim, int i, int count, int original)
{
	int rx, ry, rt, x, y, np, q, notctype=(((parts[i].ctype)%3)+2);
	int rcount, ri, rnext;
	if ((parts[i].tmp&0xFF) == 0 || count >= 2)//don't push if there is nothing there, max speed of 2 per frame
		return;
	x = (int)(parts[i].x+0.5f);
	y = (int)(parts[i].y+0.5f);
	if( !(parts[i].tmp&0x200) )
	{ 
		//normal random push
		for (q=0; q<3; q++)//try to push 3 times
		{
			sim->randomRelPos_1_noCentre(&rx,&ry);
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					rt = parts[ri].type;
					if ((rt==PT_PIPE || rt==PT_PPIP) && parts[ri].ctype!=notctype && (parts[ri].tmp&0xFF)==0)
					{
						PIPE_transfer_pipe_to_pipe(sim, parts+i, parts+ri);
						if (ri > original)
							parts[ri].flags |= PFLAG_NORMALSPEED;//skip particle push, normalizes speed
						pushParticle(sim, ri, count+1, original);
						break;
					}
					else if (rt == PT_PRTI) //Pass particles into PRTI for a pipe speed increase
					{
						PortalChannel *channel = sim->elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(parts[ri]);
						int slot = PRTI_ElemDataSim::GetPosIndex(-rx, -ry);
						particle *storePart = channel->AllocParticle(slot);
						if (storePart)
						{
							PIPE_transfer_pipe_to_part(sim, parts+i, storePart);
							break;
						}
					}
				}
				if (!(parts[i].tmp&0xFF))
					break;
			}
		}
	}
	else //predefined 1 pixel thick pipe movement
	{
		int coords = 7 - ((parts[i].tmp>>10)&7);
		bool foundSomething = false;
		FOR_PMAP_POSITION_NOENERGY(sim, x+pos_1_rx[coords], y+pos_1_ry[coords], rcount, ri, rnext)
		{
			rt = parts[ri].type;
			if ((rt==PT_PIPE || rt==PT_PPIP) && parts[ri].ctype!=notctype && (parts[ri].tmp&0xFF)==0)
			{
				PIPE_transfer_pipe_to_pipe(sim, parts+i, parts+ri);
				if (ri > original)
					parts[ri].flags |= PFLAG_NORMALSPEED;//skip particle push, normalizes speed
				pushParticle(sim,ri,count+1,original);
				foundSomething = true;
			}
			else if (rt == PT_PRTI) //Pass particles into PRTI for a pipe speed increase
			{
				PortalChannel *channel = sim->elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(parts[ri]);
				int slot = PRTI_ElemDataSim::GetPosIndex(-pos_1_rx[coords], -pos_1_ry[coords]);
				particle *storePart = channel->AllocParticle(slot);
				if (storePart)
				{
					PIPE_transfer_pipe_to_part(sim, parts+i, storePart);
					break;
				}
				foundSomething = true;
			}
		}
		if (!foundSomething)
		{
			//Move particles out of pipe automatically, much faster at ends
			rx = pos_1_rx[coords];
			ry = pos_1_ry[coords];
			np = sim->part_create(-1,x+rx,y+ry,parts[i].tmp&0xFF);
			if (np!=-1)
			{
				PIPE_transfer_pipe_to_part(sim, parts+i, parts+np);
			}
		}
	}
	return;
}

int PIPE_update(UPDATE_FUNC_ARGS)
{
	int rx, ry, np;
	int rcount, ri, rnext;
	if (!sim->IsValidElement(parts[i].tmp&0xFF))
		parts[i].tmp &= ~0xFF;
	if (parts[i].tmp & PPIP_TMPFLAG_TRIGGERS)
	{
		int pause_changed = 0;
		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_ON) // TRIGGER_ON overrides TRIGGER_OFF
		{
			if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
				pause_changed = 1;
			parts[i].tmp &= ~PPIP_TMPFLAG_PAUSED;
		}
		else if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_OFF)
		{
			if (!(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
				pause_changed = 1;
			parts[i].tmp |= PPIP_TMPFLAG_PAUSED;
		}
		if (pause_changed)
		{
			int rx, ry;
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
				{
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
						{
							if (parts[ri].type == PT_BRCK)
							{
								if (parts[i].tmp & PPIP_TMPFLAG_PAUSED)
									parts[ri].tmp = 0;
								else
									parts[ri].tmp = 1; //make surrounding BRCK glow
							}
						}
					}
				}
		}

		if (parts[i].tmp & PPIP_TMPFLAG_TRIGGER_REVERSE)
		{
			parts[i].tmp ^= PPIP_TMPFLAG_REVERSED;
			if (parts[i].ctype == 2) //Switch colors so it goes in reverse
				parts[i].ctype = 4;
			else if (parts[i].ctype == 4)
				parts[i].ctype = 2;
			if (parts[i].tmp & 0x100) //Switch one pixel pipe direction
			{
				int coords = (parts[i].tmp>>13)&0xF;
				int coords2 = (parts[i].tmp>>9)&0xF;
				parts[i].tmp &= ~0x1FE00;
				parts[i].tmp |= coords<<9;
				parts[i].tmp |= coords2<<13;
			}
		}

		parts[i].tmp &= ~PPIP_TMPFLAG_TRIGGERS;
	}

	if (parts[i].ctype>=2 && parts[i].ctype<=4 && !(parts[i].tmp & PPIP_TMPFLAG_PAUSED))
	{
		if (parts[i].life==3)
		{
			int lastneighbor = -1;
			int neighborcount = 0;
			int count = 0;
			// make automatic pipe pattern
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
						{
							if (parts[ri].type==PT_PIPE || parts[ri].type==PT_PPIP)
							{
								if (parts[ri].ctype==1)
								{
									parts[ri].ctype = (((parts[i].ctype)%3)+2);//reverse
									parts[ri].life = 6;
									if ( parts[i].tmp&0x100)//is a single pixel pipe
									{
										parts[ri].tmp |= 0x200;//will transfer to a single pixel pipe
										parts[ri].tmp |= count<<10;//coords of where it came from
										parts[i].tmp |= ((7-count)<<14);
										parts[i].tmp |= 0x2000;
									}
									neighborcount ++;
									lastneighbor = ri;
								}
								else if (parts[ri].ctype!=(((parts[i].ctype-1)%3)+2))
								{
									neighborcount ++;
									lastneighbor = ri;
								}
							}
						}
						count++;
					}
					if(neighborcount == 1)
						parts[lastneighbor].tmp |= 0x100;
		}
		else
		{
			if (parts[i].flags&PFLAG_NORMALSPEED)//skip particle push to prevent particle number being higher causing speed up
			{
				parts[i].flags &= ~PFLAG_NORMALSPEED;
			}
			else
			{
				pushParticle(sim, i,0,i);
			}

			if (nt)//there is something besides PIPE around current particle
			{
				sim->randomRelPos_1_noCentre(&rx,&ry);
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					// TODO: normal particles in pmap should block creation
					if (surround_space && (parts[i].tmp&0xFF)!=0 && (np=sim->part_create(-1,x+rx,y+ry,parts[i].tmp&0xFF))>=0)  //creating at end
					{
						PIPE_transfer_pipe_to_part(sim, parts+i, parts+np);
					}
					else
					{
						FOR_PMAP_POSITION(sim, x+rx, y+ry, rcount, ri, rnext)
						{
							//try eating particle at entrance
							if ((parts[i].tmp&0xFF) == 0 && (sim->elements[parts[ri].type].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
							{
								if (parts[ri].type==PT_SOAP)
									SOAP_detach(sim, ri);
								PIPE_transfer_part_to_pipe(sim, parts+ri, parts+i);
								sim->part_kill(ri);
							}
							else if ((parts[i].tmp&0xFF) == 0 && parts[ri].type==PT_STOR && parts[ri].tmp>0 && sim->IsValidElement(parts[ri].tmp) && (sim->elements[parts[ri].tmp].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)))
							{
								// STOR stores properties in the same places as PIPE does
								PIPE_transfer_pipe_to_pipe(sim, parts+ri, parts+i);
							}
						}
					}
				}
			}
		}
	}
	else if (!parts[i].ctype && parts[i].life<=10)
	{
		if (parts[i].temp<272.15)//manual pipe colors
		{
			if (parts[i].temp>173.25)
				parts[i].ctype = 2;
			else if (parts[i].temp>73.25)
				parts[i].ctype = 3;
			else
				parts[i].ctype = 4;
			parts[i].life = 0;
		}
		else
		{
			// make a border
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
				{
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						int index = sim->part_create(-1,x+rx,y+ry,PT_BRCK);//BRCK border, people didn't like DMND
						if (parts[i].type == PT_PPIP && index != -1)
							parts[index].tmp = 1;
					}
				}
			if (parts[i].life<=1)
				parts[i].ctype = 1;
		}
	}
	else if (parts[i].ctype==1)//wait for empty space before starting to generate automatic pipe pattern
	{
		if (!parts[i].life)
		{
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						uint8_t wallId = sim->walls.type(SimPosI(x+rx,y+ry));
						if (!sim->pmap[y+ry][x+rx].count(PMapCategory::Plain) && (!wallId || (wallId!=WL_ALLOWAIR && wallId!=WL_WALL && wallId!=WL_WALLELEC && (wallId!=WL_EWALL || sim->walls.electricity(SimPosI(x+rx,y+ry))))))
							parts[i].life=50;
					}
		}
		else if (parts[i].life==5)//check for beginning of pipe single pixel
		{
			int issingle = 1;
			for (rx=-1; rx<2; rx++)
				for (ry=-1; ry<2; ry++)
					if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						if ((sim->pmap_find_one(x+rx,y+ry,PT_PIPE)>=0 || sim->pmap_find_one(x+rx,y+ry,PT_PPIP)>=0) && parts[i].ctype==1 && parts[i].life )
							issingle = 0;
					}
			if (issingle)
				parts[i].tmp |= 0x100;
		}
		else if (parts[i].life==2)
		{
			parts[i].ctype = 2;
			parts[i].life = 6;
		}
	}
	return 0;
}

int PIPE_graphics(GRAPHICS_FUNC_ARGS)
{

	if ((cpart->tmp&0xFF) && sim->IsValidElement(cpart->tmp&0xFF))
	{
		//Create a temp. particle and do a subcall.
		particle tpart;
		int t;
		memset(&tpart, 0, sizeof(particle));
		tpart.type = cpart->tmp&0xFF;
		tpart.temp = cpart->temp;
		tpart.life = cpart->tmp2;
		tpart.tmp = cpart->pavg[0];
		tpart.ctype = cpart->pavg[1];
		if (tpart.type == PT_PHOT && tpart.ctype == 0x40000000)
			tpart.ctype = 0x3FFFFFFF;
		t = tpart.type;
		if (graphicscache[t].isready)
		{
			*pixel_mode = graphicscache[t].pixel_mode;
			*cola = graphicscache[t].cola;
			*colr = graphicscache[t].colr;
			*colg = graphicscache[t].colg;
			*colb = graphicscache[t].colb;
			*firea = graphicscache[t].firea;
			*firer = graphicscache[t].firer;
			*fireg = graphicscache[t].fireg;
			*fireb = graphicscache[t].fireb;
		}
		else
		{
			*colr = COLR(sim->elements[t].Colour);
			*colg = COLG(sim->elements[t].Colour);
			*colb = COLB(sim->elements[t].Colour);
			if (sim->elements[t].Graphics)
			{
				(*(sim->elements[t].Graphics))(sim, &tpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}
			else
			{
				Element::Graphics_default(sim, &tpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
			}
		}
	}
	else
	{
		if (cpart->ctype==2)
		{
			*colr = 50;
			*colg = 1;
			*colb = 1;
		}
		else if (cpart->ctype==3)
		{
			*colr = 1;
			*colg = 50;
			*colb = 1;
		}
		else if (cpart->ctype==4)
		{
			*colr = 1;
			*colg = 1;
			*colb = 50;
		}
		else if (cpart->temp<272.15&&cpart->ctype!=1)
		{
			if (cpart->temp>173.25&&cpart->temp<273.15)
			{
				*colr = 50;
				*colg = 1;
				*colb = 1;
			}
			if (cpart->temp>73.25&&cpart->temp<=173.15)
			{
				*colr = 1;
				*colg = 50;
				*colb = 1;
			}
			if (cpart->temp>=0&&cpart->temp<=73.15)
			{
				*colr = 1;
				*colg = 1;
				*colb = 50;
			}
		}
	}
	return 0;
}

std::string Element_UI_PIPE::getHUDText(Simulation *sim, int i, bool debugMode)
{
	int storedElem = (sim->parts[i].tmp&0xFF);
	if (!sim->IsValidElement(storedElem))
		storedElem = PT_NONE;
	if (!storedElem)
		return Name;

	std::string storedName = sim->elements[storedElem].ui->getLowercaseName();
	if (sim->parts[i].type==PT_PPIP)
		return "PPIP with " + storedName;
	else
		return "Pipe with " + storedName;
}

void PIPE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI_PIPE>();

	elem->Identifier = "DEFAULT_PT_PIPE";
	elem->ui->Name = "PIPE";
	elem->Colour = COLPACK(0x444444);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_FORCE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.95f;
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

	elem->DefaultProperties.temp = 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "PIPE, moves particles around. Once the BRCK generates, erase some for the exit. Then the PIPE generates and is usable.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = 10.0f;
	elem->HighPressureTransitionElement = PT_BRMT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 60;

	elem->Update = &PIPE_update;
	elem->Graphics = &PIPE_graphics;
}


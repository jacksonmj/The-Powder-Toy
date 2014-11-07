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

/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
int PRTO_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elementData[PT_PRTI])
		return 0;
	int r, nnx, rx, ry, np, fe = 0;
	int count = 0;
	PortalChannel *channel = ((PRTI_ElementDataContainer*)sim->elementData[PT_PRTI])->GetParticleChannel(sim, i);
	for (count=0; count<8; count++)
	{
		rx = portal_rx[(count + 4)%8];
		ry = portal_ry[(count + 4)%8];
		if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
		{
			if (!sim->pmap[y+ry][x+rx].count_notEnergy)
			{
				fe = 1;
				//add -1,0,or 1 to count
				int randomness = (count + rand()%3-1 + 4)%8;
				if (!channel->particleCount[randomness])
					continue;
				for ( nnx =0 ; nnx<PortalChannel::storageSize; nnx++)
				{
					if (!channel->portalp[randomness][nnx].type) continue;
					particle *storedPart = &(channel->portalp[randomness][nnx]);
					if (storedPart->type==PT_SPRK)// TODO: make it look better, spark creation
					{
						sim->spark_position_conductiveOnly(x+1, y+1);
						sim->spark_position_conductiveOnly(x+1, y);
						sim->spark_position_conductiveOnly(x+1, y-1);
						sim->spark_position_conductiveOnly(x, y+1);
						sim->spark_position_conductiveOnly(x, y-1);
						sim->spark_position_conductiveOnly(x-1, y+1);
						sim->spark_position_conductiveOnly(x-1, y);
						sim->spark_position_conductiveOnly(x-1, y-1);
						storedPart->type = 0;
						channel->particleCount[randomness]--;
						break;
					}
					else if (storedPart->type)
					{
						if (storedPart->type==PT_STKM)
							player.spwn = 0;
						if (storedPart->type==PT_STKM2)
							player2.spwn = 0;
						if (storedPart->type==PT_FIGH)
						{
							((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Free(storedPart->tmp);
						}
						np = sim->part_create(-1,x+rx,y+ry,storedPart->type);
						if (np<0)
						{
							if (storedPart->type==PT_STKM)
								player.spwn = 1;
							if (storedPart->type==PT_STKM2)
								player2.spwn = 1;
							if (storedPart->type==PT_FIGH)
							{
								((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->AllocSpecific(storedPart->tmp);
							}
							continue;
						}
						if (parts[np].type==PT_FIGH)
						{
							// Release the fighters[] element allocated by part_create, the one reserved when the fighter went into the portal will be used
							((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->Free(parts[np].tmp);
							((FIGH_ElementDataContainer*)sim->elementData[PT_FIGH])->AllocSpecific(storedPart->tmp);
						}

						// Don't overwrite pmap links with old values from the stored particle in the portal, otherwise crashes are likely
						int pmap_next = parts[np].pmap_next;
						int pmap_prev = parts[np].pmap_prev;
						if (storedPart->vx == 0.0f && storedPart->vy == 0.0f)
						{
							// particles that have passed from PIPE into PRTI have lost their velocity, so use the velocity of the newly created particle if the particle in the portal has no velocity
							float tmp_vx = parts[np].vx;
							float tmp_vy = parts[np].vy;
							parts[np] = *storedPart;
							parts[np].vx = tmp_vx;
							parts[np].vy = tmp_vy;
						}
						else
						{
							parts[np] = *storedPart;
						}
						parts[np].pmap_next = pmap_next;
						parts[np].pmap_prev = pmap_prev;
						parts[np].x = x+rx;
						parts[np].y = y+ry;
						storedPart->type = 0;
						channel->particleCount[randomness]--;
						break;
					}
				}
			}
		}
	}
	if (fe) {
		int orbd[4] = {0, 0, 0, 0};	//Orbital distances
		int orbl[4] = {0, 0, 0, 0};	//Orbital locations
		if (!parts[i].life) parts[i].life = rand()*rand()*rand();
		if (!parts[i].ctype) parts[i].ctype = rand()*rand()*rand();
		orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
		for (r = 0; r < 4; r++) {
			if (orbd[r]<254) {
				orbd[r] += 16;
				if (orbd[r]>254) {
					orbd[r] = 0;
					orbl[r] = rand()%255;
				} else {
					orbl[r] += 1;
					orbl[r] = orbl[r]%255;
				}
			} else {
				orbd[r] = 0;
				orbl[r] = rand()%255;
			}
		}
		orbitalparts_set(&parts[i].life, &parts[i].ctype, orbd, orbl);
	} else {
		parts[i].life = 0;
		parts[i].ctype = 0;
	}
	return 0;
}

int PRTO_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 8;
	*firer = 0;
	*fireg = 0;
	*fireb = 255;
	*pixel_mode |= EFFECT_GRAVOUT;
	*pixel_mode |= EFFECT_LINES;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_ADD;
	return 1;
}

void PRTO_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PRTO";
	elem->Name = "PRTO";
	elem->Colour = COLPACK(0x0020EB);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.005f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Portal OUT. Things come out here, now with temperature dependent channels (same as WIFI)";

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

	elem->Update = &PRTO_update;
	elem->Graphics = &PRTO_graphics;
}


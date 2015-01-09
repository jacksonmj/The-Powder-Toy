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
#include "simulation/elements/STKM.h"

/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
int PRTO_update(UPDATE_FUNC_ARGS)
{
	if (!sim->elemData(PT_PRTI))
		return 0;
	int r, nnx, rx, ry, np, fe = 0;
	int count = 0;
	PortalChannel *channel = sim->elemData<PRTI_ElemDataSim>(PT_PRTI)->GetParticleChannel(sim->parts[i]);
	for (count=0; count<8; count++)
	{
		rx = portal_rx[count];
		ry = portal_ry[count];
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
						STK_common_ElemDataSim *ed = NULL;
						if (storedPart->type==PT_STKM || storedPart->type==PT_STKM2 || storedPart->type==PT_FIGH)
							ed = sim->elemData<STK_common_ElemDataSim>(storedPart->type);

						if (ed)
						{
							ed->storageActionPending = true;//causes the create_allowed check to be overridden
							np = sim->part_create(-1,x+rx,y+ry,storedPart->type);
							if (np<0)
							{
								ed->storageActionPending = false;
								continue;
							}
							sim->part_copy_properties(*storedPart, sim->parts[np]);
							if (storedPart->type==PT_FIGH)
							{
								Stickman_data *stkdata = Stickman_data::get(sim, sim->parts[np]);
								if (stkdata)
								{
									//have to set these here because FIGH_ChangeType does not know which fighter it is
									stkdata->isStored = false;
									stkdata->set_particle(&sim->parts[np]);
								}
							}
						}
						else
						{
							np = sim->part_create(-1,x+rx,y+ry,storedPart->type);
							if (np<0)
								continue;
						}

						if (storedPart->vx == 0.0f && storedPart->vy == 0.0f)
						{
							// particles that have passed from PIPE into PRTI have lost their velocity, so use the velocity of the newly created particle if the particle in the portal has no velocity
							float tmp_vx = parts[np].vx;
							float tmp_vy = parts[np].vy;
							sim->part_copy_properties(*storedPart, parts[np]);
							parts[np].vx = tmp_vx;
							parts[np].vy = tmp_vy;
						}
						else
						{
							sim->part_copy_properties(*storedPart, parts[np]);
						}
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
		Element_PRTI::orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
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
		Element_PRTI::orbitalparts_set(&parts[i].life, &parts[i].ctype, orbd, orbl);
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
	elem->Description = "Portal OUT. Particles come out here. Also has temperature dependent channels. (same as WIFI)";

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


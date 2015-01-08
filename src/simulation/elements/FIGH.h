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

#ifndef SIMULATION_ELEMENTS_FIGH_H
#define SIMULATION_ELEMENTS_FIGH_H 

#include "simulation/ElemDataSim.h"
#include "simulation/elements/STKM.h"

class FIGH_ElemDataSim : public STK_common_ElemDataSim
{
private:
	Stickman_data fighters[100];
	int usedCount;
	Observer_ClassMember<FIGH_ElemDataSim> obs_simCleared;
public:
	static const int maxFighters = 100;
	void Simulation_Cleared()
	{
		for (int i=0; i<maxFighters; i++)
		{
			fighters[i].reset();
		}
		usedCount = 0;
		storageActionPending = false;
	}
	FIGH_ElemDataSim(Simulation *s) :
		STK_common_ElemDataSim(s),
		usedCount(0),
		obs_simCleared(sim->hook_cleared, this, &FIGH_ElemDataSim::Simulation_Cleared)
	{
		Simulation_Cleared();
	}
	Stickman_data *GetFighterData(int i)
	{
		if (i>=0 && i<maxFighters)
			return fighters+i;
		else
			return NULL;
	}
	Stickman_data *GetFighterData(particle & part)
	{
		if (part.type==PT_FIGH && part.tmp>=0 && part.tmp<maxFighters)
			return &fighters[part.tmp];
		else
			return NULL;
	}
	void on_part_create(particle &p)
	{
		if (storageActionPending)
		{
			storageActionPending = false;
			return;
		}
		if (usedCount>=maxFighters)
		{
			p.tmp = -1;
			return;
		}
		int i = 0;
		while (i<maxFighters && fighters[i].exists()) i++;
		if (i<maxFighters)
		{
			p.tmp = i;
			fighters[i].set_particle(&p);
			fighters[i].elem = PT_DUST;
			fighters[i].rocketBoots = false;
			usedCount++;
		}
		else
		{
			p.tmp = -1;
		}
	}
	void on_part_kill(particle &p)
	{
		Stickman_data *playerp = GetFighterData(p);
		if (!playerp || playerp->part!=&p)
			return;
		playerp->set_particle(NULL);
		playerp->isStored = storageActionPending;
		storageActionPending = false;
	}
	bool create_allowed()
	{
		return (usedCount<maxFighters) || storageActionPending;
	}
};

#endif

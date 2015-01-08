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

#ifndef SIMULATION_ELEMENTS_PRTI_H
#define SIMULATION_ELEMENTS_PRTI_H 

#include "simulation/ElemDataSim.h"
#include "simulation/Simulation.h"
#include "powder.h"

extern const int portal_rx[8];
extern const int portal_ry[8];

class Element_PRTI
{
public:
	static int get_channel(particle *cpart)
	{
		int q = (int)((cpart->temp-73.15f)/100+1);
		if (q>=CHANNELS)
			return CHANNELS-1;
		else if (q<0)
			return 0;
		return q;
	}
};

class particle;

class PortalChannel
{
public:
	static const int storageSize = 80;
	int particleCount[8];
	particle portalp[8][80];
	// Store a particle in a given slot (one of the 8 neighbour positions) for this portal channel, then kills the original
	// Does not check whether the particle should be in a portal
	// Returns a pointer to the particle on success, or NULL if the portal is full
	particle * StoreParticle(Simulation *sim, int store_i, int slot);
	particle * AllocParticle(int slot);
	void ClearContents();
};

class PRTI_ElemDataSim : public ElemDataSim
{
private:
	PortalChannel channels[CHANNELS];
	Observer_ClassMember<PRTI_ElemDataSim> obs_simCleared;
public:
	PRTI_ElemDataSim(Simulation *s);
	void ClearPortalContents();
	PortalChannel* GetChannel(int i)
	{
		return channels+i;
	}
	PortalChannel* GetParticleChannel(particle &p)
	{
		p.tmp = (int)((p.temp-73.15f)/100+1);
		if (p.tmp>=CHANNELS) p.tmp = CHANNELS-1;
		else if (p.tmp<0) p.tmp = 0;
		return &channels[p.tmp];
	}

	static int GetPosIndex(int rx, int ry)
	{
		if (rx>1 || ry>1 || rx<-1 || ry<-1)
		{
			// scale down if larger than +-1
			float rmax = fmaxf(fabsf(rx), fabsf(ry));
			rx = rx/rmax;
			ry = ry/rmax;
		}
		for (int i=0; i<8; i++)
		{
			if (rx==portal_rx[i] && ry==portal_ry[i])
				return i;
		}
		return 1; // Dunno, put it in the top of the portal
	}

};

#endif

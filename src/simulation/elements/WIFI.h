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

#ifndef Simulation_Elements_WIFI_h
#define Simulation_Elements_WIFI_h 

#include "common/tptmath.h"
#include "simulation/elements-shared/ElemDataSim-channels.h"

class WifiChannel
{
public:
	bool activeThisFrame, activeNextFrame;
};

class WIFI_ElemDataSim : public ElemDataSim_channels
{
private:
	Observer_ClassMember<WIFI_ElemDataSim> obs_simCleared;
	Observer_ClassMember<WIFI_ElemDataSim> obs_simBeforeUpdate;
	WifiChannel *channels;
public:
	float channelStep;
	int channelCount;
	int GetChannelId(particle &p)
	{
		return tptmath::clamp_int(int((p.temp-73.15f)/channelStep+1), 0, channelCount-1);
	}
	WifiChannel* GetParticleChannel(particle &p)
	{
		p.tmp = tptmath::clamp_int(int((p.temp-73.15f)/channelStep+1), 0, channelCount-1);
		return &channels[p.tmp];
	}
	WIFI_ElemDataSim(Simulation *s, int t, float chanStep=100, int chanCount=-1);
	~WIFI_ElemDataSim();
	bool wifi_lastframe;
	void Simulation_Cleared();
	void Simulation_BeforeUpdate();
};

#endif

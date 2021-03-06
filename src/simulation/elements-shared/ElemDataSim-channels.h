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

#ifndef Simulation_ElementUI_ctypeWavelengths_h
#define Simulation_ElementUI_ctypeWavelengths_h

#include "simulation/ElemDataSim.h"

class particle;

class ElemDataSim_channels : public ElemDataSim
{
public:
	ElemDataSim_channels(Simulation *s, int t_) : ElemDataSim(s, t_) {}
	virtual int GetChannelId(particle &p) = 0;
};

#endif

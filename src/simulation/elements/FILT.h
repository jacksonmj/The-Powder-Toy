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

#ifndef Simulation_Elements_FILT_H
#define Simulation_Elements_FILT_H 

#include "simulation/Simulation.h"

class Element_FILT
{
public:
	// Returns the effective wavelengths value of a FILT particle
	// (ctype if set, otherwise temperature based wavelengths)
	static int getWavelengths(Simulation *sim, particle* cpart)
	{
		if (cpart->ctype&0x3FFFFFFF)
		{
			return cpart->ctype;
		}
		else
		{
			int temp_bin = (int)((cpart->temp-273.0f)*0.025f);
			if (temp_bin < 0) temp_bin = 0;
			if (temp_bin > 25) temp_bin = 25;
			return (0x1F << temp_bin);
		}
	}

	// Returns the wavelengths in a particle after FILT interacts with it (e.g. a photon)
	// cpart is the FILT particle, origWl the original wavelengths in the interacting particle
	static int interactWavelengths(Simulation *sim, particle *cpart, int origWl)
	{
		const int mask = 0x3FFFFFFF;
		int filtWl = getWavelengths(sim,cpart);
		switch (cpart->tmp)
		{
			case 0:
				return filtWl; //Assign Colour
			case 1:
				return origWl & filtWl; //Filter Colour
			case 2:
				return origWl | filtWl; //Add Colour
			case 3:
				return origWl & (~filtWl); //Subtract colour of filt from colour of photon
			case 4:
				{
					int shift = cpart->ctype;
					if (!shift)
						shift = int((cpart->temp-273.0f)*0.025f);
					if (shift<=0) shift = 1;
					return (origWl << shift) & mask; // red shift
				}
			case 5:
				{
					int shift = cpart->ctype;
					if (!shift)
						shift = int((cpart->temp-273.0f)*0.025f);
					if (shift<=0) shift = 1;
					return (origWl >> shift) & mask; // blue shift
				}
			case 6:
				return origWl; // No change
			case 7:
				return origWl ^ filtWl; // XOR colours
			case 8:
				return (~origWl) & mask; // Invert colours
			case 9:
			{
				int t1 = (origWl & 0x0000FF)+sim->rng.randInt<-2,2>();
				int t2 = ((origWl & 0x00FF00)>>8)+sim->rng.randInt<-2,2>();
				int t3 = ((origWl & 0xFF0000)>>16)+sim->rng.randInt<-2,2>();
				return (origWl & 0xFF000000) | (t3<<16) | (t2<<8) | t1;
			}
			default:
				return filtWl;
		}
	}
};

#endif

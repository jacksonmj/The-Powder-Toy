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

#ifndef Simulation_Elements_PHOT_h
#define Simulation_Elements_PHOT_h

#include "simulation/Simulation.h"

class Element_PHOT
{
private:
	// Used by photoelectric_effect() to create sparks when hitting a PN junction
	static void pn_junction_sprk(Simulation *sim, int x, int y)
	{
		int rcount, ri, rnext;
		FOR_PMAP_POSITION_NOENERGY(sim, x, y, rcount, ri, rnext)
		{
			if (sim->parts[ri].type==PT_PSCN || sim->parts[ri].type==PT_NSCN)
			{
				sim->spark_particle_conductiveOnly(ri, x, y);
			}
		}
	}
public:
	// Returns the index of the "middle" wavelength in a photon wavelengths value (used to determine refraction angle)
	// If too large a range of wavelengths are present, the wavelengths in the value are set to a random subset
	//   e.g. white photons -> photons with all different rainbow colours
	//        purple photons -> red and blue photons
	static int get_wavelength_bin(Simulation *sim, int *wm)
	{
		int i;
		int w0=30, wM=0;

		if (!(*wm & 0x3FFFFFFF))// no valid wavelengths present
			return -1;

		// find lowest bit set (w0) and highest bit set (wM)
		for (i=0; i<30; i++)
			if (*wm & (1<<i)) {
				if (i < w0)
					w0 = i;
				if (i > wM)
					wM = i;
			}

		if (wM-w0 < 5)// range of wavelengths is small enough, return index of middle
			return (wM+w0)/2;

		// Too large a range of wavelengths are present, choose a random subset
		i = sim->rng.randInt(0,wM-w0-4);
		i += w0;

		*wm &= 0x1F << i;
		return i + 2;
	}

	static void photoelectric_effect(Simulation *sim, int nx, int ny)//create sparks from PHOT when hitting PSCN and NSCN
	{
		// TODO: when breaking compatibility, also allow hitting NSCN with PSCN neighbour
		if (sim->pmap_find_one(nx,ny,PT_PSCN)>=0)
		{
			if (sim->pmap_find_one(nx-1,ny,PT_NSCN)>=0 ||
					sim->pmap_find_one(nx+1,ny,PT_NSCN)>=0 ||
					sim->pmap_find_one(nx,ny-1,PT_NSCN)>=0 ||
					sim->pmap_find_one(nx,ny+1,PT_NSCN)>=0)
			{
				pn_junction_sprk(sim, nx, ny);
			}
		}
	}

	static void create_cherenkov_photon(Simulation *sim, int pp);
	static void create_gain_photon(Simulation *sim, int pp);
};


#endif

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

#ifndef Simulation_Config_h
#define Simulation_Config_h

#define PT_NUM  220

#define XRES	612
#define YRES	384
#define NPART XRES*YRES

#define XCNTR   306
#define YCNTR   192

// TODO: should probably be using distance**2 instead wherever this is used, to avoid unnecessary sqrt
#define MAX_DISTANCE sqrt(pow(XRES, 2)+pow(YRES, 2))

#define CELL    4
#define ISTP    (CELL/2)
#define CFDS	(4.0f/CELL)


// TODO: some of these constants could probably go elsewhere, such as air or gravity or PHOT headers:

#define M_GRAV 6.67300e-1

#define AIR_TSTEPP 0.3f
#define AIR_TSTEPV 0.4f
#define AIR_VADV 0.3f
#define AIR_VLOSS 0.999f
#define AIR_PLOSS 0.9999f

//#define REALISTIC

#define NGOL 25
#define NGOLALT 24 //NGOL should be 24, but use this var until I find out why

#define SURF_RANGE     10
#define NORMAL_MIN_EST 3
#define NORMAL_INTERP  20
#define NORMAL_FRAC    16

#define REFRACT        0x80000000

/* heavy flint glass, for awesome refraction/dispersion
   this way you can make roof prisms easily */
#define GLASS_IOR      1.9
#define GLASS_DISP     0.07

#endif

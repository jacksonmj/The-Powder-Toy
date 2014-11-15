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

class Element_WIFI
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


#endif

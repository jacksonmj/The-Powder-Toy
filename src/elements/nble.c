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

#include <element.h>

int update_NBLE(UPDATE_FUNC_ARGS)
{
	if (parts[i].temp > 5273.15 && pv[y/CELL][x/CELL] > 100.0f)
	{
		if (rand()%5 < 1)
		{
			int j, r, rx, ry;
			float temp = parts[i].temp + 1750 + rand()%500;
			j = create_part(-3,x+rand()%3-1,y+rand()%3-1,PT_NEUT); if (j != -1) parts[j].temp = temp;
			if (!(rand()%25)) { j = create_part(-3,x+rand()%3-1,y+rand()%3-1,PT_ELEC); if (j != -1) parts[j].temp = temp; }
			j = create_part(-3,x+rand()%3-1,y+rand()%3-1,PT_PHOT);
			if (j != -1) { parts[j].ctype = 0xFF0000; parts[j].temp = temp; }

			j = create_part(i,x,y,PT_CO2);
			if (j != -1) parts[j].temp = temp;
			
			for (ry=-2; ry<3; ry++)
				for (rx=-2; rx<3; rx++)
					if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if ((r&0xFF)==PT_PLSM)
						{
							parts[r>>8].life = rand()%150+50;
							parts[r>>8].temp = temp;
						}
						else
						{
							j = create_part(-1, x+rx, y+ry, PT_PLSM);
							if (j != -1) parts[j].temp = temp;
						}
					}

			pv[y/CELL][x/CELL] += 50;
			return 1;
		}
	}
	return 0;
}

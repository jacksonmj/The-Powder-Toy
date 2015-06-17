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

int DRAY_update(UPDATE_FUNC_ARGS)
{
	int ctype = parts[i].ctype&0xFF, ctypeExtra = parts[i].ctype>>8, copyLength = parts[i].tmp, copySpaces = parts[i].tmp2;
	int ri, rcount, rnext;
	if (copySpaces < 0)
		copySpaces = parts[i].tmp2 = 0;
	if (copyLength < 0)
		copyLength = parts[i].tmp = 0;
	else if (copyLength > 0)
		copySpaces++; //strange hack
	if (!parts[i].life) // only fire when life is 0, but nothing sets the life right now
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
				{
					FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
					{
						if (parts[ri].type == PT_SPRK && parts[ri].life == 3) //spark found, start creating
						{
							bool overwrite = parts[ri].ctype == PT_PSCN;
							int partsRemaining = copyLength, xCopyTo, yCopyTo; //positions where the line will start being copied at

							if (parts[ri].ctype == PT_INWR && rx && ry) // INWR doesn't spark from diagonals
								continue;

							//figure out where the copying will start/end
							int rri, rrcount, rrnext;
							for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; ; xCurrent+=xStep, yCurrent+=yStep)
							{
								bool foundStopPart = false;
								bool foundSomething = false;
								if (!copyLength)
								{
									FOR_PMAP_POSITION(sim, xCurrent, yCurrent, rrcount, rri, rrnext)
									{
										foundSomething = true;
										if (parts[rri].type == ctype && (ctype != PT_LIFE || parts[rri].ctype == ctypeExtra))
										{
											foundStopPart = true;
											break;
										}
									}
									if (!ctype && !foundSomething)
										foundStopPart = true;
								}
								if (foundStopPart || !(--partsRemaining && sim->InBounds(xCurrent+xStep, yCurrent+yStep)))
								{
									copyLength -= partsRemaining;
									xCopyTo = xCurrent + xStep*copySpaces;
									yCopyTo = yCurrent + yStep*copySpaces;
									break;
								}
							}

							//now, actually copy the particles
							partsRemaining = copyLength + 1;
							for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; sim->InBounds(xCopyTo, yCopyTo) && --partsRemaining; xCurrent+=xStep, yCurrent+=yStep, xCopyTo+=xStep, yCopyTo+=yStep)
							{
								if (overwrite)
									sim->delete_position(xCopyTo, yCopyTo);
								FOR_PMAP_POSITION(sim, xCurrent, yCurrent, rrcount, rri, rrnext)
								{
									int type = parts[rri].type, p;
									if (type == PT_SPRK) //hack
										p = sim->part_create(-1, xCopyTo, yCopyTo, PT_METL);
									else
										p = sim->part_create(-1, xCopyTo, yCopyTo, type);
									if (p >= 0)
									{
										if (type == PT_SPRK)
											sim->part_change_type(p, xCopyTo, yCopyTo, PT_SPRK);
										sim->part_copy_properties(parts[rri], parts[p]);
									}
								}
							}
						}
					}
				}
	}
	return 0;
}

void DRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_DRAY";
	elem->ui->Name = "DRAY";
	elem->Colour = COLPACK(0xFFAA22);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->ui->Description = "Duplicator ray. Replicates a line of particles in front of it.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &DRAY_update;
	elem->Graphics = NULL;
}

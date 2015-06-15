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

int EMP_update(UPDATE_FUNC_ARGS)
{
	int r,rx,ry,rt,t,nx,ny;
	int rcount, ri, rnext;
	if (parts[i].life)
		return 0;
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				FOR_PMAP_POSITION_NOENERGY(sim, x+rx, y+ry, rcount, ri, rnext)
				{
					if (parts[ri].type==PT_SPRK && parts[ri].life>0 && parts[ri].life<4)
					{
						goto ok;
					}
				}
			}
	return 0;
ok:
	parts[i].life=220;
	emp_decor+=3;
	if (emp_decor>40)
		emp_decor=40;
	for (r=0; r<=sim->parts_lastActiveIndex; r++)
	{
		t=parts[r].type;
		rx=parts[r].x;
		ry=parts[r].y;
		if (t==PT_SPRK || (t==PT_SWCH && parts[r].life!=0 && parts[r].life!=10) || (t==PT_WIRE && parts[r].ctype>0))
		{
			int is_elec=0;
			if (parts[r].ctype==PT_PSCN || parts[r].ctype==PT_NSCN || parts[r].ctype==PT_PTCT ||
			        parts[r].ctype==PT_NTCT || parts[r].ctype==PT_INST || parts[r].ctype==PT_SWCH || t==PT_WIRE || t==PT_SWCH)
			{
				is_elec=1;
				if (sim->rng.chance<1,100>())
					sim->part_add_temp(parts[r], 3000.0f);
				if (sim->rng.chance<1,80>())
					sim->part_change_type(r, rx, ry, PT_BREL);
				else if (sim->rng.chance<1,120>())
					sim->part_change_type(r, rx, ry, PT_NTCT);
			}
			
			for (nx=-2; nx<3; nx++)
				for (ny=-2; ny<3; ny++)
					if (rx+nx>=0 && ry+ny>=0 && rx+nx<XRES && ry+ny<YRES && (rx || ry))
					{
						FOR_PMAP_POSITION_NOENERGY(sim, rx+nx, ry+ny, rcount, ri, rnext)
						{
							rt = parts[ri].type;

							//Some elements should only be affected by wire/swch, or by a spark on inst/semiconductor
							//So not affected by spark on metl, watr etc
							if (is_elec)
							{
								switch (rt)
								{
								case PT_METL:
									if (sim->rng.chance<1,280>())
										sim->part_add_temp(parts[ri], 3000.0f);
									if (sim->rng.chance<1,300>())
										sim->part_change_type(ri, rx+nx, ry+ny, PT_BMTL);
									break;
								case PT_BMTL:
									if (sim->rng.chance<1,280>())
										sim->part_add_temp(parts[ri], 3000.0f);
									if (sim->rng.chance<1,160>())
									{
										sim->part_change_type(ri, rx+nx, ry+ny, PT_BRMT);
										sim->part_add_temp(parts[ri], 1000.0f);
									}
									break;
								case PT_WIFI:
									if (sim->rng.chance<1,8>())
									{
										//Randomise channel
										sim->part_set_temp(parts[ri],sim->rng.randInt<MIN_TEMP,MAX_TEMP>());
									}
									if (sim->rng.chance<1,16>())
									{
										sim->part_create(ri, rx+nx, ry+ny, PT_BREL);
										sim->part_add_temp(parts[ri], 1000.0f);
									}
									break;
								default:
									break;
								}
							}
							switch (rt)
							{
							case PT_SWCH:
								if (sim->rng.chance<1,100>())
									sim->part_change_type(ri, rx+nx, ry+ny, PT_BREL);
								if (sim->rng.chance<1,100>())
									sim->part_add_temp(parts[ri], 2000.0f);
								break;
							case PT_ARAY:
								if (rt==PT_ARAY && sim->rng.chance<1,60>())
								{
									sim->part_create(ri, rx+nx, ry+ny, PT_BREL);
									sim->part_add_temp(parts[ri], 1000.0f);
								}
								break;
							case PT_DLAY:
								if (rt==PT_DLAY && sim->rng.chance<1,70>())
								{
									//Randomise delay
									sim->part_set_temp(parts[ri], sim->rng.randInt<0,255>() + 273.15f);
								}
								break;
							default:
								break;
							}
						}
					}
		}
	}
	return 0;
}

int EMP_graphics(GRAPHICS_FUNC_ARGS)
{
	if(cpart->life)
	{
		*colr = cpart->life*1.5;
		*colg = cpart->life*1.5;
		*colb = 200-(cpart->life);
		if (*colr>255)
			*colr = 255;
		if (*colg>255)
			*colg = 255;
		if (*colb>255)
			*colb = 255;
		if (*colb<=0)
			*colb = 0;
	}
	return 0;
}

void EMP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->ui_create<Element_UI>();

	elem->Identifier = "DEFAULT_PT_EMP";
	elem->ui->Name = "EMP";
	elem->Colour = COLPACK(0x66AAFF);
	elem->ui->MenuVisible = 1;
	elem->ui->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.0f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 3;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 121;
	elem->Latent = 0;
	elem->ui->Description = "Electromagnetic pulse. Breaks activated electronics.";

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

	elem->Update = &EMP_update;
	elem->Graphics = &EMP_graphics;
}


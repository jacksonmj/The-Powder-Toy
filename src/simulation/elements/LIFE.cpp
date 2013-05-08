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

gol_menu gmenu[NGOL] =
{
	{"GOL",		PIXPACK(0x0CAC00), 0, "Game Of Life B3/S23"},
	{"HLIF",	PIXPACK(0xFF0000), 1, "High Life B36/S23"},
	{"ASIM",	PIXPACK(0x0000FF), 2, "Assimilation B345/S4567"},
	{"2x2",		PIXPACK(0xFFFF00), 3, "2x2 B36/S125"},
	{"DANI",	PIXPACK(0x00FFFF), 4, "Day and Night B3678/S34678"},
	{"AMOE",	PIXPACK(0xFF00FF), 5, "Amoeba B357/S1358"},
	{"MOVE",	PIXPACK(0xFFFFFF), 6, "'Move' particles. Does not move things.. it is a life type B368/S245"},
	{"PGOL",	PIXPACK(0xE05010), 7, "Pseudo Life B357/S238"},
	{"DMOE",	PIXPACK(0x500000), 8, "Diamoeba B35678/S5678"},
	{"34",		PIXPACK(0x500050), 9, "34 B34/S34"},
	{"LLIF",	PIXPACK(0x505050), 10, "Long Life B345/S5"},
	{"STAN",	PIXPACK(0x5000FF), 11, "Stains B3678/S235678"},
	{"SEED",	PIXPACK(0xFBEC7D), 12, "B2/S"},
	{"MAZE",	PIXPACK(0xA8E4A0), 13, "B3/S12345"},
	{"COAG",	PIXPACK(0x9ACD32), 14, "B378/S235678"},
	{"WALL",	PIXPACK(0x0047AB), 15, "B45678/S2345"},
	{"GNAR",	PIXPACK(0xE5B73B), 16, "B1/S1"},
	{"REPL",	PIXPACK(0x259588), 17, "B1357/S1357"},
	{"MYST",	PIXPACK(0x0C3C00), 18, "B3458/S05678"},
	{"LOTE",	PIXPACK(0xFF0000), 19, "Behaves kinda like Living on the Edge S3458/B37/4"},
	{"FRG2",	PIXPACK(0x00FF00), 20, "Like Frogs rule S124/B3/3"},
	{"STAR",	PIXPACK(0x0000FF), 21, "Like Star Wars rule S3456/B278/6"},
	{"FROG",	PIXPACK(0x00AA00), 22, "Frogs S12/B34/3"},
	{"BRAN",	PIXPACK(0xCCCC00), 23, "Brian 6 S6/B246/3"}
};

int grule[NGOL+1][10] =
{
//	 0,1,2,3,4,5,6,7,8,STATES    live=1  spawn=2 spawn&live=3   States are kind of how long until it dies, normal ones use two states(living,dead) for others the intermediate states live but do nothing
	{0,0,0,0,0,0,0,0,0,2},//blank
	{0,0,1,3,0,0,0,0,0,2},//GOL
	{0,0,1,3,0,0,2,0,0,2},//HLIF
	{0,0,0,2,3,3,1,1,0,2},//ASIM
	{0,1,1,2,0,1,2,0,0,2},//2x2
	{0,0,0,3,1,0,3,3,3,2},//DANI
	{0,1,0,3,0,3,0,2,1,2},//AMOE
	{0,0,1,2,1,1,2,0,2,2},//MOVE
	{0,0,1,3,0,2,0,2,1,2},//PGOL
	{0,0,0,2,0,3,3,3,3,2},//DMOE
	{0,0,0,3,3,0,0,0,0,2},//34
	{0,0,0,2,2,3,0,0,0,2},//LLIF
	{0,0,1,3,0,1,3,3,3,2},//STAN
	{0,0,2,0,0,0,0,0,0,2},//SEED
	{0,1,1,3,1,1,0,0,0,2},//MAZE
	{0,0,1,3,0,1,1,3,3,2},//COAG
	{0,0,1,1,3,3,2,2,2,2},//WALL
	{0,3,0,0,0,0,0,0,0,2},//GNAR
	{0,3,0,3,0,3,0,3,0,2},//REPL
	{1,0,0,2,2,3,1,1,3,2},//MYST
	{0,0,0,3,1,1,0,2,1,4},//LOTE
	{0,1,1,2,1,0,0,0,0,3},//FRG2
	{0,0,2,1,1,1,1,2,2,6},//STAR
	{0,1,1,2,2,0,0,0,0,3},//FROG
	{0,0,2,0,2,0,3,0,0,3},//BRAN
};

int goltype[NGOL] =
{
	GT_GOL,
	GT_HLIF,
	GT_ASIM,
	GT_2x2,
	GT_DANI,
	GT_AMOE,
	GT_MOVE,
	GT_PGOL,
	GT_DMOE,
	GT_34,
	GT_LLIF,
	GT_STAN,
	GT_SEED,
	GT_MAZE,
	GT_COAG,
	GT_WALL,
	GT_GNAR,
	GT_REPL,
	GT_MYST,
	GT_LOTE,
	GT_FRG2,
	GT_STAR,
	GT_FROG,
	GT_BRAN,
};

int LIFE_graphics(GRAPHICS_FUNC_ARGS)
{
	pixel pc;
	if (cpart->ctype==NGT_LOTE)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(255, 128, 0);
		else if (cpart->tmp==1)
			pc = PIXRGB(255, 255, 0);
		else
			pc = PIXRGB(255, 0, 0);
	}
	else if (cpart->ctype==NGT_FRG2)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 50);
		else
			pc = PIXRGB(0, 255, 90);
	}
	else if (cpart->ctype==NGT_STAR)//colors for life states
	{
		if (cpart->tmp==4)
			pc = PIXRGB(0, 0, 128);
		else if (cpart->tmp==3)
			pc = PIXRGB(0, 0, 150);
		else if (cpart->tmp==2)
			pc = PIXRGB(0, 0, 190);
		else if (cpart->tmp==1)
			pc = PIXRGB(0, 0, 230);
		else
			pc = PIXRGB(0, 0, 70);
	}
	else if (cpart->ctype==NGT_FROG)//colors for life states
	{
		if (cpart->tmp==2)
			pc = PIXRGB(0, 100, 0);
		else
			pc = PIXRGB(0, 255, 0);
	}
	else if (cpart->ctype==NGT_BRAN)//colors for life states
	{
		if (cpart->tmp==1)
			pc = PIXRGB(150, 150, 0);
		else
			pc = PIXRGB(255, 255, 0);
	} else {
		pc = gmenu[cpart->ctype].colour;
	}
	*colr = PIXR(pc);
	*colg = PIXG(pc);
	*colb = PIXB(pc);
	return 0;
}

void LIFE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LIFE";
	elem->Name = "LIFE";
	elem->Colour = COLPACK(0x0CAC00);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_LIFE;
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
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 9000.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->Description = "Game Of Life! B3/S23";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID|PROP_LIFE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &LIFE_graphics;
}


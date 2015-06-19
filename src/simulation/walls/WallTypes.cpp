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


#include "simulation/walls/WallsData.hpp"
#include "simulation/walls/WallTypes.hpp"
#include "simulation/WallNumbers.hpp"

#define WallNumbers_DeclInit
#define DEFINE_WALL(name, id) void WL_ ## name ## _init(WALL_INIT_FUNC_ARGS);
#include "simulation/WallNumbers.hpp"

void WallTypes::init(SimulationSharedData *simSD)
{
	#define DEFINE_WALL(name, id) if (id>=0 && id<WL_NUM) { WL_ ## name ## _init(simSD, &wt[id], id); };
	#define WallNumbers_CallInit
	#include "simulation/WallNumbers.hpp"
}

WallTypes::WallTypes() :
	wt(new WallType[WL_NUM])
{}

WallTypes::~WallTypes()
{
	delete[] wt;
}

uint8_t WallTypes::convertLegacyId(uint8_t w)
{
	for (uint8_t i=0; i<WL_NUM; i++)
	{
		if (wt[i].LegacyId==w)
			return i;
	}
	return w;
}

uint8_t WallTypes::convertV44Id(uint8_t w)
{
	for (uint8_t i=0; i<WL_NUM; i++)
	{
		if (wt[i].V44Id==w)
			return i;
	}
	return w;
}


WallType::WallType() :
	Colour2(COLPACK(0x000000)),
	Colour_ElecGlow(COLPACK(0x000000)),
	LegacyId(-1),
	V44Id(-1)
{}

void WL_ERASE_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_ERASE";
	wall->Name = "ERASE";
	wall->Description = "Erases walls.";
	wall->Colour = COLPACK(0x808080);
	wall->DrawStyle = WallType_DrawStyle::SPECIAL;
}

void WL_WALLELEC_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_CNDTW";
	wall->Name = "CONDUCTIVE WALL";
	wall->Description = "Blocks everything. Conductive.";
	wall->Colour = COLPACK(0xC0C0C0);
	wall->Colour_ElecGlow = COLPACK(0x101010);
	wall->DrawStyle = WallType_DrawStyle::SPECIAL;
	wall->LegacyId = 122;
	wall->V44Id = 8;
}

void WL_EWALL_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_EWALL";
	wall->Name = "EWALL";
	wall->Description = "E-Wall. Becomes transparent when electricity is connected.";
	wall->Colour = COLPACK(0x808080);
	wall->Colour_ElecGlow = COLPACK(0x808080);
	wall->DrawStyle = WallType_DrawStyle::SPECIAL;
	wall->LegacyId = 123;
	wall->V44Id = 7;
}

void WL_DETECT_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_DTECT";
	wall->Name = "DETECTOR";
	wall->Description = "Detector. Generates electricity when a particle is inside.";
	wall->Colour = COLPACK(0xFF8080);
	wall->Colour_ElecGlow = COLPACK(0xFF2008);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 124;
	wall->V44Id = 6;
}

void WL_STREAM_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_STRM";
	wall->Name = "STREAMLINE";
	wall->Description = "Streamline. Set start point of a streamline.";
	wall->Colour = COLPACK(0x808080);
	wall->DrawStyle = WallType_DrawStyle::SPECIAL;
	wall->LegacyId = 125;
	wall->V44Id = 5;
}

void WL_FAN_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_FAN";
	wall->Name = "FAN";
	wall->Description = "Fan. Accelerates air. Use the line tool to set direction and strength.";
	wall->Colour = COLPACK(0x8080FF);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 127;
	wall->V44Id = 4;
}

void WL_ALLOWLIQUID_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_LIQD";
	wall->Name = "LIQUID WALL";
	wall->Description = "Allows liquids, blocks all other particles. Conductive.";
	wall->Colour = COLPACK(0xC0C0C0);
	wall->Colour_ElecGlow = COLPACK(0x101010);
	wall->DrawStyle = WallType_DrawStyle::DOTS_ALIGNED;
	wall->LegacyId = 128;
	wall->V44Id = 3;
}

void WL_DESTROYALL_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_ABSRB";
	wall->Name = "ABSORB WALL";
	wall->Description = "Absorbs particles but lets air currents through.";
	wall->Colour = COLPACK(0x808080);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 129;
	wall->V44Id = 2;
}

void WL_WALL_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_WALL";
	wall->Name = "WALL";
	wall->Description = "Basic wall, blocks everything.";
	wall->Colour = COLPACK(0x808080);
	wall->DrawStyle = WallType_DrawStyle::SOLID;
	wall->LegacyId = 131;
	wall->V44Id = 1;
}

void WL_ALLOWAIR_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_AIR";
	wall->Name = "AIRONLY WALL";
	wall->Description = "Allows air, but blocks all particles.";
	wall->Colour = COLPACK(0x3C3C3C);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 132;
	wall->V44Id = 9;
}

void WL_ALLOWSOLID_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_POWDR";
	wall->Name = "POWDER WALL";
	wall->Description = "Allows powders, blocks all other particles.";
	wall->Colour = COLPACK(0x575757);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 133;
	wall->V44Id = 10;
}

void WL_ALLOWALLELEC_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_CNDTR";
	wall->Name = "CONDUCTOR";
	wall->Description = "Conductor. Allows all particles to pass through and conducts electricity.";
	wall->Colour = COLPACK(0xFFFF22);
	wall->Colour_ElecGlow = COLPACK(0x101010);
	wall->DrawStyle = WallType_DrawStyle::DOTS_ALIGNED;
	wall->LegacyId = 134;
	wall->V44Id = 11;
}

void WL_EHOLE_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_EHOLE";
	wall->Name = "EHOLE";
	wall->Description = "E-Hole. absorbs particles, releases them when powered.";
	wall->Colour = COLPACK(0x242424);
	wall->Colour_ElecGlow = COLPACK(0x101010);
	wall->DrawStyle = WallType_DrawStyle::SPECIAL;
	wall->LegacyId = 135;
	wall->V44Id = 12;
}

void WL_ALLOWGAS_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_GAS";
	wall->Name = "GAS WALL";
	wall->Description = "Allows gases, blocks all other particles.";
	wall->Colour = COLPACK(0x579777);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 140;
	wall->V44Id = 13;
}

void WL_GRAV_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_GRVTY";
	wall->Name = "GRAVITY WALL";
	wall->Description = "Gravity wall. Newtonian Gravity has no effect inside a box drawn with this.";
	wall->Colour = COLPACK(0xFFEE00);
	wall->Colour2 = COLPACK(0xAA9900);
	wall->DrawStyle = WallType_DrawStyle::STRIPES;
	wall->LegacyId = 142;
}

void WL_ALLOWENERGY_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_ENRGY";
	wall->Name = "ENERGY WALL";
	wall->Description = "Allows energy particles, blocks all other particles.";
	wall->Colour = COLPACK(0xFFAA00);
	wall->Colour2 = COLPACK(0xAA5500);
	wall->DrawStyle = WallType_DrawStyle::STRIPES;
	wall->LegacyId = 145;
}

void WL_BLOCKAIR_init(WALL_INIT_FUNC_ARGS)
{
	wall->Identifier = "DEFAULT_WL_NOAIR";
	wall->Name = "AIRBLOCK WALL";
	wall->Description = "Allows all particles, but blocks air.";
	wall->Colour = COLPACK(0xDCDCDC);
	wall->DrawStyle = WallType_DrawStyle::DOTS_OFFSET;
	wall->LegacyId = 147;
}


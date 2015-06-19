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

#ifndef simulation_walls_WallType_h
#define simulation_walls_WallType_h

#include "graphics/ARGBColour.h"
#include <string>

class Simulation;
class SimulationSharedData;

enum class WallType_DrawStyle
{
	SPECIAL,
	DOTS_OFFSET,
	DOTS_ALIGNED,
	SOLID,
	STRIPES
};

class WallType
{
public:
	ARGBColour Colour;
	ARGBColour Colour2;
	ARGBColour Colour_ElecGlow;// if emap set, add this to fire glow
	WallType_DrawStyle DrawStyle;
	int LegacyId;
	int V44Id;	
	std::string Name;
	std::string Description;
	std::string Identifier;
	WallType();
};

class WallTypes
{
protected:
	WallType *wt;
public:
	WallTypes();
	virtual ~WallTypes();
	void init(SimulationSharedData *simSD);
	WallType& operator[] (size_t w) { return wt[w]; }
	uint8_t convertLegacyId(uint8_t w);
	uint8_t convertV44Id(uint8_t w);
};

#endif

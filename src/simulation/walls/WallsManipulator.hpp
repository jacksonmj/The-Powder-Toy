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

#ifndef simulation_walls_WallsManipulator_h
#define simulation_walls_WallsManipulator_h

#include "simulation/CellsData.hpp"
#include "simulation/WallNumbers.hpp"
#include "simulation/walls/WallsData.hpp"
#include "simulation/Config.hpp"

class WallsManipulator
{
private:
	WallsDataP data;
public:
	int gravwl_timeout;

	WallsDataP getDataPtr() { return data; }

	void setManipData(WallsDataP data_);
	WallsManipulator();
	virtual ~WallsManipulator();

	float fanVelX(SimPosCell c) const {
		return data.fanVX[c.y][c.x];
	}
	float fanVelY(SimPosCell c) const {
		return data.fanVY[c.y][c.x];
	}
	void fanVelX(SimPosCell c, float newValue) {
		data.fanVX[c.y][c.x] = newValue;
	}
	void fanVelY(SimPosCell c, float newValue) {
		data.fanVY[c.y][c.x] = newValue;
	}
	void fanVel(SimPosCell c, float newVX, float newVY) {
		data.fanVX[c.y][c.x] = newVX;
		data.fanVY[c.y][c.x] = newVY;
	}
	void fanVel_flood(SimPosCell c, float newVX, float newVY);

	uint8_t type(SimPosCell c) const {
		return data.wallType[c.y][c.x];
	}
	void type(SimPosCell c, uint8_t newType) {
		if (data.wallType[c.y][c.x]==WL_GRAV && newType!=WL_GRAV)
			gravwl_timeout = 60;
		if (newType==WL_FAN && data.wallType[c.y][c.x]!=WL_FAN)
			fanVel(c, 0.0f, 0.0f);
		data.wallType[c.y][c.x] = newType;
		if (newType==WL_GRAV)
			gravwl_timeout = 60;
	}
	void type(SimAreaCell area, uint8_t newType);
	uint8_t electricity(SimPosCell c) const {
		return data.electricity[c.y][c.x];
	}
	void electricity(SimPosCell c, uint8_t newValue) {
		data.electricity[c.y][c.x] = newValue;
	}

	bool isProperWall(SimPosCell c) const // is a wall, and is not WL_STREAM
	{
		return (type(c) && type(c)!=WL_STREAM);
	}
	bool isClosedEHole(SimPosCell c) const
	{
		return (type(c)==WL_EHOLE && !electricity(c));
	}
	bool isConductive(SimPosCell c) const
	{
		return (type(c)==WL_DETECT || type(c)==WL_EWALL || type(c)==WL_ALLOWLIQUID || type(c)==WL_WALLELEC || type(c)==WL_ALLOWALLELEC || type(c)==WL_EHOLE);
	}
	bool canSpark(SimPosCell c) const
	{
		return isConductive(c) && electricity(c)<8;
	}
	void detect(SimPosCell c) // trigger WL_DETECT, if present at this location
	{
		if (type(c)==WL_DETECT && electricity(c)<8)
			makeSpark(c);
	}
	void makeSpark(SimPosCell c);
	void setBorder(uint8_t wallType);
};


#endif

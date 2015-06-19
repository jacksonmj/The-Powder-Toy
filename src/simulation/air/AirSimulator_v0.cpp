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

#include "simulation/air/AirSimulator_v0.hpp"
#include "common/tptmath.h"
#include <cmath>

#include "powder.h" // for WL_FAN

void AirSimulator_v0::sim_impl_rwInput(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	update_air(inputData, outputData, params);
	if(params.ambientHeatEnabled)
		update_airh(inputData, outputData, params);
	else
		CellsData_copy<float>(inputData.hv, outputData.hv);
}

void AirSimulator_v0::update_airh(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	int gravityMode = params.gravityMode;
	CellsFloatRP hv = inputData.hv;
	CellsFloatRP vx = outputData.vx;
	CellsFloatRP vy = outputData.vy;
	CellsFloatRP pv = outputData.pv;
	CellsFloatRP ohv = outputData.hv;
	const_CellsUCharRP bmap_blockairh = params.blockairh;

	int x, y, i, j;
	float odh, dh, dx, dy, f, tx, ty;

	for (i=0; i<YRES/CELL; i++) //reduces pressure/velocity on the edges every frame
	{
		hv[i][0] = 295.15f;
		hv[i][1] = 295.15f;
		hv[i][XRES/CELL-3] = 295.15f;
		hv[i][XRES/CELL-2] = 295.15f;
		hv[i][XRES/CELL-1] = 295.15f;
	}
	for (i=0; i<XRES/CELL; i++) //reduces pressure/velocity on the edges every frame
	{
		hv[0][i] = 295.15f;
		hv[1][i] = 295.15f;
		hv[YRES/CELL-3][i] = 295.15f;
		hv[YRES/CELL-2][i] = 295.15f;
		hv[YRES/CELL-1][i] = 295.15f;
	}
	for (y=0; y<YRES/CELL; y++) //update velocity and pressure
	{
		for (x=0; x<XRES/CELL; x++)
		{
			dh = 0.0f;
			dx = 0.0f;
			dy = 0.0f;
			for (j=-1; j<2; j++)
			{
				for (i=-1; i<2; i++)
				{
					if (y+j>0 && y+j<YRES/CELL-2 &&
							x+i>0 && x+i<XRES/CELL-2 &&
							!(bmap_blockairh[y+j][x+i]&0x8))
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y+j][x+i]*f;
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y][x]*f;
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
					}
				}
			}
			tx = x - dx*0.7f;
			ty = y - dy*0.7f;
			i = (int)tx;
			j = (int)ty;
			tx -= i;
			ty -= j;
			if (i>=2 && i<XRES/CELL-3 && j>=2 && j<YRES/CELL-3)
			{
				odh = dh;
				dh *= 1.0f - AIR_VADV;
				dh += AIR_VADV*(1.0f-tx)*(1.0f-ty)*((bmap_blockairh[j][i]&0x8) ? odh : hv[j][i]);
				dh += AIR_VADV*tx*(1.0f-ty)*((bmap_blockairh[j][i+1]&0x8) ? odh : hv[j][i+1]);
				dh += AIR_VADV*(1.0f-tx)*ty*((bmap_blockairh[j+1][i]&0x8) ? odh : hv[j+1][i]);
				dh += AIR_VADV*tx*ty*((bmap_blockairh[j+1][i+1]&0x8) ? odh : hv[j+1][i+1]);
			}
			pv[y][x] += (dh-hv[y][x])/5000.0f;
			if(!gravityMode){ //Vertical gravity only for the time being
				float airdiff = hv[y-1][x]-hv[y][x];
				if(airdiff>0 && !(bmap_blockairh[y-1][x]&0x8))
					vy[y][x] -= airdiff/5000.0f;
			}
			ohv[y][x] = dh;
		}
	}
}

void AirSimulator_v0::update_air(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	int airMode = params.airMode;
	CellsFloatRP vx = inputData.vx;
	CellsFloatRP vy = inputData.vy;
	CellsFloatRP pv = inputData.pv;
	CellsFloatRP ovx = outputData.vx;
	CellsFloatRP ovy = outputData.vy;
	CellsFloatRP opv = outputData.pv;
	const_CellsFloatRP fvx = params.wallsData.fanVX;
	const_CellsFloatRP fvy = params.wallsData.fanVY;
	const_CellsUCharRP bmap_blockair = params.blockair;
	const_CellsUCharRP bmap = params.wallsData.wallType;

	int x, y, i, j;
	float dp, dx, dy, f, tx, ty;
	const float advDistanceMult = 0.7f;
	float stepX, stepY;
	int stepLimit, step;

	if (airMode == 4) { //airMode 4 is no air/pressure update
		AirData_copy(inputData, outputData);
		return;
	}

	// DIFFERENCE TO TPT++: order of vx[RES/CELL-2] and vx[RES/CELL-1] has been reversed to avoid dependency of -1 on -2 on -3
	for (i=0; i<YRES/CELL; i++) //reduces pressure/velocity on the edges every frame
	{
		pv[i][0] = pv[i][0]*0.8f;
		pv[i][1] = pv[i][1]*0.8f;
		pv[i][2] = pv[i][2]*0.8f;
		pv[i][XRES/CELL-2] = pv[i][XRES/CELL-2]*0.8f;
		pv[i][XRES/CELL-1] = pv[i][XRES/CELL-1]*0.8f;
		vx[i][0] = vx[i][1]*0.9f;
		vx[i][1] = vx[i][2]*0.9f;
		vx[i][XRES/CELL-1] = vx[i][XRES/CELL-2]*0.9f;
		vx[i][XRES/CELL-2] = vx[i][XRES/CELL-3]*0.9f;
		vy[i][0] = vy[i][1]*0.9f;
		vy[i][1] = vy[i][2]*0.9f;
		vy[i][XRES/CELL-1] = vy[i][XRES/CELL-2]*0.9f;
		vy[i][XRES/CELL-2] = vy[i][XRES/CELL-3]*0.9f;
	}
	for (i=0; i<XRES/CELL; i++) //reduces pressure/velocity on the edges every frame
	{
		pv[0][i] = pv[0][i]*0.8f;
		pv[1][i] = pv[1][i]*0.8f;
		pv[2][i] = pv[2][i]*0.8f;
		pv[YRES/CELL-2][i] = pv[YRES/CELL-2][i]*0.8f;
		pv[YRES/CELL-1][i] = pv[YRES/CELL-1][i]*0.8f;
		vx[0][i] = vx[1][i]*0.9f;
		vx[1][i] = vx[2][i]*0.9f;
		vx[YRES/CELL-1][i] = vx[YRES/CELL-2][i]*0.9f;
		vx[YRES/CELL-2][i] = vx[YRES/CELL-3][i]*0.9f;
		vy[0][i] = vy[1][i]*0.9f;
		vy[1][i] = vy[2][i]*0.9f;
		vy[YRES/CELL-1][i] = vy[YRES/CELL-2][i]*0.9f;
		vy[YRES/CELL-2][i] = vy[YRES/CELL-3][i]*0.9f;
	}

	for (j=1; j<YRES/CELL; j++) //clear some velocities near walls
	{
		for (i=1; i<XRES/CELL; i++)
		{
			if (bmap_blockair[j][i])
			{
				vx[j][i] = 0.0f;
				vx[j][i-1] = 0.0f;
				vy[j][i] = 0.0f;
				vy[j-1][i] = 0.0f;
			}
		}
	}

	for (y=1; y<YRES/CELL; y++) //pressure adjustments from velocity
		for (x=1; x<XRES/CELL; x++)
		{
			dp = 0.0f;
			dp += vx[y][x-1] - vx[y][x];
			dp += vy[y-1][x] - vy[y][x];
			pv[y][x] *= AIR_PLOSS;
			pv[y][x] += dp*AIR_TSTEPP;
		}

	for (y=0; y<YRES/CELL-1; y++) //velocity adjustments from pressure
		for (x=0; x<XRES/CELL-1; x++)
		{
			dx = dy = 0.0f;
			dx += pv[y][x] - pv[y][x+1];
			dy += pv[y][x] - pv[y+1][x];
			vx[y][x] *= AIR_VLOSS;
			vy[y][x] *= AIR_VLOSS;
			vx[y][x] += dx*AIR_TSTEPV;
			vy[y][x] += dy*AIR_TSTEPV;
			if (bmap_blockair[y][x] || bmap_blockair[y][x+1])
				vx[y][x] = 0;
			if (bmap_blockair[y][x] || bmap_blockair[y+1][x])
				vy[y][x] = 0;
		}

	for (y=0; y<YRES/CELL; y++) //update velocity and pressure
		for (x=0; x<XRES/CELL; x++)
		{
			dx = 0.0f;
			dy = 0.0f;
			dp = 0.0f;
			for (j=-1; j<2; j++)
				for (i=-1; i<2; i++)
					if (y+j>0 && y+j<YRES/CELL-1 &&
							x+i>0 && x+i<XRES/CELL-1 &&
							!bmap_blockair[y+j][x+i])
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
						dp += pv[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
						dp += pv[y][x]*f;
					}

			tx = x - dx*advDistanceMult;
			ty = y - dy*advDistanceMult;
			if ((dx*advDistanceMult>1.0f || dy*advDistanceMult>1.0f) && (tx>=2 && tx<XRES/CELL-2 && ty>=2 && ty<YRES/CELL-2))
			{
				// Trying to take velocity from far away, check whether there is an intervening wall. Step from current position to desired source location, looking for walls, with either the x or y step size being 1 cell
				if (fabsf(dx)>fabsf(dy))
				{
					stepX = (dx<0.0f) ? 1 : -1;
					stepY = -dy/fabsf(dx);
					stepLimit = (int)(fabsf(dx*advDistanceMult));
				}
				else
				{
					stepY = (dy<0.0f) ? 1 : -1;
					stepX = -dx/fabsf(dy);
					stepLimit = (int)(fabsf(dy*advDistanceMult));
				}
				tx = x;
				ty = y;
				for (step=0; step<stepLimit; ++step)
				{
					tx += stepX;
					ty += stepY;
					if (bmap_blockair[(int)(ty+0.5f)][(int)(tx+0.5f)])
					{
						tx -= stepX;
						ty -= stepY;
						break;
					}
				}
				if (step==stepLimit)
				{
					// No wall found
					tx = x - dx*advDistanceMult;
					ty = y - dy*advDistanceMult;
				}
			}
			i = (int)tx;
			j = (int)ty;
			tx -= i;
			ty -= j;
			if (!bmap_blockair[y][x] && i>=2 && i<=XRES/CELL-3 && j>=2 && j<=YRES/CELL-3)
			{
				dx *= 1.0f - AIR_VADV;
				dy *= 1.0f - AIR_VADV;

				dx += AIR_VADV*(1.0f-tx)*(1.0f-ty)*vx[j][i];
				dy += AIR_VADV*(1.0f-tx)*(1.0f-ty)*vy[j][i];

				dx += AIR_VADV*tx*(1.0f-ty)*vx[j][i+1];
				dy += AIR_VADV*tx*(1.0f-ty)*vy[j][i+1];

				dx += AIR_VADV*(1.0f-tx)*ty*vx[j+1][i];
				dy += AIR_VADV*(1.0f-tx)*ty*vy[j+1][i];

				dx += AIR_VADV*tx*ty*vx[j+1][i+1];
				dy += AIR_VADV*tx*ty*vy[j+1][i+1];
			}

			if (bmap[y][x] == WL_FAN)
			{
				dx += fvx[y][x];
				dy += fvy[y][x];
			}
			// pressure/velocity caps
			dp = tptmath::clamp_flt(dp, -256.0f, 256.0f);
			dx = tptmath::clamp_flt(dx, -256.0f, 256.0f);
			dy = tptmath::clamp_flt(dy, -256.0f, 256.0f);

			switch (airMode)
			{
			default:
			case 0:  //Default
				break;
			case 1:  //0 Pressure
				dp = 0.0f;
				break;
			case 2:  //0 Velocity
				dx = 0.0f;
				dy = 0.0f;
				break;
			case 3: //0 Air
				dx = 0.0f;
				dy = 0.0f;
				dp = 0.0f;
				break;
			case 4: //No Update
				break;
			}

			ovx[y][x] = dx;
			ovy[y][x] = dy;
			opv[y][x] = dp;
		}
}


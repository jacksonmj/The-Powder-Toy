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

#include "simulation/air/AirSimulator_v1.hpp"
#include "common/tptmath.h"
#include <cmath>

#include "simulation/WallNumbers.hpp"
#include "simulation/CellsData_fastloop.hpp"

// TPT_NOINLINE is used in some of these functions (the ones which aren't called inside a loop) to make it easier to use callgrind to track where time is spent

TPT_NOINLINE void AirSimulator_v1::setEdges_h(CellsFloatRP data, float value)
{
	// set temperature at the edges to ambient
	for (int i=0; i<YRES/CELL; i++)
	{
		data[i][0] = value;
		data[i][1] = value;
		data[i][XRES/CELL-3] = value;
		data[i][XRES/CELL-2] = value;
		data[i][XRES/CELL-1] = value;
	}
	for (int i=0; i<XRES/CELL; i++)
	{
		data[0][i] = value;
		data[1][i] = value;
		data[YRES/CELL-3][i] = value;
		data[YRES/CELL-2][i] = value;
		data[YRES/CELL-1][i] = value;
	}
}

TPT_NOINLINE void AirSimulator_v1::reduceEdges_p(CellsFloatRP data, float multiplier)
{
	//reduce pressure at the edges
	for (int i=0; i<YRES/CELL; i++)
	{
		data[i][0] *= multiplier;
		data[i][1] *= multiplier;
		data[i][2] *= multiplier;
		data[i][XRES/CELL-2] *= multiplier;
		data[i][XRES/CELL-1] *= multiplier;
	}
	for (int i=0; i<XRES/CELL; i++)
	{
		data[0][i] *= multiplier;
		data[1][i] *= multiplier;
		data[2][i] *= multiplier;
		data[YRES/CELL-2][i] *= multiplier;
		data[YRES/CELL-1][i] *= multiplier;
	}
}

TPT_NOINLINE void AirSimulator_v1::reduceEdges_v(CellsFloatRP data, float multiplier)
{
	// at edges: reduce velocity, and propagate velocity from the middle towards edges
	for (int i=0; i<YRES/CELL; i++)
	{
		data[i][0] = data[i][1]*multiplier;
		data[i][1] = data[i][2]*multiplier;
		data[i][XRES/CELL-1] = data[i][XRES/CELL-2]*multiplier;
		data[i][XRES/CELL-2] = data[i][XRES/CELL-3]*multiplier;
	}
	for (int i=0; i<XRES/CELL; i++)
	{
		data[0][i] = data[1][i]*multiplier;
		data[1][i] = data[2][i]*multiplier;
		data[YRES/CELL-1][i] = data[YRES/CELL-2][i]*multiplier;
		data[YRES/CELL-2][i] = data[YRES/CELL-3][i]*multiplier;
	}
}

TPT_NOINLINE void AirSimulator_v1::pressureFromVelocity(
	const_CellsFloatRP src_pv,
	const_CellsFloatRP src_vx, const_CellsFloatRP src_vy,
	CellsFloatRP dest_pv
)
{
	for (int y=0; y<YRES/CELL; y++)
		dest_pv[y][0] = src_pv[y][0]*AIR_PLOSS;
	for (int x=0; x<XRES/CELL; x++)
		dest_pv[0][x] = src_pv[0][x]*AIR_PLOSS;

	// pressure adjustments from velocity
	// (in each cell, change in pressure = amount of air flowing in - amount of air flowing out = velocity in - velocity out
	for (int y=1; y<YRES/CELL; y++)
		for (int x=1; x<XRES/CELL; x++)
		{
			float pv = 0.0f;
			pv += src_vx[y][x-1] - src_vx[y][x];// (x velocity in) - (x velocity out)
			pv += src_vy[y-1][x] - src_vy[y][x];// (y velocity in) - (y velocity out)
			dest_pv[y][x] = src_pv[y][x]*AIR_PLOSS + pv*AIR_TSTEPP;
		}
}

TPT_NOINLINE void AirSimulator_v1::velocityFromPressure(
	const_CellsFloatRP src_pv,
	const_CellsFloatRP src_vx, const_CellsFloatRP src_vy,
	CellsFloatRP dest_vx, CellsFloatRP dest_vy
)
{
	for (int y=0; y<YRES/CELL; y++)
	{
		dest_vx[y][XRES/CELL-1] = src_vx[y][XRES/CELL-1]*AIR_VLOSS;
		dest_vy[y][XRES/CELL-1] = src_vy[y][XRES/CELL-1]*AIR_VLOSS;
	}
	for (int x=0; x<XRES/CELL; x++)
	{
		dest_vx[YRES/CELL-1][x] = src_vx[YRES/CELL-1][x]*AIR_VLOSS;
		dest_vy[YRES/CELL-1][x] = src_vy[YRES/CELL-1][x]*AIR_VLOSS;
	}

	// velocity adjustments from pressure
	// (in each cell, change in velocity = net force acting = pressure difference between adjacent cells)
	for (int y=0; y<YRES/CELL-1; y++)
		for (int x=0; x<XRES/CELL-1; x++)
		{
			float dx = src_pv[y][x] - src_pv[y][x+1];// dp/dx
			float dy = src_pv[y][x] - src_pv[y+1][x];// dp/dy
			dest_vx[y][x] = src_vx[y][x]*AIR_VLOSS + dx*AIR_TSTEPV;
			dest_vy[y][x] = src_vy[y][x]*AIR_VLOSS + dy*AIR_TSTEPV;
		}
}


TPT_NOINLINE void AirSimulator_v1::wallsBlockAir(CellsFloatRP vx, CellsFloatRP vy, const_CellsUCharRP blockair)
{
	//clear some velocities near walls
	// vx[y][x] is the velocity of air flow out of cell (x,y) into cell (x+1,y)

	FOR_2D_UINT8_NONZERO(blockair, x, 1, XRES/CELL, y, 1, YRES/CELL, {
		vx[y][x] = 0.0f;// no x flow out of wall
		vx[y][x-1] = 0.0f;// no x flow into wall
		vy[y][x] = 0.0f;
		vy[y-1][x] = 0.0f;
	})

	for (int y=0; y<YRES/CELL; y++)
	{
		if (blockair[y][0])
		{
			vx[y][0] = vy[y][0] = 0;
		}
	}
	for (int x=0; x<XRES/CELL; x++)
	{
		if (blockair[0][x])
		{
			vx[0][x] = vy[0][x] = 0;
		}
	}
}


// Apply a Gaussian blur to a single CellsArrayFloat
// Does not blur the edges or check for walls. These limitations, plus separating into two 1D convolutions and only doing one CellsArrayFloatP at a time, make the loops simple enough for g++ to be able to use some SIMD instructions
TPT_NOINLINE void AirSimulator_v1::blur_centreData(const_CellsFloatRP src, CellsFloatRP dest, CellsFloatRP tmp)
{
	// Normalised Gaussian kernel
	// Separable filter, so 1D x blur then 1D y blur is equivalent to a 2D blur
	float k0 = 1.0f;
	float k1 = expf(-2.0f);
	float ksum = k0 + 2*k1;
	k0 /= ksum;
	k1 /= ksum;

	// Blur in x direction
	for (int y=0; y<YRES/CELL; y++)
		for (int x=1; x<XRES/CELL-1; x++)
		{
			tmp[y][x] = src[y][x-1]*k1 + src[y][x]*k0 + src[y][x+1]*k1;
		}
	// Blur in y direction
	for (int y=1; y<YRES/CELL-1; y++)
		for (int x=0; x<XRES/CELL; x++)
		{
			dest[y][x] = tmp[y-1][x]*k1 + tmp[y][x]*k0 + tmp[y+1][x]*k1;
		}
}


// Apply a Gaussian blur to a single cell
// For neighbouring cells which don't exist or are walls, uses the values from the centre cell
void AirSimulator_v1::blur_cell_vp(
	int x, int y,
	const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_pv,
	const_CellsUCharRP blockair,
	CellsFloatRP dest_vx, CellsFloatRP dest_vy, CellsFloatRP dest_pv
)
{
	// velocity and pressure

	float *k = kernel;
	float fsum = 0.0f, dx = 0.0f, dy = 0.0f, dp = 0.0f;
	for (int j=-1; j<2; j++)
		for (int i=-1; i<2; i++)
		{
			if (y+j>=0 && y+j<YRES/CELL &&
					x+i>=0 && x+i<XRES/CELL &&
					!blockair[y+j][x+i])
			{
				float f = k[i+1+(j+1)*3];
				fsum += f;
				dx += src_vx[y+j][x+i]*f;
				dy += src_vy[y+j][x+i]*f;
				dp += src_pv[y+j][x+i]*f;
			}
		}
	if (fsum<0.99f)
	{
		dx += src_vx[y][x]*(1.0f-fsum);
		dy += src_vy[y][x]*(1.0f-fsum);
		dp += src_pv[y][x]*(1.0f-fsum);
	}
	dest_vx[y][x] = dx;
	dest_vy[y][x] = dy;
	dest_pv[y][x] = dp;
}
void AirSimulator_v1::blur_cell_h(int x, int y, const_CellsFloatRP src_hv, CellsFloatRP dest_hv, const_CellsUCharRP blockairh)
{
	// ambient heat

	float *k = kernel;
	float fsum = 0.0f, dh = 0.0f;
	for (int j=-1; j<2; j++)
		for (int i=-1; i<2; i++)
		{
			if (y+j>=0 && y+j<YRES/CELL &&
					x+i>=0 && x+i<XRES/CELL &&
					!(blockairh[y+j][x+i]&0x8))
			{
				float f = k[i+1+(j+1)*3];
				fsum += f;
				dh += src_hv[y+j][x+i]*f;
			}
		}
	if (fsum<0.99f)
	{
		dh += src_hv[y][x]*(1.0f-fsum);
	}
	dest_hv[y][x] = dh;
}

void AirSimulator_v1::blur_pressureAndVelocity(const_AirDataP src, AirDataP dest, const AirSimulator_params_base &params)
{
	blur_pressureAndVelocity(
		src.vx, src.vy, src.pv,
		params.blockair,
		dest.vx, dest.vy, dest.pv
	);
}

TPT_NOINLINE void AirSimulator_v1::blur_pressureAndVelocity(
	const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_pv,
	const_CellsUCharP blockair,
	CellsFloatRP dest_vx, CellsFloatRP dest_vy, CellsFloatRP dest_pv
)
{
	// Each element of blockair is 0 or 1, so add together to get number of blocked cells
	int blockCount = CellsData_count_1(blockair);

	if (blockCount > (XRES/CELL)*(YRES/CELL)/2) // TODO: determine the best threshold to use
	{
		// Single stage blur, if there are lots of walls which would cause recalculations in multi stage version

		for (int y=0; y<YRES/CELL; y++)
		{
			for (int x=0; x<XRES/CELL; x++)
			{
				blur_cell_vp(x, y, src_vx, src_vy, src_pv, blockair, dest_vx, dest_vy, dest_pv);
			}
		}
	}
	else
	{
		// Multi stage blur

		// Initial blur, which for speed does not check for edges or walls
		CellsFloat tmp;
		blur_centreData(src_vx, dest_vx, tmp);
		blur_centreData(src_vy, dest_vy, tmp);
		blur_centreData(src_pv, dest_pv, tmp);


		// Now fix places where initial blur gets it wrong:
		CellsUChar recalcLocations;
		CellsData_fill<unsigned char>(recalcLocations, 0);

		// around edges
		for (int x=0; x<XRES/CELL; x++)
		{
			recalcLocations[0][x] = 1;
			recalcLocations[YRES/CELL-1][x] = 1;
		}
		for (int y=1; y<YRES/CELL-1; y++)
		{
			recalcLocations[y][0] = 1;
			recalcLocations[y][XRES/CELL-1] = 1;
		}

		//  around air blocking walls
		FOR_CELLS_UINT8_NONZERO(blockair, x, y, {
			for (int j=-1; j<2; j++)
				for (int i=-1; i<2; i++)
				{
					if (y+j>=0 && y+j<YRES/CELL &&
					x+i>=0 && x+i<XRES/CELL)
					{
						recalcLocations[y+j][x+i] = 1;
					}
				}
		})

		// recalculate
		FOR_CELLS_UINT8_NONZERO(recalcLocations, x, y, {
			blur_cell_vp(x, y, src_vx, src_vy, src_pv, blockair, dest_vx, dest_vy, dest_pv);
		})
	}
}

void AirSimulator_v1::blur_heat(const_AirDataP src, AirDataP dest, const AirSimulator_params_base &params)
{
	blur_heat(src.hv, params.blockairh, dest.hv);
}

TPT_NOINLINE void AirSimulator_v1::blur_heat(const_CellsFloatRP src_hv, const_CellsUCharRP blockairh, CellsFloatRP dest_hv)
{
	// get number of blocked cells
	int blockCount = CellsData_count_and8(blockairh);

	if (blockCount > (XRES/CELL)*(YRES/CELL)/2) // TODO: determine the best threshold to use
	{
		// Single stage blur, if there are lots of walls which would cause recalculations in multi stage version

		for (int y=0; y<YRES/CELL; y++)
		{
			for (int x=0; x<XRES/CELL; x++)
			{
				blur_cell_h(x, y, src_hv, dest_hv, blockairh);
			}
		}
	}
	else
	{
		// Multi stage blur

		// Initial blur, which for speed does not check for edges or walls
		CellsFloat tmp;
		blur_centreData(src_hv, dest_hv, tmp);


		// Now fix places where initial blur gets it wrong:
		CellsUChar recalcLocations;
		CellsData_fill<unsigned char>(recalcLocations, 0);

		// around edges
		for (int x=0; x<XRES/CELL; x++)
		{
			recalcLocations[0][x] = 1;
			recalcLocations[YRES/CELL-1][x] = 1;
		}
		for (int y=1; y<YRES/CELL-1; y++)
		{
			recalcLocations[y][0] = 1;
			recalcLocations[y][XRES/CELL-1] = 1;
		}

		//  around air blocking walls
		FOR_CELLS_UINT8_MASKED(blockairh, 0x8, x, y, {
			for (int j=-1; j<2; j++)
				for (int i=-1; i<2; i++)
				{
					if (y+j>=0 && y+j<YRES/CELL &&
					x+i>=0 && x+i<XRES/CELL)
					{
						recalcLocations[y+j][x+i] = 1;
					}
				}
		})

		// recalculate
		FOR_CELLS_UINT8_NONZERO(recalcLocations, x, y, {
			blur_cell_h(x, y, src_hv, dest_hv, blockairh);
		})
	}
}

void AirSimulator_v1::velocityAdvection(const_AirDataP blurredSrc, const_AirDataP advSrc, AirDataP dest, const AirSimulator_params_base &params)
{
	velocityAdvection(
		blurredSrc.vx, blurredSrc.vy,
		advSrc.vx, advSrc.vy,
		dest.vx, dest.vy,
		params.blockair,
		params.wallsData.wallType, params.wallsData.fanVX, params.wallsData.fanVY
	);
}

TPT_NOINLINE void AirSimulator_v1::velocityAdvection(
	const_CellsFloatRP src_vx, const_CellsFloatRP src_vy,
	const_CellsFloatRP src_avx, const_CellsFloatRP src_avy,
	CellsFloatRP dest_vx, CellsFloatRP dest_vy,
	const_CellsUCharRP blockair,
	const_CellsUCharRP bmap, const_CellsFloatRP fvx, const_CellsFloatRP fvy
)
{
	int x, y, i, j;
	float dx, dy, tx, ty;
	float stepX, stepY;
	int stepLimit, step;
	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			dx = src_vx[y][x];
			dy = src_vy[y][x];
			tx = x - dx*advDistanceMult;
			ty = y - dy*advDistanceMult;
			// TODO: maybe add dx!=0 && dy!=0 &&
			if (tx>=2 && tx<XRES/CELL-2 && ty>=2 && ty<YRES/CELL-2 && !blockair[y][x])
			{
				if (dx*advDistanceMult>1.0f || dy*advDistanceMult>1.0f)
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
						if (blockair[(int)(ty+0.5f)][(int)(tx+0.5f)])
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


				dx *= 1.0f - AIR_VADV;
				dy *= 1.0f - AIR_VADV;

				// Bilinear interpolation
				dx += AIR_VADV*(1.0f-tx)*(1.0f-ty)*src_avx[j][i];
				dy += AIR_VADV*(1.0f-tx)*(1.0f-ty)*src_avy[j][i];

				dx += AIR_VADV*tx*(1.0f-ty)*src_avx[j][i+1];
				dy += AIR_VADV*tx*(1.0f-ty)*src_avy[j][i+1];

				dx += AIR_VADV*(1.0f-tx)*ty*src_avx[j+1][i];
				dy += AIR_VADV*(1.0f-tx)*ty*src_avy[j+1][i];

				dx += AIR_VADV*tx*ty*src_avx[j+1][i+1];
				dy += AIR_VADV*tx*ty*src_avy[j+1][i+1];
			}
			// Get velocity from fan wall
			if (bmap[y][x] == WL_FAN)
			{
				dx += fvx[y][x];
				dy += fvy[y][x];
			}
			dest_vx[y][x] = dx;
			dest_vy[y][x] = dy;
		}
	}
}

TPT_NOINLINE void AirSimulator_v1::heatAdvection(const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_hv, const_CellsFloatRP src_ahv, const_CellsUCharRP blockairh, CellsFloatRP dest_hv)
{
	float odh, dh, dx, dy, tx, ty;
	int x, y, i, j;
	float stepX, stepY;
	int stepLimit, step;

	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			odh = dh = src_hv[y][x];
			dx = src_vx[y][x];
			dy = src_vy[y][x];
			tx = x - dx*advDistanceMult;
			ty = y - dy*advDistanceMult;
			// TODO: maybe add dx!=0 && dy!=0 &&
			if (tx>=2 && tx<XRES/CELL-2 && ty>=2 && ty<YRES/CELL-2 && !(blockairh[y][x]&0x8))
			{
				if (dx*advDistanceMult>1.0f || dy*advDistanceMult>1.0f)
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
						if (blockairh[(int)(ty+0.5f)][(int)(tx+0.5f)]&0x8)
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

				dh *= 1.0f - AIR_VADV;
				dh += AIR_VADV*(1.0f-tx)*(1.0f-ty)*((blockairh[j][i]&0x8) ? odh : src_ahv[j][i]);
				dh += AIR_VADV*tx*(1.0f-ty)*((blockairh[j][i+1]&0x8) ? odh : src_ahv[j][i+1]);
				dh += AIR_VADV*(1.0f-tx)*ty*((blockairh[j+1][i]&0x8) ? odh : src_ahv[j+1][i]);
				dh += AIR_VADV*tx*ty*((blockairh[j+1][i+1]&0x8) ? odh : src_ahv[j+1][i+1]);
			}

			dest_hv[y][x] = dh;
		}
	}
}

TPT_NOINLINE void AirSimulator_v1::heatPressure(const_CellsFloatRP old_hv, CellsFloatRP new_hv, CellsFloatRP new_pv)
{
	int x, y;
	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			// increased temperature -> increased pressure
			new_pv[y][x] += (new_hv[y][x]-old_hv[y][x])/5000.0f;
		}
	}
}

TPT_NOINLINE void AirSimulator_v1::heatRise(const_CellsFloatRP old_hv, CellsFloatRP new_vx, CellsFloatRP new_vy, const AirSimulator_params_base &params)
{
	const_CellsUCharRP blockairh = params.blockairh;
	int gravityMode = params.gravityMode;

	if(!gravityMode)
	{
		//hot air rises, vertical gravity only for the time being
		for (int y=0; y<YRES/CELL; y++)
		{
			for (int x=0; x<XRES/CELL; x++)
			{
				float airdiff = old_hv[y-1][x]-old_hv[y][x];
				if(airdiff>0 && !(blockairh[y-1][x]&0x8))
					new_vy[y][x] -= airdiff/5000.0f;
			}
		}
	}
}

void AirSimulator_v1::sim_impl_rwInput(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params)
{
	if (params.airMode==4) // no air/pressure update mode
	{
		AirData_copy(inputData, outputData);
	}
	else if (params.airMode==3) // zero pressure+velocity mode
	{
		CellsData_fill(outputData.vx, 0.0f);
		CellsData_fill(outputData.vy, 0.0f);
		CellsData_fill(outputData.pv, 0.0f);
	}
	else
	{
		// Reduce pressure and velocity near edges
		reduceEdges_p(inputData.pv, 0.8f);
		reduceEdges_v(inputData.vx, 0.9f);
		reduceEdges_v(inputData.vy, 0.9f);

		// Zero velocity into and out of walls
		wallsBlockAir(inputData.vx, inputData.vy, params.blockair);

		// Update pressure and velocity
		pressureFromVelocity(inputData.pv, inputData.vx, inputData.vy, tmpData.pv);
		velocityFromPressure(tmpData.pv, inputData.vx, inputData.vy, tmpData.vx, tmpData.vy);

		// Zero velocity into and out of walls again (since it may have been changed by velocityFromPressure)
		wallsBlockAir(tmpData.vx, tmpData.vy, params.blockair);

		// Apply a Gaussian blur to updated pressure and velocity
		blur_pressureAndVelocity(tmpData, blurredData, params);

		if (params.airMode==1)
		{
			// zero pressure mode
			CellsData_fill(outputData.pv, 0.0f);
		}
		else
		{
			// cap final pressure
			CellsData_limit(blurredData.pv, outputData.pv, -256.0f, 256.0f);
		}

		if (params.airMode==2)
		{
			// zero velocity mode
			CellsData_fill(outputData.vx, 0.0f);
			CellsData_fill(outputData.vy, 0.0f);
		}
		else
		{
			// advection
			velocityAdvection(blurredData, tmpData, outputData, params);
			// cap final velocities
			CellsData_limit(outputData.vx, -256.0f, 256.0f);
			CellsData_limit(outputData.vy, -256.0f, 256.0f);
		}
	}

	if (params.ambientHeatEnabled)
	{
		setEdges_h(inputData.hv, params.ambientTemp);
		blur_heat(inputData, blurredData, params);
		heatAdvection(blurredData.vx, blurredData.vy, blurredData.hv, inputData.hv, params.blockairh, outputData.hv);
		heatPressure(inputData.hv, outputData.hv, outputData.pv);
		heatRise(inputData.hv, outputData.vx, outputData.vy, params);
	}
	else
	{
		CellsData_copy<float>(inputData.hv, outputData.hv);
	}
}





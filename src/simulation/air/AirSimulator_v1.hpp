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

#ifndef Simulation_Air_AirSimulatorV1_h
#define Simulation_Air_AirSimulatorV1_h

#include "simulation/air/AirSimulator.hpp"

class AirSimulator_v1 : public AirSimulator_sync
{
protected:
	static constexpr float advDistanceMult = 0.7f;

	AirData tmpData;
	AirData tmpData2;
	AirData blurredData;

	void setEdges_h(CellsFloatRP data, float value);
	void reduceEdges_p(CellsFloatRP data, float multiplier);
	void reduceEdges_v(CellsFloatRP data, float multiplier);
	void pressureFromVelocity(const_CellsFloatRP src_pv, const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, CellsFloatRP dest_pv);
	void velocityFromPressure(const_CellsFloatRP src_pv, const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, CellsFloatRP dest_vx, CellsFloatRP dest_vy);

	void wallsBlockAir(CellsFloatRP vx, CellsFloatRP vy, const_CellsUCharRP blockair);
	void blur_centreData(const_CellsFloatRP src, CellsFloatRP dest, CellsFloatRP tmp);

	void blur_pressureAndVelocity(const_AirDataP src, AirDataP dest, const AirSimulator_params_base &params);
	void blur_pressureAndVelocity(const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_pv, const_CellsUCharP blockair, CellsFloatRP dest_vx, CellsFloatRP dest_vy, CellsFloatRP dest_pv);
	void blur_cell_vp(int x, int y, const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_pv, const_CellsUCharRP blockair, CellsFloatRP dest_vx, CellsFloatRP dest_vy, CellsFloatRP dest_pv);
	void blur_heat(const_AirDataP src, AirDataP dest, const AirSimulator_params_base &params);
	void blur_heat(const_CellsFloatRP src_hv, const_CellsUCharRP blockairh, CellsFloatRP dest_hv);
	void blur_cell_h(int x, int y, const_CellsFloatRP src_hv, CellsFloatRP dest_hv, const_CellsUCharRP blockairh);

	void velocityAdvection(const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_avx, const_CellsFloatRP src_avy, CellsFloatRP dest_vx, CellsFloatRP dest_vy, const_CellsUCharRP blockair, const_CellsUCharRP bmap, const_CellsFloatRP fvx, const_CellsFloatRP fvy);
	void velocityAdvection(const_AirDataP blurredSrc, const_AirDataP advSrc, AirDataP dest, const AirSimulator_params_base &params);
	void heatAdvection(const_CellsFloatRP src_vx, const_CellsFloatRP src_vy, const_CellsFloatRP src_hv, const_CellsFloatRP src_ahv, const_CellsUCharRP blockairh, CellsFloatRP dest_hv);

	void heatPressure(const_CellsFloatRP old_hv, CellsFloatRP new_hv, CellsFloatRP new_pv);
	void heatRise(const_CellsFloatRP old_hv, CellsFloatRP new_vx, CellsFloatRP new_vy, const AirSimulator_params_base &params);

public:
	AirSimulator_v1() : AirSimulator_sync() {}
	virtual ~AirSimulator_v1() {}
	virtual void sim_impl_rwInput(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);
};

#endif

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

#ifndef Simulation_Air_AirSimulatorV0_h
#define Simulation_Air_AirSimulatorV0_h

#include "simulation/air/AirSimulator.hpp"

class AirSimulator_v0 : public AirSimulator_sync
{
protected:
	void update_air(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);
	void update_airh(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);
public:
	AirSimulator_v0() : AirSimulator_sync() {}
	virtual ~AirSimulator_v0() {}
	virtual void sim_impl_rwInput(AirDataP inputData, AirDataP outputData, AirSimulator_params_base &params);
};

#endif

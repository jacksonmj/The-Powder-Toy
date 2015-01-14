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

#ifndef Simulation_Elements_STKM_H
#define Simulation_Elements_STKM_H 

#include "simulation/ElemDataSim.h"
#include "simulation/Simulation.h"

class particle;

namespace STKM_commands
{
	enum command { Left=1, Right=2, Jump=4, Action=8 };
}
typedef STKM_commands::command STKM_command_t;

class Stickman_data
{
public:
	bool isStored;		// is this stickman currently stored in a portal (doesn't exist as a particle on screen, but should still stop another stickman being created)
	particle * part;	// pointer to current particle for the stickman (could be in parts array, or in portal data), or NULL if it doesn't exist
	char comm;           //command cell
	char pcomm;          //previous command
	int elem;            //element power
	float legs[16];      //legs' positions
	float accs[8];       //accelerations
	unsigned int elemDelayFrames; //frames to delay before particle spawn is allowed again - used when spawning LIGH
	bool rocketBoots;


	bool exists() { return part; }
	bool isOnscreen() { return part && !isStored; }
	void reset();
	void set_legs_pos(int x, int y);
	void set_legs_pos(float x, float y) { set_legs_pos((int)(x+0.5f), (int)(y+0.5f)); }
	void set_particle(particle * p);
	void commandOn(STKM_commands::command commOn) { comm |= commOn; }
	void commandOff(STKM_commands::command commOff)
	{
		if (commOff==STKM_commands::Left || commOff==STKM_commands::Right)
		{
			pcomm = comm;  //Save last movement
		}
		comm &= ~commOff;
	}
	Stickman_data();
	virtual ~Stickman_data() {}

	// A short way to get the Stickman_data* for a particle:
	static Stickman_data * get(Simulation * sim, const particle &p);
	static Stickman_data * get(Simulation * sim, int elementId, int whichFighter=-1);
};

class STK_common_ElemDataSim : public ElemDataSim
{
public:
	bool storageActionPending;// is this stickman about to be stored/retrieved from a portal - overrides existence check in STKM_create_allowed function
	STK_common_ElemDataSim(Simulation *s, int t) : ElemDataSim(s, t), storageActionPending(false) {}
	virtual void on_part_create(particle & p) = 0;
	virtual void on_part_kill(particle & p) = 0;
};

class STKM_ElemDataSim : public STK_common_ElemDataSim
{
private:
	Observer_ClassMember<Stickman_data> obs_simCleared;
public:
	Stickman_data player;
	STKM_ElemDataSim(Simulation *s, int t);
	bool create_allowed()
	{
		return storageActionPending || !player.exists();
	}
	void on_part_create(particle & p);
	void on_part_kill(particle & p);
};

void STKM_interact(Simulation *sim, Stickman_data* playerp, int i, int x, int y);
int STKM_graphics(GRAPHICS_FUNC_ARGS);
int run_stickman(Stickman_data* playerp, UPDATE_FUNC_ARGS);

#endif

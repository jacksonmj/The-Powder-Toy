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

#ifndef Simulation_h
#define Simulation_h

#include "Config.h"

#include "simulation/BasicData.hpp"
#include "simulation/air/SimAir.hpp"
#include "simulation/walls/SimWalls.hpp"
#include "simulation/Sign.hpp"
#include <cmath>
#include "powder.h"
#include "gravity.h"
#include <vector>
#include <array>
#include "common/Observer.h"
#include "common/tptmath.h"
#include "common/tpt-rng.h"

#include "simulation/ElementNumbers.h"

#define R_TEMP 22
#define MAX_TEMP 9999
#define MIN_TEMP 0
constexpr float TEMP_RANGE = MAX_TEMP-MIN_TEMP;
#define O_MAX_TEMP 3500
#define O_MIN_TEMP -273
constexpr float O_TEMP_RANGE = O_MAX_TEMP-O_MIN_TEMP;

#define FLAG_STAGNANT	0x1
#define FLAG_SKIPMOVE	0x2 // skip movement for one frame, only implemented for PHOT
#define FLAG_WATEREQUAL	0x4 // if a liquid was already checked during equalization
#define FLAG_MOVABLE	0x8 // compatibility with old saves (moving SPNG), only applies to SPNG
#define FLAG_PHOTDECO  0x8 // compatibility with old saves (decorated photons), only applies to PHOT. Having the same value as FLAG_MOVABLE is fine because they apply to different elements, and this saves space for future flags.


// Defines for element transitions
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM


// Macros to loop through all the particles in a certain position
// sim is the Simulation object ("FOR_SIM_PMAP_POS(this, ...)" in Simulation methods).
// category is a PMapCategory value (PMapCategory::All/Energy/Plain) indicating which subset of particles to loop through.
// pos is the position to loop through.
// r_i is the name to use for the particle ID variable (variable will be declared by the macro).
// If any particle in the current position is killed or moved except particle r_i and particles that have already been iterated over, then you must break out of the pmap position loop. 
#define FOR_SIM_PMAP_POS(sim, category, pos, r_i) FOR_PMAP_POS((sim)->pmap, (sim)->parts, category, pos, r_i)

// TODO: remove
#define FOR_PMAP_POSITION(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap(SimPosI((x),(y))).count(PMapCategory::All), r_next=(sim)->pmap(SimPosI((x),(y))).first(PMapCategory::All); r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_NOENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap(SimPosI((x),(y))).count(PMapCategory::Plain), r_next=(sim)->pmap(SimPosI((x),(y))).first(PMapCategory::Plain); r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_ONLYENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap(SimPosI((x),(y))).count(PMapCategory::Energy), r_next=(sim)->pmap(SimPosI((x),(y))).first(PMapCategory::Energy); r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)


class MoveResult
{
public:
	typedef enum {
		// Movement result codes, to indicate whether a particle can move and what happened to it when it did move

		// In general, lower values mean less movement is possible.
		// There are some values with similar meanings (e.g. DESTROY and DESTROYED) - these are for before and after movement, to keep some of the checks here and in Simulation::part_canMove simpler.
		// Note that the Simulation::part_canMove return value does not indicate whether a particle will change type. (Since you might want to swap and change type, or occupy same space and change type (with or without causing interpolation to be blocked) - so in this case part_canMove is used to indicate movement behaviour only).

		DESTROYED = -3,
		STORED = -2,
		CHANGED_TYPE = -1,

		// movement is blocked
		BLOCK = 0,
		// movement will cause the destruction of the moving particle
		DESTROY = 1,
		// the particle at the destination will be displaced
		DISPLACE = 2,
		// movement will cause the moving particle to be stored (distinct from DESTROY to allow placement with brush into PIPE/PRTI, and if it doesn't get stored movement is blocked by try_move instead of destroying the particle)
		STORE = 3,
		// movement is allowed, but interpolation is not, to stop the particle skipping through this location. Used when there is a reaction that occurs when a particle is moving through something, and for photons inside other substances
		ALLOW_BUT_SLOW = 4,
		// movement is allowed
		ALLOW = 5,
		// movement varies, run further checks in part_canMove_dynamic
		DYNAMIC = 6
	} Code;

	//*** Functions to use on part_move return values, but not with part_canMove return values ***
	static bool Succeeded_MaybeKilled(Code c) {
		// Moved somewhere, but particle may have been destroyed
		// (do not try to move again)
		return (c!=BLOCK);
	}
	static bool Succeeded(Code c) {
		// Moved somewhere (DESTROY and STORE are never returned by try_move, only by part_canMove)
		// (can try to move again)
		return (c>0);
	}
	static bool WasBlocked(Code c) {
		// Didn't move, but didn't get destroyed either
		// (can try to move again)
		return (c==BLOCK);
	}
	static bool WasKilled(Code c) {
		// destroyed
		// (do not try to move again)
		return (c<0);
	}

	//*** Functions to use on part_canMove return values, but not with part_move return values ***
	static bool AllowInterpolation(Code c) {
		// TODO: when breaking compatibility, make displacement slow particles down?
		if (c==MoveResult::DISPLACE || c==MoveResult::ALLOW)
			return true;
		return false;
	}
	static bool WillSucceed_MaybeDie(Code c) {
		// part_canMove predicts that Succeeded_MaybeKilled(try_move())==true
		// (will move somewhere, but particle may get destroyed)
		return (c!=BLOCK);
	}
	static bool WillSucceed(Code c) {
		// part_canMove predicts that Succeeded(try_move())==true
		// (will move somewhere, and will not get destroyed)
		return (c==DISPLACE || c==ALLOW_BUT_SLOW || c==ALLOW);
	}
	static bool WillDie(Code c) {
		// part_canMove predicts that WasKilled(try_move())==true
		// (will get destroyed)
		return (c==DESTROY || c==STORE);
	}
	static bool WillBlock(Code c) {
		// part_canMove predicts that WasBlocked(try_move())==true
		// (will not move)
		return (c==BLOCK);
	}
};

class InterpolateMoveResult
{
public:
	SimPosI dest, clear;
	SimPosF destf, clearf;
	MoveResult::Code destCode;
	bool limitApplied;
	InterpolateMoveResult() : limitApplied(false) {}
};

enum class HeatMode : short
{
	Legacy=0,
	Normal=1
};

class SimulationSharedData;

class Simulation;

class Simulation :
	public Sim_BasicData
{
public:
	//simulation random number generator, should only be used for simulation, not graphics or scripts
	tpt_rng rngBase;
	RandomStore_auto rng;
public:
	SimAir air;
	SimWalls walls;

	MoveResult::Code can_move[PT_NUM][PT_NUM];
	static constexpr float maxVelocity = 1e4f;

protected:
	HeatMode heatMode;
	short edgeMode;// TODO: make into enum
	bool stackingCheckQueued;
public:
	short airMode;
	int ambientHeatEnabled;// TODO: should really be bool, but quickmenu can only handle ints
	float ambientTemp;

	ObserverSubject hook_cleared;
	ObserverSubject hook_beforeUpdate;
	ObserverSubject hook_afterUpdate;

	Signs signs;

	short option_edgeMode() { return edgeMode; }
	void option_edgeMode(short newMode);
	HeatMode option_heatMode() { return heatMode; }
	void option_heatMode(HeatMode newMode);

	template<class ElemDataClass_T, typename... Args>
	void elemData_create(int elementId, Args&&... args)
	{
		delete elemDataSim_[elementId];
		elemDataSim_[elementId] = new ElemDataClass_T(this, elementId, args...);
	}

	Simulation(std::shared_ptr<SimulationSharedData> sd);
	~Simulation();
	void clear();
	bool Check();
	void StackingCheck();
	void queueStackingCheck();
	void UpdateParticles();

	int part_create(int p, SimPosF newPosF, int t);
	bool part_change_type(int i, SimPosI pos, int t);
	void part_kill(int i, SimPosI pos);
	int part_killAll(SimPosI pos, int only_type=0, int except_id=-1);
	int part_killAll(SimAreaI area, int only_type=0, int except_id=-1);
	int part_killAll(SimAreaF area, int only_type=0, int except_id=-1);

	bool part_change_type(int i, int t)
	{ return part_change_type(i, SimPosF(parts[i].x, parts[i].y), t); }
	void part_kill(int i)
	{ part_kill(i, SimPosF(parts[i].x, parts[i].y)); }


	// Functions for changing particle temperature, respecting temperature caps.
	// TODO: The _noLatent functions also set the stored transition energy of the particle to make it appear as though latent heat does not apply - the particles will change into the type that they should be at that temperature, instead of the temperature increase just contributing towards the stored transition energy
	// set_temp sets the temperature to a specific value, add_temp changes the temperature by the given amount (can be positive or negative)
	void part_set_temp(particle & p, float newTemp) {
		p.temp = tptmath::clamp_flt(newTemp, MIN_TEMP, MAX_TEMP);
	}
	void part_set_temp_noLatent(particle & p, float newTemp) {
		part_set_temp(p, newTemp);
	}
	void part_add_temp(particle & p, float change) {
		p.temp = tptmath::clamp_flt(p.temp+change, MIN_TEMP, MAX_TEMP);
	}
	void part_add_temp_noLatent(particle & p, float change) {
		part_add_temp(p, change);
	}
	void part_add_energy(particle & p, float amount);// TODO

	void spark_particle_nocheck(int i, SimPosI pos);
	void spark_particle_nocheck_forceSPRK(int i, SimPosI pos);
	bool spark_particle(int i, SimPosI pos);
	bool spark_particle_conductiveOnly(int i, SimPosI pos);
	int spark_position(SimPosI pos);
	int spark_position_conductiveOnly(SimPosI pos);

	bool isWallBlocking(SimPosCell c, int type);
	bool isWallDeadly(SimPosCell c, int type);
	MoveResult::Code part_move(int i, SimPosI srcPos, SimPosF destPosF);
	MoveResult::Code part_canMove(int pt, SimPosI newPos, bool coordHandleEdgesDone=false);
	MoveResult::Code part_canMove_dynamic(int pt, SimPosI newPos, int ri, MoveResult::Code result);
	void interpolateMove(InterpolateMoveResult& result, bool interact, int t, SimPosF start, SimPosDF delta, bool limitDelta=true, float stepSize=ISTP);
	void interpolateMove(InterpolateMoveResult& result, bool interact, const particle& p)
	{
		interpolateMove(result, interact, p.type, SimPosF(p.x,p.y), SimPosDF(p.vx,p.vy));
	}

	void InitCanMove();
	MoveResult::Code part_move(int i, SimPosF newPosF)
	{ return part_move(i, SimPosF(parts[i].x, parts[i].y), newPosF); }

	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling


	bool part_set_pos(int i, SimPosI currPos, SimPosF newPosF)
	{
		newPosF = pos_handleEdges(SimPosF(SimPosFT(newPosF)));
		if (!Sim_BasicData::part_set_pos(i, currPos, newPosF))
		{
			part_kill(i, currPos);
			return false;
		}
		return true;
	}
	bool part_set_pos(int i, SimPosF newPosF)
	{
		return part_set_pos(i, SimPosF(parts[i].x, parts[i].y), newPosF);
	}

	// Adjust coords to take account of edgeMode setting
	// Sets changed to true if coords were changed
	SimPosI pos_handleEdges(SimPosI p, bool *changed=nullptr) const
	{
		if (edgeMode == 2)
		{
			if (pos_inMainArea(p))
			{
				if (changed)
					*changed = false;
				return p;
			}
			// Need to wrap
			if (changed)
				*changed = true;
			const int mainWidth = XRES-CELL*2,  mainHeight = YRES-CELL*2;
			if (p.x >= CELL-mainWidth && p.x < XRES-CELL+mainWidth && p.y >= CELL-mainHeight && p.y < YRES-CELL+mainHeight)
			{
				// Not far out of bounds, so wrapping can be done with single add/subtracts
				if (p.x < CELL)
					p.x += mainWidth;
				else if (p.x >= XRES-CELL)
					p.x -= mainWidth;
				if (p.y < CELL)
					p.y += mainHeight;
				else if (p.y >= YRES-CELL)
					p.y -= mainHeight;
				return p;
			}
			else
			{
				// Large distance out of bounds, so need to use slower method
				return pos_wrapMainArea(p);
			}
		}
		if (changed)
			*changed = false;
		return p;
	}
	SimPosF pos_handleEdges(SimPosF pf, bool *changed=nullptr) const
	{
		if (edgeMode == 2)
		{
			SimPosI p(pf);
			if (pos_inMainArea(p))
			{
				if (changed)
					*changed = false;
				return pf;
			}
			// Need to wrap
			if (changed)
				*changed = true;
			const int mainWidth = XRES-CELL*2,  mainHeight = YRES-CELL*2;
			if (p.x >= CELL-mainWidth && p.x < XRES-CELL+mainWidth && p.y >= CELL-mainHeight && p.y < YRES-CELL+mainHeight)
			{
				// Not far out of bounds, so wrapping can be done with single add/subtracts
				if (p.x < CELL)
					pf.x += mainWidth;
				else if (p.x >= XRES-CELL)
					pf.x -= mainWidth;
				if (p.y < CELL)
					pf.y += mainHeight;
				else if (p.y >= YRES-CELL)
					pf.y -= mainHeight;
				return SimPosFT(pf);
			}
			else
			{
				// Large distance out of bounds, so need to use slower method
				return pos_wrapMainArea(pf);
			}
		}
		if (changed)
			*changed = false;
		return pf;
	}


	int pmap_find_one_conductive(SimPosI pos, int t) const
	{
		const PMapCategory c = PMapCategory::Plain;
		int count = pmap(pos).count(c);
		int i = pmap(pos).first(c);
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (part_cmp_conductive(parts[i], t))
				return i;
		}
		return -1;
	}

	// Is this particle an element of type t, ignoring the current SPRKed status of this particle?
	bool part_cmp_conductive(const particle& p, int t) const
	{
		return (p.type==t || (p.type==PT_SPRK && p.ctype==t));
	}
	// Is this particle a sparkable element (though not necessarily able to be sparked immediately)?
	bool part_is_sparkable(const particle& p) const
	{
		if (p.type==PT_WIRE || p.type==PT_INST || (elements[p.type].Properties&PROP_CONDUCTS))
			return true;
		if (p.type==PT_SWCH && p.life >= 10)
			return true;
		if (p.type==PT_SPRK && p.ctype >= 0 && p.ctype < PT_NUM && ((elements[p.ctype].Properties&PROP_CONDUCTS) || p.ctype==PT_INST || p.ctype==PT_SWCH))
			return true;
		return false;
	}
	// Returns true if spark trying to travel between i1 and i2 is blocked by INSL (or other future spark blocking elements)
	// Plus a version for if integer coordinates are already known
	bool is_spark_blocked(int i1, int i2) const
	{
		SimPosI pos = SimPos::midpoint_tl(SimPosF(parts[i1].x, parts[i1].y), SimPosF(parts[i1].x, parts[i1].y));
		if (pmap_find_one(pos, PT_INSL)>=0)
			return true;
		return false;
	}
	bool is_spark_blocked(SimPosI p1, SimPosI p2) const
	{
		SimPosI pos = SimPos::midpoint_tl(p1, p2);
		if (pmap_find_one(pos, PT_INSL)>=0)
			return true;
		return false;
	}

	void GetGravityAccel(int x, int y, float particleGrav, float newtonGrav, float & pGravX, float & pGravY) const
	{
		pGravX = newtonGrav*gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
		pGravY = newtonGrav*gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
		switch (gravityMode)
		{
			default:
			case 0: //normal, vertical gravity
				pGravY += particleGrav;
				break;
			case 1: //no gravity
				break;
			case 2: //radial gravity
				if (x-XCNTR != 0 || y-YCNTR != 0)
				{
					float pGravMult = particleGrav/sqrt((x-XCNTR)*(x-XCNTR) + (y-YCNTR)*(y-YCNTR));
					pGravX -= pGravMult * (float)(x - XCNTR);
					pGravY -= pGravMult * (float)(y - YCNTR);
				}
		}
	}

	static std::array<SimPosDI,9> const posdata_D1;
	static std::array<SimPosDI,8> const posdata_D1_noCentre;
	static std::array<SimPosDI,4> const posdata_D1_noDiag_noCentre;
	static std::array<SimPosDI,25> const posdata_D2;
	static std::array<SimPosDI,24> const posdata_D2_noCentre;
	// Generate a random relative position
	template <class A>
	SimPosDI pos_random(A &data)
	{
		return data[rng.randInt<0,std::tuple_size<A>::value>()];
	}
	SimPosDI pos_randomD1()//radius 1
	{
		return pos_random(posdata_D1);
	}
	SimPosDI pos_randomD1_noCentre()//radius 1, excluding (0,0)
	{
		return pos_random(posdata_D1_noCentre);
	}
	SimPosDI pos_randomD1_noDiag_noCentre()//radius 1, excluding (0,0) and diagonals
	{
		return pos_random(posdata_D1_noDiag_noCentre);
	}
	SimPosDI pos_randomD2()//radius 2
	{
		return pos_random(posdata_D2);
	}
	SimPosDI pos_randomD2_noCentre()//radius 2, excluding (0,0)
	{
		return pos_random(posdata_D2_noCentre);
	}

	// Convert x,y args to SimPos args. TODO: remove these
	bool InBounds(int x, int y) const
	{
		return pos_isValid(SimPosI(x,y));
	}
	bool IsValidElement(int t) const
	{
		return element_isValid(t);
	}
	void spark_particle_nocheck(int i, int x, int y)
	{
		spark_particle_nocheck(i, SimPosI(x,y));
	}
	void spark_particle_nocheck_forceSPRK(int i, int x, int y)
	{
		spark_particle_nocheck_forceSPRK(i, SimPosI(x,y));
	}
	bool spark_particle(int i, int x, int y)
	{
		return spark_particle(i, SimPosI(x,y));
	}
	bool spark_particle_conductiveOnly(int i, int x, int y)
	{
		return spark_particle_conductiveOnly(i, SimPosI(x,y));
	}
	int spark_position(int x, int y)
	{
		return spark_position(SimPosI(x,y));
	}
	int spark_position_conductiveOnly(int x, int y)
	{
		return spark_position_conductiveOnly(SimPosI(x,y));
	}

	MoveResult::Code part_move(int i, int x,int y, float nxf,float nyf)
	{
		return part_move(i, SimPosI(x,y), SimPosF(nxf,nyf));
	}
	MoveResult::Code part_canMove(int pt, int nx,int ny, bool coordCheckDone=false)
	{
		return part_canMove(pt, SimPosI(nx,ny), coordCheckDone);
	}
	MoveResult::Code part_canMove_dynamic(int pt, int nx,int ny, int ri, MoveResult::Code result)
	{
		return part_canMove_dynamic(pt, SimPosI(nx,ny), ri, result);
	}
	MoveResult::Code part_move(int i, float nxf,float nyf)
	{
		return part_move(i, SimPosF(nxf,nyf));
	}
	void part_set_pos(int i, int x, int y, float nxf, float nyf)
	{
		part_set_pos(i, SimPosI(x,y), SimPosF(nxf, nyf));
	}
	void part_set_pos(int i, float nxf, float nyf)
	{
		part_set_pos(i, SimPosF(nxf, nyf));
	}
	int part_create(int i, int x, int y, int t)
	{
		return part_create(i, SimPosI(x,y), t);
	}
	bool part_change_type(int i, int x, int y, int t)
	{
		return part_change_type(i, SimPosI(x,y), t);
	}
	void part_kill(int i, int x, int y)
	{
		part_kill(i, SimPosI(x,y));
	}
	using Sim_BasicData::pmap_find_one;
	int pmap_find_one(int x, int y, int t) const
	{
		return pmap_find_one(SimPosI(x,y), t);
	}
	int pmap_find_one_conductive(int x, int y, int t) const
	{
		return pmap_find_one_conductive(SimPosI(x,y), t);
	}
	void randomRelPos_1(int *rx, int *ry)
	{
		SimPosDI p = pos_randomD1();
		*rx = p.x, *ry = p.y;
	}
	void randomRelPos_1_noCentre(int *rx, int *ry)
	{
		SimPosDI p = pos_randomD1_noCentre();
		*rx = p.x, *ry = p.y;
	}
	void randomRelPos_1_noDiag_noCentre(int *rx, int *ry)
	{
		SimPosDI p = pos_randomD1_noDiag_noCentre();
		*rx = p.x, *ry = p.y;
	}
	void randomRelPos_2(int *rx, int *ry)
	{
		SimPosDI p = pos_randomD2();
		*rx = p.x, *ry = p.y;
	}
	void randomRelPos_2_noCentre(int *rx, int *ry)
	{
		SimPosDI p = pos_randomD2_noCentre();
		*rx = p.x, *ry = p.y;
	}
};

void Simulation_Compat_CopyData(Simulation *sim);

// TODO: remove these
extern Simulation *globalSim;
extern AirData undo_airData;
extern WallsData undo_wallsData;

#endif

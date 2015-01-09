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
#include "simulation/Element.h"
#include "simulation/ElemDataSim.h"
#include <cmath>
#include "powder.h"
#include "gravity.h"
#include <vector>
#include <memory>
#include "common/Observer.h"

#include "simulation/ElementNumbers.h"

#define R_TEMP 22
#define MAX_TEMP 9999
#define MIN_TEMP 0
#define O_MAX_TEMP 3500
#define O_MIN_TEMP -273

#define ST_NONE 0
#define ST_SOLID 1
#define ST_LIQUID 2
#define ST_GAS 3
/*
   TODO: We should start to implement these.
*/
#define TYPE_PART			0x00001 //1 Powders
#define TYPE_LIQUID			0x00002 //2 Liquids
#define TYPE_SOLID			0x00004 //4 Solids
#define TYPE_GAS			0x00008 //8 Gases (Includes plasma)
#define TYPE_ENERGY			0x00010 //16 Energy (Thunder, Light, Neutrons etc.)
#define PROP_CONDUCTS		0x00020 //32 Conducts electricity
#define PROP_BLACK			0x00040 //64 Absorbs Photons (not currently implemented or used, a photwl attribute might be better)
#define PROP_NEUTPENETRATE	0x00080 //128 Penetrated by neutrons
#define PROP_NEUTABSORB		0x00100 //256 Absorbs neutrons, reflect is default
#define PROP_NEUTPASS		0x00200 //512 Neutrons pass through, such as with glass
#define PROP_DEADLY			0x00400 //1024 Is deadly for stickman
#define PROP_HOT_GLOW		0x00800 //2048 Hot Metal Glow
#define PROP_LIFE			0x01000 //4096 Is a GoL type
#define PROP_RADIOACTIVE	0x02000 //8192 Radioactive
#define PROP_LIFE_DEC		0x04000 //2^14 Life decreases by one every frame if > zero
#define PROP_LIFE_KILL		0x08000 //2^15 Kill when life value is <= zero
#define PROP_LIFE_KILL_DEC	0x10000 //2^16 Kill when life value is decremented to<= zero
#define PROP_SPARKSETTLE	0x20000	//2^17 Allow Sparks/Embers to settle
#define PROP_NOAMBHEAT      0x40000 //2^18 Don't transfer or receive heat from ambient heat.
#define PROP_DRAWONCTYPE       0x80000  //2^19 Set its ctype to another element if the element is drawn upon it (like what CLNE does)
#define PROP_NOCTYPEDRAW       0x100000 // 2^20 When this element is drawn upon with, do not set ctype (like BCLN for CLNE)

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
// sim is the Simulation object ("FOR_PMAP_POSITION(this, ...)" in Simulation methods)
// x and y are the coordinates of the position to loop through
// r_count, r_i, and r_next are int variables (which need declaring before using this macro). r_count is the number of particles left to iterate in this position, r_i the current particle index, and r_next is temporary storage for the next particle index
// If any particle in the current position is killed or moved except particle r_i and particles that have already been iterated over, then you must break out of the pmap position loop. 
#define FOR_PMAP_POSITION(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count, r_next = (sim)->pmap[(y)][(x)].first; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_NOENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count_notEnergy, r_next = (sim)->pmap[(y)][(x)].first; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)
#define FOR_PMAP_POSITION_ONLYENERGY(sim, x, y, r_count, r_i, r_next) for (r_count=(sim)->pmap[(y)][(x)].count-(sim)->pmap[(y)][(x)].count_notEnergy, r_next = (sim)->pmap[(y)][(x)].first_energy; r_count ? (r_i=r_next, r_next=(sim)->parts[r_i].pmap_next, true):false; r_count--)


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

class SimulationSharedData;

struct pmap_entry
{
	int count;// number of particles, including energy particles
	int first;// ID of first particle
	int count_notEnergy;// number of particles, not including energy particles
	int first_energy;// ID of first energy particle, or if there are no energy particles, ID of last non-energy particle
};
typedef struct pmap_entry pmap_entry;

class Simulation
{
protected:
	ElemDataSim *elemDataSim_[PT_NUM];
public:
	std::shared_ptr<SimulationSharedData> simSD;
	particle parts[NPART];
	int parts_lastActiveIndex;
#ifdef DEBUG_PARTSALLOC
	bool partsFree[NPART];
#endif
	int elementCount[PT_NUM];
	Element *elements;
	pmap_entry pmap[YRES][XRES];
	MoveResult::Code can_move[PT_NUM][PT_NUM];
	int pfree;

	static constexpr float maxVelocity = 1e4f;

	short heat_mode;//Will be a replacement for legacy_enable at some point. 0=heat sim off, 1=heat sim on, 2=realistic heat on (todo)
	short edgeMode;

	ObserverSubject hook_cleared;
	ObserverSubject hook_beforeUpdate;
	ObserverSubject hook_afterUpdate;

	short option_edgeMode() { return edgeMode; }
	void option_edgeMode(short newMode);

	template <class ElemDataClass_T>
	ElemDataClass_T * elemData(int elementId) {
		return static_cast<ElemDataClass_T*>(elemDataSim_[elementId]);
	}

	ElemDataSim * elemData(int elementId) { return elemDataSim_[elementId]; }

	template<class ElemDataClass_T, typename... Args>
	void elemData_create(int elementId, Args&&... args) {
		delete elemDataSim_[elementId];
		elemDataSim_[elementId] = new ElemDataClass_T(args...);
	}

	Simulation(std::shared_ptr<SimulationSharedData> sd);
	~Simulation();
	void Clear();
	bool Check();
	void UpdateParticles();
	void RecalcFreeParticles();
	void RecalcElementCounts();
	void pmap_reset();

	int part_create(int p, float x, float y, int t);
	void part_kill(int i);
	void part_kill(int i, int x, int y);
	bool part_change_type(int i, int x, int y, int t);

	int delete_position(int x, int y, int only_type=0, int except_id=-1);
	int delete_position_notEnergy(int x, int y, int only_type=0, int except_id=-1);

	// Functions for changing particle temperature, respecting temperature caps.
	// The _noLatent functions also set the stored transition energy of the particle to make it appear as though latent heat does not apply - the particles will change into the type that they should be at that temperature, instead of the temperature increase just contributing towards the stored transition energy
	// set_temp sets the temperature to a specific value, add_temp changes the temperature by the given amount (can be positive or negative)
	void part_set_temp(int i, float newTemp);
	void part_set_temp_noLatent(int i, float newTemp);
	void part_add_temp(int i, float change);
	void part_add_temp_noLatent(int i, float change);
	void part_add_energy(int i, float amount);

	void spark_particle_nocheck(int i, int x, int y);
	void spark_particle_nocheck_forceSPRK(int i, int x, int y);
	bool spark_particle(int i, int x, int y);
	bool spark_particle_conductiveOnly(int i, int x, int y);
	int spark_position(int x, int y);
	int spark_position_conductiveOnly(int x, int y);

	bool IsWallBlocking(int x, int y, int type);
	MoveResult::Code part_move(int i, int x,int y, float nxf,float nyf);
	MoveResult::Code part_canMove(int pt, int nx,int ny, bool coordCheckDone=false);
	MoveResult::Code part_canMove_dynamic(int pt, int nx,int ny, int ri, MoveResult::Code result);
	void InitCanMove();
	MoveResult::Code part_move(int i, float nxf,float nyf) {
		return part_move(i, (int)(parts[i].x+0.5f),(int)(parts[i].y+0.5f), nxf,nyf);
	}

	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling

	bool IsValidElement(int t) const
	{
		return (t>=0 && t<PT_NUM && elements[t].Enabled);
	}
	bool InBounds(int x, int y) const
	{
		return (x>=0 && y>=0 && x<XRES && y<YRES);
	}
	// Copy particle properties, except for position and pmap list links
	static void part_copy_properties(const particle& src, particle& dest)
	{
		float tmp_x = dest.x, tmp_y = dest.y;
		int tmp_pmap_prev = dest.pmap_prev, tmp_pmap_next = dest.pmap_next;

		dest = src;

		dest.x = tmp_x;
		dest.y = tmp_y;
		dest.pmap_prev = tmp_pmap_prev;
		dest.pmap_next = tmp_pmap_next;
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
		if (pmap_find_one(((int)(parts[i1].x+0.5f) + (int)(parts[i2].x+0.5f))/2, ((int)(parts[i1].y+0.5f) + (int)(parts[i2].y+0.5f))/2, PT_INSL)>=0)
			return true;
		return false;
	}
	bool is_spark_blocked(int x1, int y1, int x2, int y2) const
	{
		if (pmap_find_one((x1+x2)/2, (y1+y2)/2, PT_INSL)>=0)
			return true;
		return false;
	}
	
	// Returns true if a particle of type t exists halfway between i1 and i2
	bool check_middle_particle_type(int i1, int i2, int t) const
	{
		if (pmap_find_one((int)((parts[i1].x + parts[i2].x)/2+0.5f), (int)((parts[i1].y + parts[i2].y)/2+0.5f), t)>=0)
			return true;
		else
			return false;
	}

	// Most of the time, part_alloc and part_free should not be used directly unless you really know what you're doing. 
	// Use part_create and part_kill instead.
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		if (i>parts_lastActiveIndex)
			parts_lastActiveIndex = i;
#ifdef DEBUG_PARTSALLOC
		if (!partsFree[i])
			printf("Particle allocated that isn't free: %d\n", i);
		partsFree[i] = false;
#endif
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle freed twice: %d\n", i);
		partsFree[i] = true;
#endif
	}
	static void FloatTruncCoords(float & xf, float & yf)
	{
		// hopefully force truncation of floats in x87 registers by storing and reloading from memory, so that rounding issues don't cause particles to appear in the wrong pmap list. If using -mfpmath=sse or an ARM CPU, this may be unnecessary.
		volatile float tmpx = xf, tmpy = yf;
		xf = tmpx, yf = tmpy;
	}
	// Move particle #i to nxf,nyf without any checks or reacting with particles it collides with
	void part_set_pos(int i, int x, int y, float nxf, float nyf)
	{
		FloatTruncCoords(nxf, nyf);
		TranslateCoords(nxf, nyf);

#ifdef DEBUG_PARTSALLOC
		if (partsFree[i])
			printf("Particle moved after free: %d\n", i);
		if ((int)(parts[i].x+0.5f)!=x || (int)(parts[i].y+0.5f)!=y)
			printf("Provided original coords wrong for part_set_pos (particle %d): alleged %d,%d actual %d,%d\n", i, x, y, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f));
#endif

		parts[i].x = nxf, parts[i].y = nyf;
		int nx = (int)(parts[i].x+0.5f), ny = (int)(parts[i].y+0.5f);
		if (ny!=y || nx!=x)
		{
			if (nx<CELL || nx>=XRES-CELL || ny<CELL || ny>=YRES-CELL)
			{
				// part_kill removes the particle from pmap, so use the original coords so it's removed from the correct pmap location
				part_kill(i, x, y);
			}
			else
			{
				pmap_remove(i, x, y, parts[i].type);
				pmap_add(i, nx, ny, parts[i].type);
			}
		}
	}
	void part_set_pos(int i, float nxf, float nyf) {
		part_set_pos(i, (int)(parts[i].x+0.5f), (int)(parts[i].y+0.5f), nxf, nyf);
	}
	
	// Adjust coords to take account of edgeMode setting
	// Returns true if coords were changed
	bool TranslateCoords(int & x, int & y) const
	{
		if (edgeMode == 2)
		{
			int diffx = 0.0f, diffy = 0.0f;
			if (x < CELL)
				diffx = XRES-CELL*2;
			if (x >= XRES-CELL)
				diffx = -(XRES-CELL*2);
			if (y < CELL)
				diffy = YRES-CELL*2;
			if (y >= YRES-CELL)
				diffy = -(YRES-CELL*2);
			if (diffx || diffy)
			{
				x = x+diffx;
				y = y+diffy;
				return true;
			}
		}
		return false;
	}
	bool TranslateCoords(float & xf, float & yf) const
	{
		if (edgeMode == 2)
		{
			int x = (int)(xf+0.5f), y = (int)(yf+0.5f);
			float diffx = 0.0f, diffy = 0.0f;
			if (x < CELL)
				diffx = XRES-CELL*2;
			if (x >= XRES-CELL)
				diffx = -(XRES-CELL*2);
			if (y < CELL)
				diffy = YRES-CELL*2;
			if (y >= YRES-CELL)
				diffy = -(YRES-CELL*2);
			if (diffx || diffy)
			{
				volatile float tmpx = xf+diffx, tmpy = yf+diffy;
				xf = tmpx, yf = tmpy;
				return true;
			}
		}
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

	void pmap_add(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (elements[t].Properties&TYPE_ENERGY)
		{
			if (pmap[y][x].count-pmap[y][x].count_notEnergy)
			{
				// If there are some energy particles already, insert at head of energy particle list
				int prevHead = pmap[y][x].first_energy;
				if (pmap[y][x].count_notEnergy)
				{
					// If there are some non-energy particles, link to end of that list
					parts[i].pmap_prev = parts[prevHead].pmap_prev;
					parts[parts[prevHead].pmap_prev].pmap_next = i;
				}
				else
				{
					parts[i].pmap_prev = -1;
				}
				parts[i].pmap_next = prevHead;
				parts[prevHead].pmap_prev = i;
			}
			else if (pmap[y][x].count)
			{
				// If there are no energy particles, then first_energy is the last non-energy particle. Insert this particle after it.
				int i_prev = pmap[y][x].first_energy;
				parts[i_prev].pmap_next = i;
				parts[i].pmap_prev = i_prev;
				parts[i].pmap_next = -1;
			}
			else
			{
				parts[i].pmap_next = -1;
				parts[i].pmap_prev = -1;
			}
			pmap[y][x].first_energy = i;
			if (!pmap[y][x].count_notEnergy)
				pmap[y][x].first = i;
		}
		else
		{
			if (pmap[y][x].count)
			{
				parts[pmap[y][x].first].pmap_prev = i;
				parts[i].pmap_next = pmap[y][x].first;
			}
			else
			{
				parts[i].pmap_next = -1;
				// If this is the only particle, it is the last non-energy particle too (which is the ID stored in first_energy when there are no energy particles)
				pmap[y][x].first_energy = i;
			}
			parts[i].pmap_prev = -1;
			pmap[y][x].first = i;
		}
		pmap[y][x].count++;
		if (!(elements[t].Properties&TYPE_ENERGY))
			pmap[y][x].count_notEnergy++;
	}
	void pmap_remove(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (parts[i].pmap_prev>=0)
			parts[parts[i].pmap_prev].pmap_next = parts[i].pmap_next;
		if (parts[i].pmap_next>=0)
			parts[parts[i].pmap_next].pmap_prev = parts[i].pmap_prev;
		if (pmap[y][x].first==i)
			pmap[y][x].first = parts[i].pmap_next;

		int energyCount = pmap[y][x].count-pmap[y][x].count_notEnergy;
		if (energyCount<=1)
		{
			if (pmap[y][x].first_energy==i)
			{
				// energyCount==1 and is first_energy: this is the only energy particle left
				// energyCount==0 and is first_energy: this is the last non-energy particle
				// In both cases, set first_energy to pmap_prev so that first_energy is the ID of the last non-energy particle
				pmap[y][x].first_energy = parts[i].pmap_prev;
			}
		}
		else if (pmap[y][x].first_energy==i)
		{
			pmap[y][x].first_energy = parts[i].pmap_next;
		}

		pmap[y][x].count--;
		if (!(elements[t].Properties&TYPE_ENERGY))
			pmap[y][x].count_notEnergy--;
	}
	int pmap_find_one(int x, int y, int t) const
	{
		int count;
		if (elements[t].Properties&TYPE_ENERGY)
			count = pmap[y][x].count;
		else
			count = pmap[y][x].count_notEnergy;
		int i = pmap[y][x].first;
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (parts[i].type==t)
				return i;
		}
		return -1;
	}
	int pmap_find_one_conductive(int x, int y, int t) const
	{
		int count = pmap[y][x].count_notEnergy;
		int i = pmap[y][x].first;
		for (; count>0; i=parts[i].pmap_next, count--)
		{
			if (part_cmp_conductive(parts[i], t))
				return i;
		}
		return -1;
	}
};

void Simulation_Compat_CopyData(Simulation *sim);

extern Simulation *globalSim; // TODO: remove this

#endif

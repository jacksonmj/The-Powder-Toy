/**
 * Powder Toy - element numbers
 *
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

#if (!defined(WallNumbers_Done_Numbers) || (defined(WallNumbers_DeclInit) && !defined(WallNumbers_Done_DeclInit)) || defined(WallNumbers_CallInit) || defined(WallNumbers_CustomAction))

class SimulationSharedData;
class WallType;

// If WallNumbers_DeclInit is defined: declaring WL_AAAA_init functions (set DEFINE_WALL to something appropriate first)
// If WallNumbers_CallInit is defined: calling WL_AAAA_init functions (set DEFINE_WALL to something appropriate first)
// otherwise default: define wall numbers (WL_AAAA=123)

#undef WALL_INIT_FUNC_ARGS
#define WALL_INIT_FUNC_ARGS SimulationSharedData *simSD, WallType *wall, int wt
#undef WallNumbers_Numbers

#if (defined(WallNumbers_DeclInit) && !defined(WallNumbers_Done_DeclInit))
	#define WallNumbers_Done_DeclInit
#elif defined(WallNumbers_CallInit) || defined(WallNumbers_CustomAction)

#else
	#define WallNumbers_Done_Numbers
	#define WallNumbers_Numbers
	#define DEFINE_WALL(name, id) WL_ ## name = id,
#endif

#ifdef WallNumbers_Numbers
enum walltype_ids
{
#endif

DEFINE_WALL(ERASE, 0)
DEFINE_WALL(WALLELEC, 1)
DEFINE_WALL(EWALL, 2)
DEFINE_WALL(DETECT , 3)
DEFINE_WALL(STREAM, 4)
DEFINE_WALL(FAN, 5)
DEFINE_WALL(ALLOWLIQUID, 6)
DEFINE_WALL(DESTROYALL, 7)
DEFINE_WALL(WALL, 8)
DEFINE_WALL(ALLOWAIR, 9)
DEFINE_WALL(ALLOWSOLID, 10)
DEFINE_WALL(ALLOWALLELEC, 11)
DEFINE_WALL(EHOLE, 12)
DEFINE_WALL(ALLOWGAS, 13)
DEFINE_WALL(GRAV, 14)
DEFINE_WALL(ALLOWENERGY, 15)
DEFINE_WALL(BLOCKAIR, 16)

// New walls go above this line


#ifdef WallNumbers_Numbers
WL_NUM
};
#define WL_FLOODHELPER 255
#define WL_NONE WL_ERASE
#endif

#undef DEFINE_WALL
#undef WallNumbers_Numbers
#endif

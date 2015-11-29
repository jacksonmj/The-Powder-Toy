/**
 * Powder Toy - particle simulation (header)
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
#ifndef POWDER_H
#define POWDER_H

#include "Config.h"

#include "graphics.h"
#include "defines.h"
#include "interface.h"
#include "misc.h"

#define CM_COUNT 11
#define CM_CRACK 10
#define CM_LIFE 9
#define CM_GRAD 8
#define CM_NOTHING 7
#define CM_FANCY 6
#define CM_HEAT 5
#define CM_BLOB 4
#define CM_FIRE 3
#define CM_PERS 2
#define CM_PRESS 1
#define CM_VEL 0

#define BRUSH_REPLACEMODE 0x1
#define BRUSH_SPECIFIC_DELETE 0x2

// Ugly way of identifying things until brushes/tools are rewritten
#define UI_WALLOFFSET 1000
#define UI_TOOLOFFSET 2000

#include "simulation/WallNumbers.hpp"

#define WL_SIGN	0
#define SPC_AIR 1
#define SPC_HEAT 2
#define SPC_COOL 3
#define SPC_VACUUM 4
#define SPC_WIND 5
#define SPC_PGRV 6
#define SPC_NGRV 7
#define SPC_PROP 8
#define UI_TOOLCOUNT 9


#define OLD_PT_WIND 147

class Simulation;

#include "simulation/Particle.h"
#include "simulation/Element.h"

void TRON_init_graphics();

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy);

struct gol_menu
{
	const char *name;
	pixel colour;
	int goltype;
	const char *description;
};
typedef struct gol_menu gol_menu;

extern gol_menu gmenu[NGOL];

struct wall_type
{
	pixel colour;
	pixel eglow; // if emap set, add this to fire glow
	int drawstyle;
	const char *descs;
};
typedef struct wall_type wall_type;

extern wall_type wtypes[];

extern const particle emptyparticle;

extern int ppip_changed;

extern particle *parts;
extern particle *cb_parts;

extern int lighting_recreate;

int get_normal_interp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);

int flood_prop(int x, int y, int parttype, size_t propoffset, void * propvalue, int proptype);

int InCurrentBrush(int i, int j, int rx, int ry);

int get_brush_flags();

int create_part(int p, int x, int y, int t);

void delete_part(int x, int y, int flags);

void create_arc(int sx, int sy, int dx, int dy, int midpoints, int variance, int type, int flags);

int nearest_part(int ci, int t, int max_d);

void rotate_area(int area_x, int area_y, int area_w, int area_h, int invert);

void clear_area(int area_x, int area_y, int area_w, int area_h);

void create_box(int x1, int y1, int x2, int y2, int c, int flags);

int flood_parts(int x, int y, int c, int matchElement, int matchWall, int flags);

int flood_water(int x, int y, int i, int originaly, int check);

int create_parts(int x, int y, int rx, int ry, int c, int flags, int fill);

int create_parts2(int f, int x, int y, int c, int rx, int ry, int flags);

void create_line(int x1, int y1, int x2, int y2, int rx, int ry, int c, int flags);

#endif

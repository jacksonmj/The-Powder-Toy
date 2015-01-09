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

#include "air.h"
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

#define UI_WALLSTART 222
#define UI_ACTUALSTART 122
#define UI_WALLCOUNT 26

#define WL_WALLELEC	122
#define WL_EWALL	123
#define WL_DETECT	124
#define WL_STREAM	125
#define WL_SIGN	126
#define WL_FAN	127
#define WL_FANHELPER 255
#define WL_ALLOWLIQUID	128
#define WL_DESTROYALL	129
#define WL_ERASE	130
#define WL_WALL	131
#define WL_ALLOWAIR	132
#define WL_ALLOWSOLID	133
#define WL_ALLOWALLELEC	134
#define WL_EHOLE	135

#define SPC_AIR 236
#define SPC_HEAT 237
#define SPC_COOL 238
#define SPC_VACUUM 239
#define SPC_WIND 241
#define SPC_PGRV 243
#define SPC_NGRV 244
#define SPC_PROP 246

#define WL_ALLOWGAS	140
#define WL_GRAV		142
#define WL_ALLOWENERGY 145
#define WL_BLOCKAIR 147

#include "simulation/ElementNumbers.h"
#define OLD_PT_WIND 147
#define PT_NUM  220

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

class Simulation;
#define UPDATE_FUNC_ARGS Simulation *sim, int i, int x, int y, int surround_space, int nt
#define UPDATE_FUNC_SUBCALL_ARGS sim, i, x, y, surround_space, nt
#define GRAPHICS_FUNC_ARGS Simulation *sim, particle *cpart, int nx, int ny, int *pixel_mode, int* cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS sim, cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb

#include "simulation/Particle.h"

int graphics_DEFAULT(GRAPHICS_FUNC_ARGS);

void TRON_init_graphics();

int update_PYRO(UPDATE_FUNC_ARGS);

int update_MISC(UPDATE_FUNC_ARGS);
int update_legacy_PYRO(UPDATE_FUNC_ARGS);
int update_legacy_all(UPDATE_FUNC_ARGS);

void PPIP_flood_trigger(Simulation* sim, int x, int y, int sparkedBy);

struct part_type
{
	char *name;
	pixel pcolors;
	float advection;
	float airdrag;
	float airloss;
	float loss;
	float collision;
	float gravity;
	float diffusion;
	float hotair;
	int falldown;
	int flammable;
	int explosive;
	int meltable;
	int hardness;
	int menu;
	int enabled;
	int weight;
	int menusection;
	float heat;
	unsigned char hconduct;
	char *descs;
	char state;
	unsigned int properties;
	int (*update_func) (UPDATE_FUNC_ARGS);
	int (*graphics_func) (GRAPHICS_FUNC_ARGS);
};
typedef struct part_type part_type;

struct part_transition
{
	float plv; // transition occurs if pv is lower than this
	int plt;
	float phv; // transition occurs if pv is higher than this
	int pht;
	float tlv; // transition occurs if t is lower than this
	int tlt;
	float thv; // transition occurs if t is higher than this
	int tht;
};
typedef struct part_transition part_transition;

// TODO: falldown, properties, state - should at least one of these be removed?
extern part_type ptypes[PT_NUM];
extern unsigned int platent[PT_NUM];

// temporarily define abbreviations for impossible p/t values
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM
extern part_transition ptransitions[PT_NUM];

//Old IDs for GOL types
#define GT_GOL 78
#define GT_HLIF 79
#define GT_ASIM 80
#define GT_2x2 81
#define GT_DANI 82
#define GT_AMOE 83
#define GT_MOVE 84
#define GT_PGOL 85
#define GT_DMOE 86
#define GT_34 87
#define GT_LLIF 88
#define GT_STAN 89
#define GT_SEED 134
#define GT_MAZE 135
#define GT_COAG 136
#define GT_WALL 137
#define GT_GNAR 138
#define GT_REPL 139
#define GT_MYST 140
#define GT_LOTE 142
#define GT_FRG2 143
#define GT_STAR 144
#define GT_FROG 145
#define GT_BRAN 146
 
//New IDs for GOL types
#define NGT_GOL 0
#define NGT_HLIF 1
#define NGT_ASIM 2
#define NGT_2x2 3
#define NGT_DANI 4
#define NGT_AMOE 5
#define NGT_MOVE 6
#define NGT_PGOL 7
#define NGT_DMOE 8
#define NGT_34 9
#define NGT_LLIF 10
#define NGT_STAN 11
#define NGT_SEED 12
#define NGT_MAZE 13
#define NGT_COAG 14
#define NGT_WALL 15
#define NGT_GNAR 16
#define NGT_REPL 17
#define NGT_MYST 18
#define NGT_LOTE 19
#define NGT_FRG2 20
#define NGT_STAR 21
#define NGT_FROG 22
#define NGT_BRAN 23

struct gol_menu
{
	const char *name;
	pixel colour;
	int goltype;
	const char *description;
};
typedef struct gol_menu gol_menu;

extern gol_menu gmenu[NGOL];
extern int grule[NGOL+1][10];
extern int goltype[NGOL];
static int loverule[9][9] =
{
	{0,0,1,1,0,0,0,0,0},
	{0,1,0,0,1,1,0,0,0},
	{1,0,0,0,0,0,1,0,0},
	{1,0,0,0,0,0,0,1,0},
	{0,1,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,1,0},
	{1,0,0,0,0,0,1,0,0},
	{0,1,0,0,1,1,0,0,0},
	{0,0,1,1,0,0,0,0,0},
};
static int lolzrule[9][9] =
{
	{0,0,0,0,0,0,0,0,0},
	{1,0,0,0,0,0,1,0,0},
	{1,0,0,0,0,0,1,0,0},
	{1,0,0,1,1,0,0,1,0},
	{1,0,1,0,0,1,0,1,0},
	{1,0,1,0,0,1,0,1,0},
	{0,1,0,1,1,0,0,1,0},
	{0,1,0,0,0,0,0,1,0},
	{0,1,0,0,0,0,0,1,0},
};

struct wall_type
{
	pixel colour;
	pixel eglow; // if emap set, add this to fire glow
	int drawstyle;
	const char *descs;
};
typedef struct wall_type wall_type;

extern wall_type wtypes[];

#define CHANNELS ((int)(MAX_TEMP-73)/100+2)
extern particle portalp[CHANNELS][8][80];
extern const particle emptyparticle;
extern const int portal_rx[8];
extern const int portal_ry[8];

extern int force_stacking_check;
extern int ppip_changed;

extern int airMode;

extern particle *parts;
extern particle *cb_parts;
extern int parts_lastActiveIndex;

extern unsigned char bmap[YRES/CELL][XRES/CELL];
extern unsigned char emap[YRES/CELL][XRES/CELL];

extern unsigned char cb_bmap[YRES/CELL][XRES/CELL];
extern unsigned char cb_emap[YRES/CELL][XRES/CELL];

extern unsigned pmap[YRES][XRES];
extern unsigned photons[YRES][XRES];

extern int GRAV;
extern int GRAV_R;
extern int GRAV_G;
extern int GRAV_B;
extern int GRAV_R2;
extern int GRAV_G2;
extern int GRAV_B2;

extern int lighting_recreate;

int get_normal_interp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);

void kill_part(int i);

int flood_prop(int x, int y, int parttype, size_t propoffset, void * propvalue, int proptype);

void part_change_type(int i, int x, int y, int t);

int InCurrentBrush(int i, int j, int rx, int ry);

int get_brush_flags();

int create_part(int p, int x, int y, int t);

void delete_part(int x, int y, int flags);

int is_wire(int x, int y);

int is_wire_off(int x, int y);

void set_emap(int x, int y);

void create_arc(int sx, int sy, int dx, int dy, int midpoints, int variance, int type, int flags);

int nearest_part(int ci, int t, int max_d);

void update_wallmaps();

void rotate_area(int area_x, int area_y, int area_w, int area_h, int invert);

void clear_area(int area_x, int area_y, int area_w, int area_h);

void create_box(int x1, int y1, int x2, int y2, int c, int flags);

int flood_parts(int x, int y, int c, int cm, int bm, int flags);

int flood_water(int x, int y, int i, int originaly, int check);

int create_parts(int x, int y, int rx, int ry, int c, int flags, int fill);

int create_parts2(int f, int x, int y, int c, int rx, int ry, int flags);

void create_line(int x1, int y1, int x2, int y2, int rx, int ry, int c, int flags);

#endif

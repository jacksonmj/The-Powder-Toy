/**
 * Powder Toy - Main source
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
#ifndef DEFINE_H
#define DEFINE_H

#include "common/Compat.hpp"
#include "simulation/Config.hpp"

#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
 
//VersionInfoStart
#define SAVE_VERSION 90
#define MINOR_VERSION 0
#define BUILD_NUM 316
//VersionInfoEnd

#define IDENT_VERSION "G" //Change this if you're not Simon! It should be a single letter

#define MTOS_EXPAND(str) #str
#define MTOS(str) MTOS_EXPAND(str)

#define SERVER "powdertoy.co.uk"
#define SCRIPTSERVER "powdertoy.co.uk"
#define STATICSERVER "static.powdertoy.co.uk"
//#define UPDATESERVER "powdertoy.co.uk" // change this to check for updates on a different server

#ifndef UPDATESERVER
	#define UPDATESERVER "powdertoy.co.uk"
#endif

#define LOCAL_SAVE_DIR "Saves"

#define APPDATA_SUBDIR "\\HardWIRED"

#define THUMB_CACHE_SIZE 256

#define IMGCONNS 3
#define TIMEOUT 100
#define HTTP_TIMEOUT 10

#ifdef RENDERER
#define MENUSIZE 0
#define BARSIZE 0
#else
#define MENUSIZE 40
#define BARSIZE 17
#endif

#define GRAV_DIFF

#define TAG_MAX 256

#define ZSIZE_D	16
#define ZFACTOR_D	8
extern unsigned char ZFACTOR;
extern unsigned char ZSIZE;

#define GRID_X 5
#define GRID_Y 4
#define GRID_P 3
#define GRID_S 6
#define GRID_Z 3

#define CATALOGUE_X 4
#define CATALOGUE_Y 3
#define CATALOGUE_S 6
#define CATALOGUE_Z 3

#define STAMP_MAX 240

#define SAVE_OPS


#define CIRCLE_BRUSH 0
#define SQUARE_BRUSH 1
#define TRI_BRUSH 2
#define BRUSH_NUM 3

#ifdef PIX16
typedef unsigned short pixel;
#else
typedef unsigned int pixel;
#endif

#ifdef WIN32
#define strcasecmp stricmp
#endif
#if defined(_MSC_VER)
#if _MSC_VER < 1800
#define fmin min
#define fminf min
#define fmax max
#define fmaxf max
#else
#include <algorithm>
#endif
#endif

#if defined(DEBUG) || defined(RENDERER) || defined(X86_SSE2)
#define HIGH_QUALITY_RESAMPLE //High quality image resampling, slower but much higher quality than linear interpolation
#endif

#define DEBUG_PARTS		0x0001
#define DEBUG_PARTCOUNT	0x0002
#define DEBUG_DRAWTOOL	0x0004
#define DEBUG_PERFORMANCE_CALC 0x0008
#define DEBUG_PERFORMANCE_FRAME 0x0010

typedef unsigned char uint8;

extern int saveURIOpen;
extern char * saveDataOpen;
extern int saveDataOpenSize;

extern int amd;

extern int FPSB;

extern int legacy_enable;
extern int sound_enable;
extern int kiosk_enable;
extern int decorations_enable;
extern int active_menu;
extern int hud_enable;
extern int pretty_powder;
extern int drawgrav_enable;
extern int ngrav_enable;
extern int limitFPS;
extern int water_equal_test;
extern int quickoptions_tooltip_fade;
extern int loop_time;

extern int debug_flags;
#define DEBUG_PERF_FRAMECOUNT 256
extern int debug_perf_istart;
extern int debug_perf_iend;
extern long debug_perf_frametime[DEBUG_PERF_FRAMECOUNT];
extern long debug_perf_partitime[DEBUG_PERF_FRAMECOUNT];
extern long debug_perf_time;

extern int active_menu;

extern int sys_pause;
extern int framerender;

extern int mousex, mousey;

struct stamp
{
	char name[11];
	pixel *thumb;
	int thumb_w, thumb_h, dodelete;
};
typedef struct stamp stamp;

extern int frameidx;
extern int MSIGN;
extern int SEC;
extern int SEC2;
extern int console_mode;
extern int REPLACE_MODE;
extern int CURRENT_BRUSH;
extern int GRID_MODE;
extern int DEBUG_MODE;
extern stamp stamps[STAMP_MAX];
extern int stamp_count;
extern int itc;
extern char itc_msg[64];

extern int do_open;
extern int sys_pause;
extern int sys_shortcuts;
extern int legacy_enable; //Used to disable new features such as heat, will be set by commandline or save.
extern int framerender;
extern pixel *vid_buf;

extern unsigned char last_major, last_minor, update_flag, last_build;

extern char http_proxy_string[256];

//Functions in main.c
void thumb_cache_inval(const char *id);
void thumb_cache_add(const char *id, void *thumb, int size);
int thumb_cache_find(const char *id, void **thumb, int *size);
void clear_sim(void);
void del_stamp(int d);
void sdl_seticon(void);
void play_sound(char *file);
int set_scale(int scale, int kiosk);
void dump_frame(pixel *src, int w, int h, int pitch);
#endif

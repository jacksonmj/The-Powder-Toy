/**
 * Powder Toy - graphics (header)
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
#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <SDL/SDL.h>
#include "defines.h"

//Defining PIXELBYTES indicates that R, G, B values are stored in separate bytes, and that the pixel format is therefore suitable for use with MMX instructions

#define PIXELCHANNELS 3
#ifdef PIX16
#define PIXELSIZE 2
#define PIXPACK(x) ((((x)>>8)&0xF800)|(((x)>>5)&0x07E0)|(((x)>>3)&0x001F))
#define PIXRGB(r,g,b) ((((r)<<8)&0xF800)|(((g)<<3)&0x07E0)|(((b)>>3)&0x001F))
#define PIXR(x) (((x)>>8)&0xF8)
#define PIXG(x) (((x)>>3)&0xFC)
#define PIXB(x) (((x)<<3)&0xF8)
#else
#define PIXELSIZE 4
#ifdef PIX32BGR
#define PIXPACK(x) ((((x)>>16)&0x0000FF)|((x)&0x00FF00)|(((x)<<16)&0xFF0000))
#define PIXRGB(r,g,b) (((b)<<16)|((g)<<8)|((r)))// (((b)<<16)|((g)<<8)|(r))
#define PIXR(x) ((x)&0xFF)
#define PIXG(x) (((x)>>8)&0xFF)
#define PIXB(x) (((x)>>16)&0xFF)
#define PIXELBYTES 1
#else
#ifdef PIX32BGRA
#define PIXPACK(x) ((((x)>>8)&0x0000FF00)|(((x)<<8)&0x00FF0000)|(((x)<<24)&0xFF000000))
#define PIXRGB(r,g,b) (((b)<<24)|((g)<<16)|((r)<<8))
#define PIXR(x) (((x)>>8)&0xFF)
#define PIXG(x) (((x)>>16)&0xFF)
#define PIXB(x) (((x)>>24)&0xFF)
#define PIXELBYTES 1
#elif defined(PIX32OGL)
#define PIXELCHANNELS 4
#define PIXPACK(x) (0xFF000000|((x)&0xFFFFFF))
#define PIXRGB(r,g,b) (0xFF000000|((r)<<16)|((g)<<8)|((b)))
#define PIXRGBA(r,g,b,a) (((a)<<24)|((r)<<16)|((g)<<8)|((b)))
#define PIXA(x) (((x)>>24)&0xFF)
#define PIXR(x) (((x)>>16)&0xFF)
#define PIXG(x) (((x)>>8)&0xFF)
#define PIXB(x) ((x)&0xFF)
#define PIXELBYTES 1
#else
#define PIXPACK(x) (x)
#define PIXRGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define PIXR(x) (((x)>>16)&0xFF)
#define PIXG(x) (((x)>>8)&0xFF)
#define PIXB(x) ((x)&0xFF)
#define PIXELBYTES 1
#endif
#endif
#endif

#define ARGBColour_TO_PIXEL(x) (PIXRGB(COLR(x),COLG(x),COLB(x)))

extern unsigned int *render_modes;
extern unsigned int render_mode;
extern unsigned int colour_mode;
extern unsigned int *display_modes;
extern unsigned int display_mode;

extern SDL_Surface *sdl_scrn;
extern int sdl_scale;

extern int sandcolour;
extern int sandcolour_frame;

extern unsigned char fire_r[YRES/CELL][XRES/CELL];
extern unsigned char fire_g[YRES/CELL][XRES/CELL];
extern unsigned char fire_b[YRES/CELL][XRES/CELL];

extern unsigned int fire_alpha[CELL*3][CELL*3];
extern pixel *pers_bg;

extern char * flm_data;
extern int flm_data_points;
extern pixel flm_data_colours[];
extern float flm_data_pos[];

extern char * plasma_data;
extern int plasma_data_points;
extern pixel plasma_data_colours[];
extern float plasma_data_pos[];

struct gcache_item
{
	int isready;
	int pixel_mode;
	int cola, colr, colg, colb;
	int firea, firer, fireg, fireb;
};
typedef struct gcache_item gcache_item;

extern gcache_item *graphicscache;

void prepare_graphicscache();

char * generate_gradient(pixel * colours, float * points, int pointcount, int size);

void draw_other(pixel *vid);

void draw_rgba_image(pixel *vid, unsigned char *data, int x, int y, float a);

void *ptif_pack(pixel *src, int w, int h, int *result_size);

pixel *ptif_unpack(void *datain, int size, int *w, int *h);

pixel *resample_img_nn(pixel *src, int sw, int sh, int rw, int rh);

pixel *resample_img(pixel *src, int sw, int sh, int rw, int rh);

pixel *rescale_img(pixel *src, int sw, int sh, int *qw, int *qh, int f);

void render_gravlensing(pixel *src, pixel * dst);

void sdl_blit_1(int x, int y, int w, int h, pixel *src, int pitch);

void sdl_blit_2(int x, int y, int w, int h, pixel *src, int pitch);

void sdl_blit(int x, int y, int w, int h, pixel *src, int pitch);

void drawblob(pixel *vid, int x, int y, unsigned char cr, unsigned char cg, unsigned char cb);

void draw_tool(pixel *vid_buf, int b, int sl, int sr, unsigned pc, unsigned iswall);

int draw_tool_xy(pixel *vid_buf, int x, int y, int b, unsigned pc);

void draw_menu(pixel *vid_buf, int i, int hover);

void drawpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

int addchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawtext(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a);

int drawtext_outline(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a, int outr, int outg, int outb, int outa);

int drawtextwrap(pixel *vid, int x, int y, int w, const char *s, int r, int g, int b, int a);

void drawrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void fillrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void clearrect(pixel *vid, int x, int y, int w, int h);

void drawdots(pixel *vid, int x, int y, int h, int r, int g, int b, int a);

int textwidth(const char *s);

int drawtextmax(pixel *vid, int x, int y, int w, char *s, int r, int g, int b, int a);

int textnwidth(char *s, int n);

void textnpos(char *s, int n, int w, int *cx, int *cy);

int textwidthx(char *s, int w);

int textposxy(char *s, int width, int w, int h);

int textwrapheight(char *s, int width);

void blendpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void draw_icon(pixel *vid_buf, int x, int y, char ch, int flag);

void draw_air(pixel *vid);

void draw_grav_zones(pixel *vid);

void draw_grav(pixel *vid);

void draw_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int a);

void addpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void xor_pixel(int x, int y, pixel *vid);

void xor_line(int x1, int y1, int x2, int y2, pixel *vid);

void xor_rect(pixel *vid, int x, int y, int w, int h);

void blend_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int a);

void render_parts(pixel *vid);

void render_before(pixel *part_vbuf);

void render_after(pixel *part_vbuf, pixel *vid_buf);

#ifdef OGLR
void draw_parts_fbo();
#endif

void draw_parts(pixel *vid);

void draw_walls(pixel *vid);

void create_decorations(int x, int y, int rx, int ry, int r, int g, int b, int click, int tool);

void create_decoration(int x, int y, int r, int g, int b, int click, int tool);

void line_decorations(int x1, int y1, int x2, int y2, int rx, int ry, int r, int g, int b, int click, int tool);

void box_decorations(int x1, int y1, int x2, int y2, int r, int g, int b, int click, int tool);

void draw_color_menu(pixel *vid_buf, int i, int hover);

void draw_wavelengths(pixel *vid, int x, int y, int h, int wl);

void render_signs(pixel *vid_buf);

void render_fire(pixel *dst);

void prepare_alpha(int size, float intensity);

void draw_image(pixel *vid, pixel *img, int x, int y, int w, int h, int a);

void dim_copy(pixel *dst, pixel *src);

void dim_copy_pers(pixel *dst, pixel *src);

void render_zoom(pixel *img);

int render_thumb(void *thumb, int size, int bzip2, pixel *vid_buf, int px, int py, int scl);

void render_cursor(pixel *vid, int x, int y, int t, int rx, int ry);

int sdl_open(void);

void draw_debug_info(pixel* vid, int lm, int lx, int ly, int cx, int cy, int line_x, int line_y);

void init_display_modes();
void update_display_modes();

#ifdef OGLR
void clearScreen(float alpha);
void ogl_blit(int x, int y, int w, int h, pixel *src, int pitch, int scale);
#endif

#endif

#ifdef INCLUDE_SHADERS
extern const char * fireFragment;
extern const char * fireVertex;
extern const char * lensFragment;
extern const char * lensVertex;
extern const char * airVFragment;
extern const char * airVVertex;
extern const char * airPFragment;
extern const char * airPVertex;
extern const char * airCFragment;
extern const char * airCVertex;
#endif

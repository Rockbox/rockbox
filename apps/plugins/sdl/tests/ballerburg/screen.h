/*
    screen.h - prototypes and definitions for screen.c

    Copyright (C) 2010, 2013  Thomas Huth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>

extern SDL_Surface *surf;

void scr_init(void);
void scr_exit(void);
void scr_togglefullscreen(void);
void scr_clear(void);
void scr_l_text(int x, int y, const char *text);
void scr_ctr_text(int cx, int y, const char *text);
void scr_circle(int x, int y, int w);
void scr_sf_interior(short val);
void scr_sf_style(short val);
void scr_bar(short *xy);
void clr(short x, short y, short w, short h);
void clr_bg(short x, short y, short w, short h);
void scr_fillarea(short num, short *xy);
void scr_pline(short num, short *xy);
void scr_line(int x1, int y1, int x2, int y2, int rgba);
int scr_getpixel(int x, int y);
void scr_color(int c);
void scr_fillcolor(int c);
void color(int a);
void scr_init_done_button(int *bx, int *by, int *bw, int *bh);
void scr_draw_done_button(int selected);
void scr_cannonball(int x, int y);
void *scr_save_bg(int x, int y, int w, int h);
void scr_restore_bg(void *ps);
void scr_update(int x, int y, int w, int h);

int DlgAlert_Notice(const char *text, const char *button);
int DlgAlert_Query(const char *text, const char *button1, const char *button2);

#if WITH_SDL2
void SDL_UpdateRects(SDL_Surface *screen, int numrects, SDL_Rect *rects);
void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Sint32 w, Sint32 h);
#endif

/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg <daniel@haxx.se>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "screenhack.h"

#include "version.h"

#include "lcd-x11.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

#define PROGNAME "rockboxui"

/* -- -- */

GC draw_gc, erase_gc;
static Colormap cmap;
static XColor color_track, color_car;

static long maxx, maxy;
static double track_zoom=1;

Display *dpy;
Window window;

XrmOptionDescRec options [] = {
  /* { "-subtractive",	".additive",	XrmoptionNoArg, "false" }, */
  { "-server",		".server",	XrmoptionSepArg, 0 },
  { "-help",		".help",	XrmoptionNoArg, "false" },
  { 0, 0, 0, 0 }
};
char *progclass = "rockboxui";

char *defaults [] = {
  ".background:	lightgreen",
  ".foreground:	black",
  "*help:       false",
  0
};

void init_window ()
{
  XGCValues gcv;
  XWindowAttributes xgwa;

  XGetWindowAttributes (dpy, window, &xgwa);

  color_track.red=65535;
  color_track.green=65535;
  color_track.blue=65535;

  color_car.red=65535;
  color_car.green=65535;
  color_car.blue=0;

  cmap = xgwa.colormap;

  gcv.function = GXxor;
  gcv.foreground =
    get_pixel_resource ("foreground", "Foreground", dpy, cmap);
  draw_gc = erase_gc = XCreateGC (dpy, window, GCForeground, &gcv);
  XAllocColor (dpy, cmap, &color_track);
  XAllocColor (dpy, cmap, &color_car);

  screen_resized(200, 100);
}

void screen_resized(int width, int height)
{
#if 0
  XWindowAttributes xgwa;
  XGetWindowAttributes (dpy, window, &xgwa);
  maxx = ((long)(xgwa.width));
  maxy = ((long)(xgwa.height));
#else
  maxx = width-1;
  maxy = height-1;
#endif
  XSetForeground (dpy, draw_gc, get_pixel_resource ("background", "Background",
                                                    dpy, cmap));
  XFillRectangle(dpy, window, draw_gc, 0, 0, width, height);

}

static void help(void)
{
  printf(PROGNAME " " ROCKBOXUI_VERSION " " __DATE__ "\n"
         "usage: " PROGNAME "\n"
         );
}

void drawline(int color, int x1, int y1, int x2, int y2)
{
  if (color==0) {
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("background", "Background", dpy, cmap));
  }
  else
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("foreground", "Foreground", dpy, cmap));

  XDrawLine(dpy, window, draw_gc, 
            (int)(x1*track_zoom), 
            (int)(y1*track_zoom), 
            (int)(x2*track_zoom), 
            (int)(y2*track_zoom));
}

void drawdot(int color, int x, int y)
{
  if (color==0) {
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("background", "Background", dpy, cmap));
  }
  else
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("foreground", "Foreground", dpy, cmap));

  XDrawPoint(dpy, window, draw_gc, x, y);
}

void drawdots(int color, XPoint *points, int count)
{
  if (color==0) {
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("background", "Background", dpy, cmap));
  }
  else
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("foreground", "Foreground", dpy, cmap));

  
  XDrawPoints(dpy, window, draw_gc, points, count, CoordModeOrigin);
}

void drawtext(int color, int x, int y, char *text)
{
  if (color==0) {
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("background", "Background", dpy, cmap));
  }
  else
    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("foreground", "Foreground", dpy, cmap));

  XDrawString(dpy, window, draw_gc, x, y, text, strlen(text));
}

/* this is where the applicaton starts */
extern void app_main(void);

void
screenhack (Display *the_dpy, Window the_window)
{
  Bool helpme;

  /* This doesn't work, but I don't know why (Daniel 1999-12-01) */
  helpme = get_boolean_resource ("help", "Boolean");
  if(helpme) {
    help();
  }
  printf(PROGNAME " " ROCKBOXUI_VERSION " (" __DATE__ ")\n");

  dpy=the_dpy;
  window=the_window;

  init_window();

  screen_redraw();

#ifdef HAVE_LCD_CHARCELLS
  // FIXME??
  lcd_setfont(2);
#endif

  app_main();
}

void screen_redraw()
{
  /* draw a border around the "Recorder" screen */

#define X1 0
#define Y1 0
#ifdef HAVE_LCD_BITMAP
#define X2 (LCD_WIDTH + MARGIN_X*2)
#define Y2 (LCD_HEIGHT + MARGIN_Y)
#else
#define X2 137
#define Y2 53
#endif

  drawline(1, X1, Y1, X2, Y1);
  drawline(1, X2, Y1, X2, Y2);
  drawline(1, X1, Y2, X2, Y2);
  drawline(1, X1, Y1, X1, Y2);

  lcd_update();
}

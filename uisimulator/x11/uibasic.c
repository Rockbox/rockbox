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

#include "config.h"
#include "screenhack.h"

#include "version.h"

#include "lcd-x11.h"
#include "lcd-playersim.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

#define PROGNAME "rockboxui"

/* -- -- */

GC draw_gc;
static Colormap cmap;

int display_zoom=1;

Display *dpy;
Window window;
bool lcd_display_redraw=true;

XrmOptionDescRec options [] = {
    /* { "-subtractive", ".additive", XrmoptionNoArg, "false" }, */
    { "-server",      ".server",   XrmoptionSepArg, 0 },
    { "-help",        ".help",     XrmoptionNoArg, "false" },
    { 0, 0, 0, 0 }
};
char *progclass = "rockboxui";

char *defaults [] = {
#ifdef IRIVER_H100
    ".background: lightblue",
#elif defined ARCHOS_GMINI120
    ".background: royalblue",
#else
    ".background: lightgreen",
#endif
    ".foreground: black",
    "*help:       false",
    0
};

void init_window ()
{
    XGCValues gcv;
    XWindowAttributes xgwa;

    XGetWindowAttributes (dpy, window, &xgwa);

    cmap = xgwa.colormap;

    gcv.function = GXxor;
    gcv.foreground = get_pixel_resource("foreground", "Foreground", dpy, cmap);
    draw_gc = XCreateGC (dpy, window, GCForeground, &gcv);

    screen_resized(LCD_WIDTH, LCD_HEIGHT);
}

void screen_resized(int width, int height)
{
    int maxx, maxy;
    maxx = width;
    maxy = height;

    XSetForeground(dpy, draw_gc,
                   get_pixel_resource("background", "Background", dpy, cmap));
    XFillRectangle(dpy, window, draw_gc, 0, 0, width*display_zoom,
                   height*display_zoom);
    lcd_display_redraw=true;
    screen_redraw();
}

void drawrect(int color, int x1, int y1, int x2, int y2)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    XFillRectangle(dpy, window, draw_gc, x1*display_zoom, y1*display_zoom,
                   x2*display_zoom, y2*display_zoom);
}

static void help(void)
{
    printf(PROGNAME " " ROCKBOXUI_VERSION " " __DATE__ "\n"
           "usage: " PROGNAME "\n");
}

void drawline(int color, int x1, int y1, int x2, int y2)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    XDrawLine(dpy, window, draw_gc,
              (int)(x1*display_zoom),
              (int)(y1*display_zoom),
              (int)(x2*display_zoom),
              (int)(y2*display_zoom));
}

void drawdot(int color, int x, int y)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    XFillRectangle(dpy, window, draw_gc, x*display_zoom, y*display_zoom,
                   display_zoom, display_zoom);
}

void drawdots(int color, struct coordinate *points, int count)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    while (count--) {
        XFillRectangle(dpy, window, draw_gc,
                       points[count].x*display_zoom,
                       points[count].y*display_zoom,
                       display_zoom,
                       display_zoom);
    }
}

void drawrectangles(int color, struct rectangle *points, int count)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    while (count--) {
        XFillRectangle(dpy, window, draw_gc,
                       points[count].x*display_zoom,
                       points[count].y*display_zoom,
                       points[count].width*display_zoom,
                       points[count].height*display_zoom);
    }
}

void drawtext(int color, int x, int y, char *text)
{
    if (color==0)
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("background", "Background", dpy, cmap));
    else
        XSetForeground(dpy, draw_gc,
                       get_pixel_resource("foreground", "Foreground", dpy, cmap));

    XDrawString(dpy, window, draw_gc, x*display_zoom, y*display_zoom, text,
                strlen(text));
}

/* this is where the applicaton starts */
extern void app_main(void);

void
screenhack (Display *the_dpy, Window the_window)
{
    Bool helpme;

    /* This doesn't work, but I don't know why (Daniel 1999-12-01) */
    helpme = get_boolean_resource ("help", "Boolean");
    if(helpme)
        help();

    printf(PROGNAME " " ROCKBOXUI_VERSION " (" __DATE__ ")\n");

    dpy=the_dpy;
    window=the_window;

    init_window();

    screen_redraw();

    app_main();
}

void screen_redraw()
{
    /* draw a border around the "Recorder" screen */
#define X1 0
#define Y1 0
#define X2 (LCD_WIDTH + MARGIN_X*2)
#define Y2 (LCD_HEIGHT + MARGIN_Y)

    drawline(1, X1, Y1, X2, Y1);
    drawline(1, X2, Y1, X2, Y2);
    drawline(1, X1, Y2, X2, Y2);
    drawline(1, X1, Y1, X1, Y2);
    lcd_update();
}

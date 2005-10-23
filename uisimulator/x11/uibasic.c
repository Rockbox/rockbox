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
bool lcd_display_redraw=true;

XrmOptionDescRec options [] = {
    /* { "-subtractive", ".additive", XrmoptionNoArg, "false" }, */
    { "-server",      ".server",   XrmoptionSepArg, 0 },
    { "-help",        ".help",     XrmoptionNoArg, "false" },
    { 0, 0, 0, 0 }
};
char *progclass = "rockboxui";

#ifdef IRIVER_H100_SERIES
#define BGCOLOR "lightblue"
#elif defined ARCHOS_GMINI120
#define BGCOLOR "royalblue"
#else
#define BGCOLOR "lightgreen"
#endif


char *defaults [] = {
    ".background: " BGCOLOR,
    ".foreground: black",
    "*help:       false",
    0
};

static XColor getcolor[4];

/* set a range of bitmap indices to a gradient from startcolour to endcolour
   inherited from the win32 sim code by Jens Arnold */
static void lcdcolors(int index, int count, XColor *start, XColor *end)
{
    int i;
    count--;
    for (i = 0; i <= count; i++)
    {
        getcolor[i+index].red = start->red
            + (end->red - start->red) * i / count;
        getcolor[i+index].green = start->green
            + (end->green - start->green) * i / count;
        getcolor[i+index].blue = start->blue
            + (end->blue - start->blue) * i / count;
        XAllocColor (dpy, cmap, &getcolor[i+index]);
    }
}


void init_window ()
{
    XGCValues gcv;
    XWindowAttributes xgwa;

    XGetWindowAttributes (dpy, window, &xgwa);
    XColor bg;
    XColor fg;

    cmap = xgwa.colormap;

    XParseColor (dpy, cmap, BGCOLOR, &bg);
    XParseColor (dpy, cmap, "black", &fg);
    getcolor[0] = bg;
    getcolor[1] = bg;
    getcolor[2] = bg;
    getcolor[3] = bg;

    lcdcolors(0, 4, &bg, &fg);

#if 0
    for(i=0; i<4; i++) {
        printf("color %d: %d %d %d\n",
               i,
               getcolor[i].red,
               getcolor[i].green,
               getcolor[i].blue);
    }
#endif
    
    gcv.function = GXxor;
    gcv.foreground = getcolor[3].pixel;
    draw_gc = XCreateGC (dpy, window, GCForeground, &gcv);

    screen_resized(LCD_WIDTH, LCD_HEIGHT);
}

void screen_resized(int width, int height)
{
    int maxx, maxy;
    maxx = width;
    maxy = height;

    XtAppLock(app);
    XSetForeground(dpy, draw_gc, getcolor[0].pixel);

    XFillRectangle(dpy, window, draw_gc, 0, 0, width*display_zoom,
                   height*display_zoom);
    XtAppUnlock(app);
    screen_redraw();
}

void drawrect(int color, int x1, int y1, int x2, int y2)
{
    XtAppLock(app);
    XSetForeground(dpy, draw_gc, getcolor[color].pixel);
    XFillRectangle(dpy, window, draw_gc, x1*display_zoom, y1*display_zoom,
                   x2*display_zoom, y2*display_zoom);
    XtAppUnlock(app);
}

static void help(void)
{
    printf(PROGNAME " " ROCKBOXUI_VERSION " " __DATE__ "\n"
           "usage: " PROGNAME "\n");
}

static void drawline(int color, int x1, int y1, int x2, int y2)
{
    XtAppLock(app);
    XSetForeground(dpy, draw_gc, getcolor[color].pixel);

    XDrawLine(dpy, window, draw_gc,
              (int)(x1*display_zoom),
              (int)(y1*display_zoom),
              (int)(x2*display_zoom),
              (int)(y2*display_zoom));
    XtAppUnlock(app);
}

void dots(int *colors, struct coordinate *points, int count)
{
    int color;
    XtAppLock(app);

    while (count--) {
        color = colors[count];
        XSetForeground(dpy, draw_gc, getcolor[color].pixel);
        XFillRectangle(dpy, window, draw_gc,
                       points[count].x*display_zoom,
                       points[count].y*display_zoom,
                       display_zoom,
                       display_zoom);
    }
    XtAppUnlock(app);
}

/* this is where the applicaton starts */
extern void app_main(void);

void screenhack()
{
    Bool helpme;

    /* This doesn't work, but I don't know why (Daniel 1999-12-01) */
    helpme = get_boolean_resource ("help", "Boolean");
    if(helpme)
        help();

    printf(PROGNAME " " ROCKBOXUI_VERSION " (" __DATE__ ")\n");

    init_window();

    screen_redraw();

    app_main();
}

/* used for the player sim */
void drawdots(int color, struct coordinate *points, int count)
{
    XtAppLock(app);
    XSetForeground(dpy, draw_gc, getcolor[color==0?0:3].pixel);

    while (count--) {
        XFillRectangle(dpy, window, draw_gc,
                       points[count].x*display_zoom,
                       points[count].y*display_zoom,
                       display_zoom,
                       display_zoom);
    }
    XtAppUnlock(app);
}

/* used for the player sim */
void drawrectangles(int color, struct rectangle *points, int count)
{
    XtAppLock(app);

    XSetForeground(dpy, draw_gc, getcolor[color==0?0:3].pixel);
    while (count--) {
        XFillRectangle(dpy, window, draw_gc,
                       points[count].x*display_zoom,
                       points[count].y*display_zoom,
                       points[count].width*display_zoom,
                       points[count].height*display_zoom);
    }
    XtAppUnlock(app);
}


void screen_redraw()
{
    /* draw a border around the screen */
#define X1 0
#define Y1 0
#define X2 (LCD_WIDTH + 2*MARGIN_X - 1)
#define Y2 (LCD_HEIGHT + 2*MARGIN_Y - 1)

    drawline(1, X1, Y1, X2, Y1);
    drawline(1, X2, Y1, X2, Y2);
    drawline(1, X1, Y2, X2, Y2);
    drawline(1, X1, Y1, X1, Y2);
    lcd_display_redraw = true;
    lcd_update();
#ifdef LCD_REMOTE_HEIGHT
    /* draw a border around the remote LCD screen */
#define RX1 0
#define RY1 (Y2 + 1)
#define RX2 (LCD_REMOTE_WIDTH + 2*MARGIN_X - 1)
#define RY2 (RY1 + LCD_REMOTE_HEIGHT + 2*MARGIN_Y - 1)

    drawline(1, RX1, RY1, RX2, RY1);
    drawline(1, RX2, RY1, RX2, RY2);
    drawline(1, RX1, RY2, RX2, RY2);
    drawline(1, RX1, RY1, RX1, RY2);
    lcd_display_redraw = true;
    lcd_remote_update();
#endif
}

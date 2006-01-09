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

#include <SDL.h>

#include "config.h"
#include "screenhack.h"

#include "version.h"

#include "lcd-x11.h"
#include "lcd-playersim.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

#define PROGNAME "rockboxui"

/* -- -- */

extern SDL_Surface *surface;

int display_zoom=2;

bool lcd_display_redraw=true;

char *progclass = "rockboxui";

void init_window ()
{
    /* stub */
}

/* used for the player sim */
void drawdots(int color, struct coordinate *points, int count)
{
    SDL_Rect rect;
    
    while (count--) {
        rect.x = points[count].x * display_zoom;
        rect.y = points[count].y * display_zoom;
        rect.w = display_zoom;
        rect.h = display_zoom;
        
        SDL_FillRect(surface, &rect, color);
    }
}

void drawrect(int color, int x1, int y1, int x2, int y2)
{
    SDL_Rect rect;
    
    rect.x = x1 * display_zoom;
    rect.y = y1 * display_zoom;
    rect.w = (x2-x1) * display_zoom;
    rect.h = (y2-y1) * display_zoom;

    SDL_FillRect(surface, &rect, color);
}

#if 0
static void help(void)
{
    printf(PROGNAME " " ROCKBOXUI_VERSION " " __DATE__ "\n"
           "usage: " PROGNAME "\n");
}
#endif 

void dots(int *colors, struct coordinate *points, int count)
{
    int bpp = surface->format->BytesPerPixel;

    if (SDL_MUSTLOCK(surface)) {
        if (SDL_LockSurface(surface)) {
            fprintf(stderr, "cannot lock surface: %s", SDL_GetError());
            exit(-1);
        }
    }

    while (count--) {
        int x_off, y_off;

        for (x_off = 0; x_off < display_zoom; x_off++) {
            for (y_off = 0; y_off < display_zoom; y_off++) {
                int x = points[count].x*display_zoom + x_off;
                int y = points[count].y*display_zoom + y_off;

                Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

		        switch (bpp) {
		            case 1:
		                *p = colors[count];
		                break;
		
		            case 2:
		                *(Uint16 *)p = colors[count];
		                break;
		
		            case 3:
		                if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
		                    p[0] = (colors[count] >> 16) & 0xff;
		                    p[1] = (colors[count] >> 8) & 0xff;
		                    p[2] = (colors[count]) & 0xff;
		                } else {
		                    p[2] = (colors[count] >> 16) & 0xff;
		                    p[1] = (colors[count] >> 8) & 0xff;
		                    p[0] = (colors[count]) & 0xff;
		                }
		                break;
		
		            case 4:
		                *(Uint32 *)p = colors[count];
		                break;
		        }
            }
        }
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    SDL_UpdateRect(surface, 0, 0, 0, 0);
}

/* this is where the applicaton starts */
extern void app_main(void);

void screenhack()
{
#if 0
    Bool helpme;

    /* This doesn't work, but I don't know why (Daniel 1999-12-01) */
    helpme = get_boolean_resource ("help", "Boolean");
    if(helpme)
        help();
#endif

    printf(PROGNAME " " ROCKBOXUI_VERSION " (" __DATE__ ")\n");

    init_window();

    screen_redraw();

    app_main();
}

/* used for the player sim */
void drawrectangles(int color, struct rectangle *points, int count)
{
    SDL_Rect rect;
    Uint32 sdl_white = SDL_MapRGB(surface->format, 255, 255, 255);
    Uint32 sdl_black = SDL_MapRGB(surface->format, 0, 0, 0);

    while (count--) {
        rect.x = points[count].x * display_zoom;
        rect.y = points[count].y * display_zoom;
        rect.w = points[count].width * display_zoom;
        rect.h = points[count].height * display_zoom;
        
        SDL_FillRect(surface, &rect, color ? sdl_white : sdl_black);
    }
}


void screen_redraw()
{
    /* draw a border around the screen */
    int X1 = 0;
    int Y1 = 0;
    int X2 = LCD_WIDTH + 2*MARGIN_X - 1;
    int Y2 = LCD_HEIGHT + 2*MARGIN_Y - 1;

    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), X1, Y1, X2, Y1+1);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), X2, Y1, X2+1, Y2);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), X1, Y2, X2, Y2+1);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), X1, Y1, X1+1, Y2);

    lcd_display_redraw = true;
    lcd_update();

#ifdef LCD_REMOTE_HEIGHT
    /* draw a border around the remote LCD screen */
    int RX1 = 0;
    int RY1 = Y2 + 1;
    int RX2 = LCD_REMOTE_WIDTH + 2*MARGIN_X - 1;
    int RY2 = RY1 + LCD_REMOTE_HEIGHT + 2*MARGIN_Y - 1;

    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), RX1, RY1, RX2, RY1+1);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), RX2, RY1, RX2+1, RY2);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), RX1, RY2, RX2, RY2+1);
    drawrect(SDL_MapRGB(surface->format, 255, 255, 255), RX1, RY1, RX1+1, RY2);

    lcd_display_redraw = true;
    lcd_remote_update();
#endif
}

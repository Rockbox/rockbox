/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"
#include "lcd-playersim.h"
#include "uisdl.h"
#include "lcd-sdl.h"

SDL_Surface* lcd_surface;
SDL_Color lcd_color_zero = {UI_LCD_BGCOLORLIGHT, 0};
SDL_Color lcd_color_max  = {0, 0, 0, 0};

/* Defined in lcd-playersim.c */
extern void lcd_print_char(int x, int y);

void lcd_update(void)
{
    int x, y;
    SDL_Rect dest = {UI_LCD_POSX, UI_LCD_POSY, UI_LCD_WIDTH, UI_LCD_HEIGHT};

    SDL_LockSurface(lcd_surface);
    
    for (y=0; y<2; y++) {
        for (x=0; x<11; x++) {
            lcd_print_char(x, y);
        }
    }
    
    SDL_UnlockSurface(lcd_surface);

    if (!background) {
        dest.x -= UI_LCD_POSX;
        dest.y -= UI_LCD_POSY;
    }
    
    SDL_BlitSurface(lcd_surface, NULL, gui_surface, &dest);
    SDL_UpdateRect(gui_surface, dest.x, dest.y, dest.w, dest.h);
    SDL_Flip(gui_surface);
}

void drawdots(int color, struct coordinate *points, int count)
{
    SDL_Rect dest;
    Uint32 sdlcolor;
    
    SDL_LockSurface(lcd_surface);

    if (color == 1) {
        sdlcolor = SDL_MapRGB(lcd_surface->format, lcd_color_max.r, lcd_color_max.g, lcd_color_max.b);
    } else {
        sdlcolor = SDL_MapRGB(lcd_surface->format, lcd_color_zero.r, lcd_color_zero.g, lcd_color_zero.b);
    }

    while (count--) {
        dest.x = points[count].x * display_zoom;
        dest.y = points[count].y * display_zoom;
        dest.w = 1 * display_zoom;
        dest.h = 1 * display_zoom;

        SDL_FillRect(lcd_surface, &dest, sdlcolor);
    }

    SDL_UnlockSurface(lcd_surface);
}

void drawrectangles(int color, struct rectangle *points, int count)
{
    SDL_Rect dest;
    Uint32 sdlcolor;

    SDL_LockSurface(lcd_surface);

    if (color == 1) {
        sdlcolor = SDL_MapRGB(lcd_surface->format, lcd_color_max.r, lcd_color_max.g, lcd_color_max.b);
    } else {
        sdlcolor = SDL_MapRGB(lcd_surface->format, lcd_color_zero.r, lcd_color_zero.g, lcd_color_zero.b);
    }
    
    while (count--) {
        dest.x = points[count].x * display_zoom;
        dest.y = points[count].y * display_zoom;
        dest.w = points[count].width * display_zoom;
        dest.h = points[count].height * display_zoom;
        
        SDL_FillRect(lcd_surface, &dest, sdlcolor);
    }

    SDL_UnlockSurface(lcd_surface);
}

/* initialise simulator lcd driver */
void sim_lcd_init(void)
{
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH * display_zoom,
        LCD_HEIGHT * display_zoom, 8, 0, 0, 0, 0);

    sdl_set_gradient(lcd_surface, &lcd_color_zero, &lcd_color_max, (1<<LCD_DEPTH));
}


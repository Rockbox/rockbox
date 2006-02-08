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

#include "uisdl.h"
#include "lcd.h"
#include "lcd-playersim.h"

SDL_Surface* lcd_surface;

#if LCD_DEPTH == 16
#else
SDL_Color lcd_palette[(1<<LCD_DEPTH)];
SDL_Color lcd_color_zero = {UI_LCD_BGCOLORLIGHT, 0};
SDL_Color lcd_color_max  = {0, 0, 0, 0};

#endif

#ifdef HAVE_LCD_BITMAP

#ifdef HAVE_REMOTE_LCD
SDL_Surface *remote_surface;
SDL_Color remote_palette[(1<<LCD_REMOTE_DEPTH)];
SDL_Color remote_color_zero = {UI_REMOTE_BGCOLORLIGHT, 0};
SDL_Color remote_color_max  = {0, 0, 0, 0};

#endif

void lcd_update (void)
{
    /* update a full screen rect */
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start, int width, int height)
{
    int x, y;
    int xmax, ymax;

    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT;

    SDL_LockSurface(lcd_surface);

    int bpp = lcd_surface->format->BytesPerPixel;

    for (x = x_start; x < xmax; x++)
    {
        for (y = y_start; y < ymax; y++)
        {
            Uint8 *p = (Uint8 *)lcd_surface->pixels + y * lcd_surface->pitch + x * bpp;

#if LCD_DEPTH == 1
            *(Uint8 *)p = ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1);
#elif LCD_DEPTH == 2
            *(Uint8 *)p = ((lcd_framebuffer[y/4][x] >> (2 * (y & 3))) & 3);
#elif LCD_DEPTH == 16
#if LCD_PIXELFORMAT == RGB565SWAPPED
            unsigned bits = lcd_framebuffer[y][x];
            *(Uint32 *)p = (bits >> 8) | (bits << 8);
#else
            *(Uint32 *)p = lcd_framebuffer[y][x];
#endif
#endif
        }
    }

    SDL_UnlockSurface(lcd_surface);

    SDL_Rect src = {x_start, y_start, xmax, ymax};
    SDL_Rect dest = {UI_LCD_POSX + x_start, UI_LCD_POSY + y_start, xmax, ymax};


    SDL_BlitSurface(lcd_surface, &src, gui_surface, &dest);
    SDL_UpdateRect(gui_surface, dest.x, dest.y, dest.w, dest.h);
    SDL_Flip(gui_surface);

}

#ifdef HAVE_REMOTE_LCD

extern unsigned char lcd_remote_framebuffer[LCD_REMOTE_HEIGHT/8][LCD_REMOTE_WIDTH];

void lcd_remote_update (void)
{
    lcd_remote_update_rect(0, 0, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT);
}

void lcd_remote_update_rect(int x_start, int y_start,
                           int width, int height)
{
    int x, y;
    int xmax, ymax;

    ymax = y_start + height;
    xmax = x_start + width;

    if(xmax > LCD_REMOTE_WIDTH)
        xmax = LCD_REMOTE_WIDTH;
    if(ymax >= LCD_REMOTE_HEIGHT)
        ymax = LCD_REMOTE_HEIGHT;

    SDL_LockSurface(remote_surface);

    int bpp = remote_surface->format->BytesPerPixel;

    for (x = x_start; x < xmax; x++)
        for (y = y_start; y < ymax; y++)
        {
            Uint8 *p = (Uint8 *)remote_surface->pixels + y * remote_surface->pitch + x * bpp;

            *(Uint32 *)p = ((lcd_remote_framebuffer[y/8][x] >> (y & 7)) & 1);
        }

    SDL_UnlockSurface(remote_surface);

    SDL_Rect src = {x_start, y_start, xmax, ymax};
    SDL_Rect dest = {UI_REMOTE_POSX + x_start, UI_REMOTE_POSY + y_start, xmax, ymax};

    SDL_BlitSurface(remote_surface, &src, gui_surface, &dest);
    SDL_UpdateRect(gui_surface, dest.x, dest.y, dest.w, dest.h);
    SDL_Flip(gui_surface);

}

#endif /* HAVE_REMOTE_LCD */
#endif /* HAVE_LCD_BITMAP */

#ifdef HAVE_LCD_CHARCELLS
/* Defined in lcd-playersim.c */
extern void lcd_print_char(int x, int y);
extern bool lcd_display_redraw;
extern unsigned char hardware_buffer_lcd[11][2];
static unsigned char lcd_buffer_copy[11][2];

void lcd_update(void)
{
    int x, y;
    bool changed = false;
    SDL_Rect dest = {UI_LCD_POSX, UI_LCD_POSY, UI_LCD_WIDTH, UI_LCD_HEIGHT};

    for (y = 0; y < 2; y++)
    {
        for (x = 0; x < 11; x++)
        {
            if (lcd_display_redraw ||
                lcd_buffer_copy[x][y] != hardware_buffer_lcd[x][y])
            {
                lcd_buffer_copy[x][y] = hardware_buffer_lcd[x][y];
                lcd_print_char(x, y);
                changed = true;
            }
        }
    }
    if (changed)
    {
        SDL_BlitSurface(lcd_surface, NULL, gui_surface, &dest);
        SDL_UpdateRect(gui_surface, dest.x, dest.y, dest.w, dest.h);
        SDL_Flip(gui_surface);
    }
    lcd_display_redraw = false;
}

void drawdots(int color, struct coordinate *points, int count)
{
    int bpp = lcd_surface->format->BytesPerPixel;

    SDL_LockSurface(lcd_surface);

    while (count--)
    {
        Uint8 *p = (Uint8 *)lcd_surface->pixels + (points[count].y) * lcd_surface->pitch + (points[count].x) * bpp;

        *p = color;
    }

    SDL_UnlockSurface(lcd_surface);
}

void drawrectangles(int color, struct rectangle *points, int count)
{
    int bpp = lcd_surface->format->BytesPerPixel;

    SDL_LockSurface(lcd_surface);

    while (count--)
    {
        int x;
        int y;
        int ix;
        int iy;

        for (x = points[count].x, ix = 0; ix < points[count].width; x++, ix++)
        {
            for (y = points[count].y, iy = 0; iy < points[count].height; y++, iy++)
            {
                Uint8 *p = (Uint8 *)lcd_surface->pixels + y * lcd_surface->pitch + x * bpp;

                *p = color;
            }
        }
    }

    SDL_UnlockSurface(lcd_surface);
}
#endif /* HAVE_LCD_CHARCELLS */

#if LCD_DEPTH <= 8
/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void lcdcolors(int index, int count, SDL_Color *start, SDL_Color *end)
{
    int i;

    count--;
    for (i = 0; i <= count; i++)
    {
        lcd_palette[i+index].r = start->r
                              + (end->r - start->r) * i / count;
        lcd_palette[i+index].g = start->g
                              + (end->g - start->g) * i / count;
        lcd_palette[i+index].b = start->b
                              + (end->b - start->b) * i / count;
    }

    SDL_SetPalette(lcd_surface, SDL_LOGPAL|SDL_PHYSPAL, lcd_palette, index, count);
}
#endif

#ifdef HAVE_REMOTE_LCD
/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void lcdremotecolors(int index, int count, SDL_Color *start, SDL_Color *end)
{
    int i;

    count--;
    for (i = 0; i <= count; i++)
    {
        remote_palette[i+index].r = start->r
                              + (end->r - start->r) * i / count;
        remote_palette[i+index].g = start->g
                              + (end->g - start->g) * i / count;
        remote_palette[i+index].b = start->b
                              + (end->b - start->b) * i / count;
    }

    SDL_SetPalette(remote_surface, SDL_LOGPAL|SDL_PHYSPAL, remote_palette, index, count);
}
#endif

/* initialise simulator lcd driver */
void simlcdinit(void)
{
#if LCD_DEPTH == 16
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH, LCD_HEIGHT, 16,
                                  0, 0, 0, 0);
#else
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH, LCD_HEIGHT, 8,
                                  0, 0, 0, 0);
#endif

#if LCD_DEPTH <= 8
    lcdcolors(0, (1<<LCD_DEPTH), &lcd_color_zero, &lcd_color_max);
#endif

#ifdef HAVE_REMOTE_LCD
    remote_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT, 8,
                                  0, 0, 0, 0);

    lcdremotecolors(0, (1<<LCD_REMOTE_DEPTH), &remote_color_zero, &remote_color_max);
#endif
}

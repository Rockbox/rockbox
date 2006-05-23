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

#include "debug.h"
#include "lcd.h"
#include "misc.h"
#include <string.h>

#include "lcd-playersim.h"
#include "uisdl.h"
#include "lcd-sdl.h"

/* extern functions, needed for screendump() */
extern int sim_creat(const char *name, mode_t mode);
extern ssize_t write(int fd, const void *buf, size_t count);
extern int close(int fd);

SDL_Surface* lcd_surface;
SDL_Color lcd_color_zero = {UI_LCD_BGCOLOR, 0};
SDL_Color lcd_backlight_color_zero = {UI_LCD_BGCOLORLIGHT, 0};
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

#ifdef CONFIG_BACKLIGHT
void sim_backlight(int value)
{
    if (value > 0) {
        sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero, &lcd_color_max,
                         0, (1<<LCD_DEPTH));
    } else {
        sdl_set_gradient(lcd_surface, &lcd_color_zero, &lcd_color_max,
                         0, (1<<LCD_DEPTH));
    }
    
    lcd_update();
}
#endif

/* initialise simulator lcd driver */
void sim_lcd_init(void)
{
    lcd_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, LCD_WIDTH * display_zoom,
        LCD_HEIGHT * display_zoom, 8, 0, 0, 0, 0);

    sdl_set_gradient(lcd_surface, &lcd_backlight_color_zero, &lcd_color_max,
                     0, (1<<LCD_DEPTH));
}

#define BMP_COMPRESSION 0 /* BI_RGB */
#define BMP_NUMCOLORS (1 << LCD_DEPTH)
#define BMP_BPP 1
#define BMP_LINESIZE (((LCD_WIDTH + 31) / 32) * 4)

#define BMP_HEADERSIZE (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE   (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE  (BMP_HEADERSIZE + BMP_DATASIZE)

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff

static const unsigned char bmpheader[] =
{
    0x42, 0x4d,                 /* 'BM' */
    LE32_CONST(BMP_TOTALSIZE),  /* Total file size */
    0x00, 0x00, 0x00, 0x00,     /* Reserved */
    LE32_CONST(BMP_HEADERSIZE), /* Offset to start of pixel data */

    0x28, 0x00, 0x00, 0x00,     /* Size of (2nd) header */
    LE32_CONST(LCD_WIDTH),      /* Width in pixels */
    LE32_CONST(LCD_HEIGHT),     /* Height in pixels */
    0x01, 0x00,                 /* Number of planes (always 1) */
    LE16_CONST(BMP_BPP),        /* Bits per pixel 1/4/8/16/24 */
    LE32_CONST(BMP_COMPRESSION),/* Compression mode */
    LE32_CONST(BMP_DATASIZE),   /* Size of bitmap data */
    0xc4, 0x0e, 0x00, 0x00,     /* Horizontal resolution (pixels/meter) */
    0xc4, 0x0e, 0x00, 0x00,     /* Vertical resolution (pixels/meter) */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of used colours */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of important colours */

    0x90, 0xee, 0x90, 0x00,     /* Colour #0 */
    0x00, 0x00, 0x00, 0x00      /* Colour #1 */
};

void screen_dump(void)
{
    int fd;
    char filename[MAX_PATH];
    int x, y;
    static unsigned char line[BMP_LINESIZE];

    create_numbered_filename(filename, "", "dump_", ".bmp", 4);
    DEBUGF("screen_dump\n");

    fd = sim_creat(filename, 1 /*O_WRONLY*/);
    if (fd < 0)
        return;

    write(fd, bmpheader, sizeof(bmpheader));
    SDL_LockSurface(lcd_surface);

    /* BMP image goes bottom up */
    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
        Uint8 *src = (Uint8 *)lcd_surface->pixels 
                   + y * LCD_WIDTH * display_zoom * display_zoom;
        unsigned char *dst = line;
        unsigned dst_mask = 0x80;

        memset(line, 0, sizeof(line));
        for (x = LCD_WIDTH; x > 0; x--)
        {
            if (*src)
                *dst |= dst_mask;
            src += display_zoom;
            dst_mask >>= 1;
            if (dst_mask == 0)
            {
                dst++;
                dst_mask = 0x80;
            }
        }
        write(fd, line, sizeof(line));
    }
    SDL_UnlockSurface(lcd_surface);
    close(fd);
}

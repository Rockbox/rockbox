/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Felix Arends
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <windows.h>
#include <process.h>
#include "uisw32.h"
#include "lcd.h"
#include "lcd-playersim.h"

#if LCD_DEPTH == 16
unsigned short bitmap[LCD_HEIGHT][LCD_WIDTH]; /* the ui display */

BITMAPINFO256 bmi =
{
  {sizeof (BITMAPINFOHEADER),
   LCD_WIDTH, -LCD_HEIGHT, 1, 16,
   BI_BITFIELDS, 0, 0, 0, 3, 3,
  }, /* bitfield masks (RGB565) */
  {{0x00, 0xf8, 0, 0}, {0xe0, 0x07, 0, 0}, {0x1f, 0x00, 0, 0}}
}; /* bitmap information */
#else
unsigned char bitmap[LCD_HEIGHT][LCD_WIDTH]; /* the ui display */
RGBQUAD color_zero = {UI_LCD_BGCOLORLIGHT, 0};
RGBQUAD color_max  = {0, 0, 0, 0};

BITMAPINFO256 bmi =
{
  {sizeof (BITMAPINFOHEADER),
   LCD_WIDTH, -LCD_HEIGHT, 1, 8,
   BI_RGB, 0, 0, 0, 2, 2,
  },
  {} /* colour lookup table gets filled later */
}; /* bitmap information */
#endif

#ifdef HAVE_LCD_BITMAP

#ifdef HAVE_REMOTE_LCD
unsigned char remote_bitmap[LCD_REMOTE_HEIGHT][LCD_REMOTE_WIDTH];
RGBQUAD remote_color_zero = {UI_REMOTE_BGCOLORLIGHT, 0};
RGBQUAD remote_color_max  = {0, 0, 0, 0};

BITMAPINFO256 remote_bmi =
{
  {sizeof (BITMAPINFOHEADER),
   LCD_REMOTE_WIDTH, -LCD_REMOTE_HEIGHT, 1, 8,
   BI_RGB, 0, 0, 0, 2, 2,
  },
  {} /* colour lookup table gets filled later */
};
#endif

void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

void lcd_update_rect(int x_start, int y_start,
                     int width, int height)
{
    int x, y;
    int xmax, ymax;
    RECT r;

    ymax = y_start + height;
    xmax = x_start + width;
    
    if (hGUIWnd == NULL)
        _endthread ();

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT)
        ymax = LCD_HEIGHT;

    for (x = x_start; x < xmax; x++)
        for (y = y_start; y < ymax; y++)
        {
#if LCD_DEPTH == 1
            bitmap[y][x] = ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1);
#elif LCD_DEPTH == 2
#if LCD_PIXELFORMAT == HORIZONTAL_PACKING
            bitmap[y][x] = ((lcd_framebuffer[y][x/4] >> (2 * (x & 3))) & 3);
#else
            bitmap[y][x] = ((lcd_framebuffer[y/4][x] >> (2 * (y & 3))) & 3);
#endif
#elif LCD_DEPTH == 16
#if LCD_PIXELFORMAT == RGB565SWAPPED
            unsigned bits = lcd_framebuffer[y][x];
            bitmap[y][x] = (bits >> 8) | (bits << 8);
#else
            bitmap[y][x] = lcd_framebuffer[y][x];
#endif
#endif
        }

    /* Invalidate only the window part that actually did change */
    GetClientRect (hGUIWnd, &r);
    r.left   = (UI_LCD_POSX + (UI_LCD_WIDTH * x_start / LCD_WIDTH)) 
               * r.right / UI_WIDTH;
    r.top    = (UI_LCD_POSY + (UI_LCD_HEIGHT * y_start / LCD_HEIGHT))
               * r.bottom / UI_HEIGHT;
    r.right  = (UI_LCD_POSX + (UI_LCD_WIDTH * xmax / LCD_WIDTH))
               * r.right / UI_WIDTH;
    r.bottom = (UI_LCD_POSY + (UI_LCD_HEIGHT * ymax / LCD_HEIGHT))
               * r.bottom / UI_HEIGHT;
    InvalidateRect (hGUIWnd, &r, FALSE);
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
    RECT r;

    ymax = y_start + height;
    xmax = x_start + width;
    
    if (hGUIWnd == NULL)
        _endthread ();

    if(xmax > LCD_REMOTE_WIDTH)
        xmax = LCD_REMOTE_WIDTH;
    if(ymax >= LCD_REMOTE_HEIGHT)
        ymax = LCD_REMOTE_HEIGHT;

    for (x = x_start; x < xmax; x++)
        for (y = y_start; y < ymax; y++)
            remote_bitmap[y][x] = ((lcd_remote_framebuffer[y/8][x] >> (y & 7)) & 1);

    /* Invalidate only the window part that actually did change */
    GetClientRect (hGUIWnd, &r);
    r.left   = (UI_REMOTE_POSX + (UI_REMOTE_WIDTH * x_start / LCD_REMOTE_WIDTH))
               * r.right / UI_WIDTH;
    r.top    = (UI_REMOTE_POSY + (UI_REMOTE_HEIGHT * y_start / LCD_REMOTE_HEIGHT))
               * r.bottom / UI_HEIGHT;
    r.right  = (UI_REMOTE_POSX + (UI_REMOTE_WIDTH * xmax / LCD_REMOTE_WIDTH))
               * r.right / UI_WIDTH;
    r.bottom = (UI_REMOTE_POSY + (UI_REMOTE_HEIGHT * ymax / LCD_REMOTE_HEIGHT))
               * r.bottom / UI_HEIGHT;
    InvalidateRect (hGUIWnd, &r, FALSE);
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
    RECT r;

    if (hGUIWnd == NULL)
        _endthread ();

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
        /* Invalidate only the window part that actually did change */
        GetClientRect (hGUIWnd, &r);
        r.left = UI_LCD_POSX * r.right / UI_WIDTH;
        r.top = UI_LCD_POSY * r.bottom / UI_HEIGHT;
        r.right = (UI_LCD_POSX + UI_LCD_WIDTH) * r.right / UI_WIDTH;
        r.bottom = (UI_LCD_POSY + UI_LCD_HEIGHT) * r.bottom / UI_HEIGHT;
        InvalidateRect (hGUIWnd, &r, FALSE);
    }
    lcd_display_redraw = false;
}

void drawdots(int color, struct coordinate *points, int count)
{
    while (count--)
    {
        bitmap[points[count].y][points[count].x] = color;
    }
}

void drawrectangles(int color, struct rectangle *points, int count)
{
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
                bitmap[y][x] = color;
            }
        }
    }
}
#endif /* HAVE_LCD_CHARCELLS */

#if 0
/* set backlight state of lcd */
void lcd_backlight (bool on)
{
    if (on)
        color_zero = {UI_LCD_BGCOLORLIGHT, 0};
    else
        color_zero = {UI_LCD_BGCOLOR, 0};

    lcdcolors(0, (1<<LCD_DEPTH), &color_zero, &color_max);
    InvalidateRect (hGUIWnd, NULL, FALSE);
}
#endif

#if LCD_DEPTH <= 8
/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void lcdcolors(int index, int count, RGBQUAD *start, RGBQUAD *end)
{
    int i;

    bmi.bmiHeader.biClrUsed = index + count;
    bmi.bmiHeader.biClrImportant = index + count;

    count--;
    for (i = 0; i <= count; i++)
    {
        bmi.bmiColors[i+index].rgbRed = start->rgbRed
                              + (end->rgbRed - start->rgbRed) * i / count;
        bmi.bmiColors[i+index].rgbGreen = start->rgbGreen
                              + (end->rgbGreen - start->rgbGreen) * i / count;
        bmi.bmiColors[i+index].rgbBlue = start->rgbBlue
                              + (end->rgbBlue - start->rgbBlue) * i / count;
    }
}
#endif

#ifdef HAVE_REMOTE_LCD
/* set a range of bitmap indices to a gradient from startcolour to endcolour */
void lcdremotecolors(int index, int count, RGBQUAD *start, RGBQUAD *end)
{
    int i;
    
    remote_bmi.bmiHeader.biClrUsed = index + count;
    remote_bmi.bmiHeader.biClrImportant = index + count;

    count--;
    for (i = 0; i <= count; i++)
    {
        remote_bmi.bmiColors[i+index].rgbRed = start->rgbRed
                              + (end->rgbRed - start->rgbRed) * i / count;
        remote_bmi.bmiColors[i+index].rgbGreen = start->rgbGreen
                              + (end->rgbGreen - start->rgbGreen) * i / count;
        remote_bmi.bmiColors[i+index].rgbBlue = start->rgbBlue
                              + (end->rgbBlue - start->rgbBlue) * i / count;
    }
}
#endif

/* initialise simulator lcd driver */
void simlcdinit(void)
{
#if LCD_DEPTH <= 8
    lcdcolors(0, (1<<LCD_DEPTH), &color_zero, &color_max);
#endif
#ifdef HAVE_REMOTE_LCD
    lcdremotecolors(0, (1<<LCD_REMOTE_DEPTH), &remote_color_zero, &remote_color_max);
#endif
}


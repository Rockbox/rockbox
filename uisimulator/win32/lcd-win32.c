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

unsigned char lcd_framebuffer[LCD_HEIGHT/8][LCD_WIDTH]; /* the display */
char bitmap[LCD_HEIGHT][LCD_WIDTH]; /* the ui display */

BITMAPINFO2 bmi =
{
  {sizeof (BITMAPINFOHEADER),
   LCD_WIDTH, -LCD_HEIGHT, 1, 8,
   BI_RGB, 0, 0, 0, 2, 2,
  },
  {
    //{UI_LCD_BGCOLOR, 0}, /* green background color */
    {UI_LCD_BGCOLORLIGHT, 0}, /* green background color */
    {UI_LCD_BLACK, 0}    /* black color */
  }
  
}; /* bitmap information */

#ifdef HAVE_LCD_CHARCELLS
/* Defined in lcd-playersim.c */
extern void lcd_print_char(int x, int y);
extern bool lcd_display_redraw;
extern unsigned char hardware_buffer_lcd[11][2];
static unsigned char lcd_buffer_copy[11][2];
#endif 

/* lcd_update()
   update lcd */
void lcd_update()
{
    int x, y;
    RECT r;

    if (hGUIWnd == NULL)
        _endthread ();

#ifdef HAVE_LCD_CHARCELLS
    for (y = 0; y < 2; y++)
    {
        for (x = 0; x < 11; x++)
        {
            if (lcd_display_redraw ||
                lcd_buffer_copy[x][y] != hardware_buffer_lcd[x][y])
            {
                lcd_buffer_copy[x][y] = hardware_buffer_lcd[x][y];
                lcd_print_char(x, y);
            }
        }
    }

    lcd_display_redraw = false;
#endif

    for (x = 0; x < LCD_WIDTH; x++)
        for (y = 0; y < LCD_HEIGHT; y++)
            bitmap[y][x] = ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1);

    /* Invalidate only the window part that actually did change */
    GetClientRect (hGUIWnd, &r);
    r.left = UI_LCD_POSX * r.right / UI_WIDTH;
    r.top = UI_LCD_POSY * r.bottom / UI_HEIGHT;
    r.right = (UI_LCD_POSX + UI_LCD_WIDTH) * r.right / UI_WIDTH;
    r.bottom = (UI_LCD_POSY + UI_LCD_HEIGHT) * r.bottom / UI_HEIGHT;
    InvalidateRect (hGUIWnd, &r, FALSE);
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
            bitmap[y][x] = ((lcd_framebuffer[y/8][x] >> (y & 7)) & 1);

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

void lcd_remote_update(void)
{
    
}

void lcd_remote_update_rect(int x_start, int y_start,
                            int width, int height)
{
    (void)x_start;
    (void)y_start;
    (void)width;
    (void)height;
}

/* lcd_backlight()
   set backlight state of lcd */
void lcd_backlight (bool on)
{
    if (on)
    {
        RGBQUAD blon = {UI_LCD_BGCOLORLIGHT, 0};
        bmi.bmiColors[0] = blon;
    }
    else
    {
        RGBQUAD blon = {UI_LCD_BGCOLOR, 0};
        bmi.bmiColors[0] = blon;
    }

    InvalidateRect (hGUIWnd, NULL, FALSE);
}

void drawdots(int color, struct coordinate *points, int count)
{
    while (count--)
    {
        if (color)
        {
            DRAW_PIXEL(points[count].x, points[count].y);
        }
        else
        {
            CLEAR_PIXEL(points[count].x, points[count].y);
        }
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
            for (y = points[count].y, iy = 0; iy < points[count].width; y++, iy++)
            {
                if (color)
                {
                    DRAW_PIXEL(x, y);
                }
                else
                {
                    CLEAR_PIXEL(x, y);
                }
            }
        }
    }
}

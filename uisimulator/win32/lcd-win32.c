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

unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8]; /* the display */
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

void lcd_set_invert_display(bool invert)
{
    (void)invert;
}

/* lcd_update()
   update lcd */
void lcd_update()
{
    int x, y;
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
            bitmap[y][x] = ((lcd_framebuffer[x][y/8] >> (y & 7)) & 1);

    InvalidateRect (hGUIWnd, NULL, FALSE);

    /* natural sleep :) Bagder: why is this here? */
    //Sleep (50);
}

void lcd_update_rect(int x_start, int y_start,
                     int width, int height)
{
    int x, y;
    int xmax, ymax;

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
            bitmap[y][x] = ((lcd_framebuffer[x][y/8] >> (y & 7)) & 1);

    /* Bagder: If I only knew how, I would make this call only invalidate
       the actual rectangle we want updated here, this NULL thing here will
       make the full rectangle updated! */
    InvalidateRect (hGUIWnd, NULL, FALSE);
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

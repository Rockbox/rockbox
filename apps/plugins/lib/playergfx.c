/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Bitmap graphics on player LCD!
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_CHARCELLS /* Player only :) */
#include "playergfx.h"

/* Global variables */
static struct plugin_api *pgfx_rb = NULL; /* global api struct pointer */
static int char_width;
static int char_height;
static int pixel_height;
static unsigned char gfx_chars[8];
static unsigned char gfx_buffer[56];


bool pgfx_init(struct plugin_api* newrb, int cwidth, int cheight)
{
    int i;

    if (((unsigned) cwidth * (unsigned) cheight) > 8 || (unsigned) cheight > 2)
        return false;

    pgfx_rb = newrb;
    char_width = cwidth;
    char_height = cheight;
    pixel_height = 7 * char_height;
    
    for (i = 0; i < cwidth * cheight; i++)
    {
        if ((gfx_chars[i] = pgfx_rb->lcd_get_locked_pattern()) == 0)
        {
            pgfx_release();
            return false;
        }
    }

    return true;
}

void pgfx_release(void)
{
    int i;
    
    for (i = 0; i < 8; i++)
        if (gfx_chars[i])
            pgfx_rb->lcd_unlock_pattern(gfx_chars[i]);
}

void pgfx_display(int cx, int cy)
{
    int i, j;
    
    for (i = 0; i < char_width * char_height; i++)
        pgfx_rb->lcd_define_pattern(gfx_chars[i], gfx_buffer + 7 * i);
        

    for (i = 0; i < char_width; i++)
        for (j = 0; j < char_height; j++)
            pgfx_rb->lcd_putc(cx + i, cy + j, gfx_chars[char_height * i + j]);
}

void pgfx_clear_display(void)
{
    pgfx_rb->memset(gfx_buffer, 0, char_width * pixel_height);
}

void pgfx_drawpixel(int x, int y)
{
    gfx_buffer[pixel_height * (x/5) + y] |= 0x10 >> (x%5);
}

void pgfx_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if(x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if(y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i = 0; i < numpixels; i++)
    {
        pgfx_drawpixel(x, y);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

#endif /* HAVE_LCD_CHARCELLS */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "hwcompat.h"

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"

#include "font-player.h"

/*** definitions ***/

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define ICON_HEIGHT 10

unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8];

static int double_height=1;

void lcd_print_icon(int x, int icon_line, bool enable, char **icon)
{
  int xpos = x;
  int ypos = icon_line*(ICON_HEIGHT+CHAR_HEIGHT*2);
  int row=0, col;
  
  while (icon[row]) {
    col=0;
    while (icon[row][col]) {
      lcd_framebuffer[xpos+col][(ypos+row)/8] &= ~(1<<((row+ypos)&7));
      if (enable) {
	if (icon[row][col]=='*') {
	  lcd_framebuffer[xpos+col][(ypos+row)/8] |= 1<<((row+ypos)&7);
	}
      }
      col++;
    }
    row++;
  }
  lcd_update();
}

void lcd_print_char(int x, int y, unsigned char ch)
{
  int xpos = x * CHAR_WIDTH;
  int ypos = y * CHAR_HEIGHT;
  int col, row;
  int y2;
  //  double_height=2;
  if (double_height == 2 && y == 1)
    return; /* Second row can't be printed in double height. ??*/

  xpos*=2;

  /*  printf("(%d,%d)='%d'\n", x, y, ch);*/
  for (col=0; col<5; col++) {
    for (row=0; row<7; row++) {
      for (y2=0; y2<double_height*2; y2++) {
	int y;
	unsigned char bitval;

	if (double_height*row >=7)
	  y2+=double_height;
	y=y2+double_height*2*row+2*ypos+ICON_HEIGHT;

	bitval=3<<((y&6));
	if (font_player[ch][col]&(1<<row)) {
	  lcd_framebuffer[xpos+col*2][y/8] |= bitval;
	  
	} else {
	  lcd_framebuffer[xpos+col*2][y/8] &= ~bitval;
	}
	lcd_framebuffer[xpos+col*2+1][y/8] = 
	  lcd_framebuffer[xpos+col*2][y/8];
      }
    }
  }
}


/*
 * Draw a rectangle with upper left corner at (x, y)
 * and size (nx, ny)
 */
void lcd_drawrect (int x, int y, int nx, int ny)
{
    int i;

    if (x > LCD_WIDTH)
        return;
    if (y > LCD_HEIGHT)
        return;

    if (x + nx > LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (y + ny > LCD_HEIGHT)
        ny = LCD_HEIGHT - y;

    /* vertical lines */
    for (i = 0; i < ny; i++) {
        DRAW_PIXEL(x, (y + i));
        DRAW_PIXEL((x + nx - 1), (y + i));
    }

    /* horizontal lines */
    for (i = 0; i < nx; i++) {
        DRAW_PIXEL((x + i),y);
        DRAW_PIXEL((x + i),(y + ny - 1));
    }
}

/* Invert a rectangular area at (x, y), size (nx, ny) */
void lcd_invertrect (int x, int y, int nx, int ny)
{
    int i, j;

    if (x > LCD_WIDTH)
        return;
    if (y > LCD_HEIGHT)
        return;

    if (x + nx > LCD_WIDTH)
        nx = LCD_WIDTH - x;
    if (y + ny > LCD_HEIGHT)
        ny = LCD_HEIGHT - y;

    for (i = 0; i < nx; i++)
        for (j = 0; j < ny; j++)
            INVERT_PIXEL((x + i), (y + j));
}

void lcd_drawline( int x1, int y1, int x2, int y2 )
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);

    if(deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
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

    for(i=0; i<numpixels; i++)
    {
        DRAW_PIXEL(x,y);

        if(d < 0)
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

void lcd_clearline( int x1, int y1, int x2, int y2 )
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);

    if(deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
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

    for(i=0; i<numpixels; i++)
    {
        CLEAR_PIXEL(x,y);

        if(d < 0)
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

/*
 * Set a single pixel
 */
void lcd_drawpixel(int x, int y)
{
    DRAW_PIXEL(x,y);
}

/*
 * Clear a single pixel
 */
void lcd_clearpixel(int x, int y)
{
    CLEAR_PIXEL(x,y);
}

/*
 * Invert a single pixel
 */
void lcd_invertpixel(int x, int y)
{
    INVERT_PIXEL(x,y);
}

void lcd_clear_display(void)
{
    memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
}

void lcd_puts(int x, int y, unsigned char *str)
{
    int i;
    for (i=0; *str && x<11; i++)
        lcd_print_char(x++, y, *str++);
    for (; x<11; x++)
        lcd_print_char(x, y, ' ');
    lcd_update();
}

void lcd_double_height(bool on)
{
    double_height = 1;
    if (on)
        double_height = 2;
}

void lcd_define_pattern(int which, char *pattern, int length)
{
    int i, j;
    int pat = which / 8;
    char icon[8];
    memset(icon, 0, sizeof icon);

    DEBUGF("Defining pattern %d\n", pat);
    for (j = 0; j <= 5; j++) {
        for (i = 0; i < length; i++) {
            if ((pattern[i])&(1<<(j)))
                icon[5-j] |= (1<<(i));
        }
    }
    for (i = 1; i <= 5; i++)
    {
        font_player[pat][i-1] = icon[i];
    }
}

extern void lcd_puts(int x, int y, unsigned char *str);

void lcd_putc(int x, int y, unsigned char ch)
{
    lcd_print_char(x, y, ch);
    lcd_update();
}

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

#include "lcd.h"

void lcd_data (int data)
{
  lcd_byte (data,1);
}

void lcd_instruction (int instruction)
{
  lcd_byte (instruction,0);
}

void lcd_zero (int length)
{
  length *= 8;
  while (--length >= 0)
    lcd_data (0);
}

void lcd_fill (int data,int length)
{
  length *= 8;
  while (--length >= 0)
    lcd_data (data);
}

void lcd_copy (void *data,int count)
{
  while (--count >= 0)
    lcd_data (*((char *)data)++);
}


#ifdef HAVE_LCD_CHARCELLS
#  ifndef JBP_OLD

static char const lcd_ascii[] =
{
/*****************************************************************************************/
/*         x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF */
/*    ************************************************************************************/
/* 0x */ 0x00,0x01,0x02,0x03,0x00,0x00,0x00,0x00,0x04,0x05,0x00,0x00,0x00,0x00,0x00,0x00,
/* 1x */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* 2x */ 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
/* 3x */ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
/* 4x */ 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
/* 5x */ 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
/* 6x */ 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
/* 7x */ 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x20,0x20,0x20,0x20,0x20,
/* 8x */ 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
/* 9x */ 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
/* Ax */ 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
/* Bx */ 0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
/* Cx */ 0x41,0x41,0x41,0x41,0x41,0x41,0x20,0x43,0x45,0x45,0x45,0x45,0x49,0x49,0x49,0x49,
/* Dx */ 0x44,0x4E,0x4F,0x4F,0x4F,0x4F,0x4F,0x20,0x20,0x55,0x55,0x55,0x55,0x59,0x20,0x20,
/* Ex */ 0x61,0x61,0x61,0x61,0x61,0x61,0x20,0x63,0x65,0x65,0x65,0x65,0x69,0x69,0x69,0x69,
/* Fx */ 0x64,0x6E,0x6F,0x6F,0x6F,0x6F,0x6F,0x20,0x20,0x75,0x75,0x75,0x75,0x79,0x79,0x79
/******/
  };

#  else

static char const lcd_ascii[] =
  {
/*****************************************************************************************/
/*         x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF */
/*    ************************************************************************************/
/* 0x */ 0x00,0x01,0x02,0x03,0x00,0x00,0x00,0x00,0x85,0x89,0x00,0x00,0x00,0x00,0x00,0x00,
/* 1x */ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/* 2x */ 0x24,0x25,0x26,0x37,0x06,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,
/* 3x */ 0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,
/* 4x */ 0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,
/* 5x */ 0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0xA9,0x33,0xCE,0x00,0x15,
/* 6x */ 0x00,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,
/* 7x */ 0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x24,0x24,0x24,0x24,0x24,
/* 8x */ 0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
/* 9x */ 0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
/* Ax */ 0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
/* Bx */ 0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
/* Cx */ 0x45,0x45,0x45,0x45,0x45,0x45,0x24,0x47,0x49,0x49,0x49,0x49,0x4D,0x4D,0x4D,0x4D,
/* Dx */ 0x48,0x52,0x53,0x53,0x53,0x53,0x53,0x24,0x24,0x59,0x59,0x59,0x59,0x5D,0x24,0x24,
/* Ex */ 0x65,0x65,0x65,0x65,0x65,0x65,0x24,0x67,0x69,0x69,0x69,0x69,0x6D,0x6D,0x6D,0x6D,
/* Fx */ 0x73,0x72,0x73,0x73,0x73,0x73,0x73,0x24,0x24,0x79,0x79,0x79,0x79,0x7D,0x24,0x7D
/******/
  };

#  endif

void lcd_puts (char const *string)
{
  while (*string)
    lcd_data (LCD_ASCII(*string++));
}

void lcd_putns (char const *string,int n)
{
  while (n--)
    lcd_data (LCD_ASCII(*string++));
}

void lcd_putc (int character)
{
  lcd_data (LCD_ASCII(character));
}

void lcd_pattern (int which,char const *pattern,int count)
{
  lcd_instruction (LCD_PRAM|which);
  lcd_copy ((void *)pattern,count);
}

void lcd_puthex (unsigned int value,int digits)
{
  switch (digits) {
  case 8:
    lcd_puthex (value >> 16,4);
  case 4:
    lcd_puthex (value >> 8,2);
  case 2:
    lcd_puthex (value >> 4,1);
  case 1:
    value &= 15;
    lcd_putc (value+((value < 10) ? '0' : ('A'-10)));
  }
}


/* HAVE_LCD_CHARCELLS */
#elif defined(HAVE_LCD_BITMAP)

/*
 * All bitmaps have this format:
 * Bits within a byte are arranged veritcally, LSB at top.
 * Bytes are stored in column-major format, with byte 0 at top left,
 * byte 1 is 2nd from top, etc.  Bytes following left-most column
 * starts 2nd left column, etc.
 *
 * Note: The HW takes bitmap bytes in row-major order.
 *
 * Memory copy of display bitmap
 */
unsigned char display[DISP_X][DISP_Y/8];

/*
 * ASCII character generation tables
 *
 * This contains only the printable characters (0x20-0x7f).
 * Each element in this table is a character pattern bitmap.
 */
#define	ASCII_MIN			0x20	/* First char in table */
#define	ASCII_MAX			0x7f	/* Last char in table */

extern const unsigned char char_gen_6x8[][5][1];
extern const unsigned char char_gen_8x12[][7][2];
extern const unsigned char char_gen_12x16[][11][2];


/* All zeros and ones bitmaps for area filling */
static const unsigned char zeros[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00 };
static const unsigned char ones[]  = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                       0xff, 0xff };

static int lcd_y;    /* Current pixel row */
static int lcd_x;    /* Current pixel column */
static int lcd_size; /* Current font width */

#ifndef SIMULATOR

/*
 * Initialize LCD
 */
void lcd_init (void)
{
    /* Initialize PB0-3 as output pins */
    PBCR2 &= 0xff00; /* MD = 00 */
    PBIOR |= 0x000f; /* IOR = 1 */

    /* Initialize LCD */
    lcd_write (TRUE, LCD_CNTL_RESET);
    lcd_write (TRUE, LCD_CNTL_POWER);
    lcd_write (TRUE, LCD_CNTL_SEGREMAP);
    lcd_write (TRUE, LCD_CNTL_OUTSCAN);
    lcd_write (TRUE, LCD_CNTL_CONTRAST);
    lcd_write (TRUE, 0x30); /* Contrast parameter */
    lcd_write (TRUE, LCD_CNTL_DISPON);

    lcd_clear_display();
    lcd_update();
}

/*
 * Update the display.
 * This must be called after all other LCD funtions that change the display.
 */
void lcd_update (void)
{
  int x, y;

  /* Copy display bitmap to hardware */
  for (y = 0; y < DISP_Y/8; y++)
  {
    lcd_write (TRUE, LCD_CNTL_PAGE | (y & 0xf));
    lcd_write (TRUE, LCD_CNTL_HIGHCOL);
    lcd_write (TRUE, LCD_CNTL_LOWCOL);

    for (x = 0; x < DISP_X; x++)
      lcd_write (FALSE, display[x][y]);
  }
}

static void lcd_write (BOOL command, int value)
{
  int bit;

  /* Enable chip select, set DC if data */
  PBDR &= ~(PBDR_LCD_CS1|PBDR_LCD_DC);
  if (!command)
    PBDR |= PBDR_LCD_DC;

  /* Send each bit, starting with MSB */
  for (bit = 0x80; bit > 0; bit >>= 1)
  {
    PBDR &= ~(PBDR_LCD_SDA|PBDR_LCD_SCK);
    if (value & bit)
      PBDR |= PBDR_LCD_SDA;
    PBDR |= PBDR_LCD_SCK;
  }
  
  /* Disable chip select */
  PBDR |= PBDR_LCD_CS1;
}

#endif /* SIMULATOR */

/*
 * Clear the display
 */
void lcd_clear_display (void)
{
	lcd_position (0, 0, 8);
	memset (display, 0, sizeof display);
}

/*
 * Set current x,y position and font size
 */
void lcd_position (int x, int y, int size)
{
	if (x >= 0 && x < DISP_X && y >= 0 && y < DISP_Y)
	{
		lcd_x = x;
		lcd_y = y;
	}

	lcd_size = size;
}

/*
 * Display a string at current position and size
 */
void lcd_string (const char *str)
{
	int x = lcd_x;
	int nx = lcd_size;
	int ny, ch;
	const unsigned char *src;

	if (nx == 12)
		ny = 16;
	else if (nx == 8)
		ny = 12;
	else
	{
		nx = 6;
		ny = 8;
	}

	while ((ch = *str++) != '\0')
	{
		if (ch == '\n' || lcd_x + nx > DISP_X)
		{
                  /* Wrap to next line */
                  lcd_x = x;
                  lcd_y += ny;
		}

		if (lcd_y + ny > DISP_Y)
			return;

		/* Limit to char generation table */
		if (ch >= ASCII_MIN && ch <= ASCII_MAX)
		{
			if (nx == 12)
				src = char_gen_12x16[ch-ASCII_MIN][0];
			else if (nx == 8)
				src = char_gen_8x12[ch-ASCII_MIN][0];
			else
				src = char_gen_6x8[ch-ASCII_MIN][0];

			lcd_bitmap (src, lcd_x, lcd_y, nx-1, ny, TRUE);
			lcd_bitmap (zeros, lcd_x+nx-1, lcd_y, 1, ny, TRUE);

			lcd_x += nx;
		}
	}
}

/*
 * Display a bitmap at (x, y), size (nx, ny)
 * clear is TRUE to clear destination area first
 */
void lcd_bitmap (const unsigned char *src, int x, int y, int nx, int ny,
                 bool clear)
{
	unsigned char *dst;
	unsigned char *dst2 = &display[x][y/8];
	unsigned int data, mask, mask2, mask3, mask4;
	int shift = y & 7;

	ny += shift;

	/* Calculate bit masks */
	mask4 = ~(0xfe << ((ny-1) & 7));
	if (clear)
	{
		mask = ~(0xff << shift);
		mask2 = 0;
		mask3 = ~mask4;
		if (ny <= 8)
			mask3 |= mask;
	}
	else
		mask = mask2 = mask3 = 0xff;

	/* Loop for each column */
	for (x = 0; x < nx; x++)
	{
		dst = dst2;
		dst2 += DISP_Y/8;
		data = 0;
		y = 0;

		if (ny > 8)
		{
                  /* First partial row */
                  data = *src++ << shift;
                  *dst = (*dst & mask) ^ data;
                  data >>= 8;
                  dst++;

                  /* Intermediate rows */
                  for (y = 8; y < ny-8; y += 8)
                  {
                    data |= *src++ << shift;
                    *dst = (*dst & mask2) ^ data;
                    data >>= 8;
                    dst++;
                  }
		}

		/* Last partial row */
		if (y + shift < ny)
                  data |= *src++ << shift;
		*dst = (*dst & mask3) ^ (data & mask4);
	}
}

/*
 * Clear a rectangular area at (x, y), size (nx, ny)
 */
void lcd_clearrect (int x, int y, int nx, int ny)
{
  int i;
  for (i = 0; i < nx; i++)
    lcd_bitmap (zeros, x+i, y, 1, ny, TRUE);
}

/*
 * Fill a rectangular area at (x, y), size (nx, ny)
 */
void lcd_fillrect (int x, int y, int nx, int ny)
{
  int i;
  for (i = 0; i < nx; i++)
    lcd_bitmap (ones, x+i, y, 1, ny, TRUE);
}

/* Invert a rectangular area at (x, y), size (nx, ny) */
void lcd_invertrect (int x, int y, int nx, int ny)
{
  int i;
  for (i = 0; i < nx; i++)
    lcd_bitmap (ones, x+i, y, 1, ny, FALSE);
}

#define DRAW_PIXEL(x,y) display[x][y/8] |= (1<<(y%7))

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

#else
/* no LCD defined, no code to use */
#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "rockbox-mode.el")
 * end:
 */

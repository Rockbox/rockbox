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
#include "lcd.h"
#include <string.h>
#include <stdlib.h>

/*** definitions ***/

#define LCDR (PBDR+1)

/* PA14 : /LCD-BL --- backlight */
#define LCD_BL 6

#ifdef HAVE_LCD_CHARCELLS

#define LCD_DS  1 // PB0 = 1 --- 0001 ---  LCD-DS
#define LCD_CS  2 // PB1 = 1 --- 0010 --- /LCD-CS
#define LCD_SD  4 // PB2 = 1 --- 0100 ---  LCD-SD
#define LCD_SC  8 // PB3 = 1 --- 1000 ---  LCD-SC
#ifdef HAVE_NEW_CHARCELL_LCD
#  define LCD_CONTRAST_SET ((char)0x50)
#  define LCD_CRAM         ((char)0x80) /* Characters */
#  define LCD_PRAM         ((char)0xC0) /*  Patterns  */
#  define LCD_IRAM         ((char)0x40) /*    Icons   */
#else
#  define LCD_CONTRAST_SET ((char)0xA8)
#  define LCD_CRAM         ((char)0xB0) /* Characters */
#  define LCD_PRAM         ((char)0x80) /*  Patterns  */
#  define LCD_IRAM         ((char)0xE0) /*    Icons   */
#endif
#define LCD_ASCII(c)       (lcd_ascii[(c)&255])
#define LCD_CURSOR(x,y)    ((char)(LCD_CRAM+((y)*16+(x))))
#define LCD_ICON(i)        ((char)(LCD_IRAM+i))          

#elif HAVE_LCD_BITMAP

#define LCD_SD  1 // PB0 = 1 --- 0001
#define LCD_SC  2 // PB1 = 1 --- 0010
#define LCD_RS  4 // PB2 = 1 --- 0100
#define LCD_CS  8 // PB3 = 1 --- 1000
#define LCD_DS LCD_RS

#define LCD_SET_LOWER_COLUMN_ADDRESS              ((char)0x00)
#define LCD_SET_HIGHER_COLUMN_ADDRESS             ((char)0x10)
#define LCD_SET_INTERNAL_REGULATOR_RESISTOR_RATIO ((char)0x20)
#define LCD_SET_POWER_CONTROL_REGISTER            ((char)0x28)
#define LCD_SET_DISPLAY_START_LINE                ((char)0x40)
#define LCD_SET_CONTRAST_CONTROL_REGISTER         ((char)0x81)
#define LCD_SET_SEGMENT_REMAP                     ((char)0xA0)
#define LCD_SET_LCD_BIAS                          ((char)0xA2)
#define LCD_SET_ENTIRE_DISPLAY_OFF                ((char)0xA4)
#define LCD_SET_ENTIRE_DISPLAY_ON                 ((char)0xA5)
#define LCD_SET_NORMAL_DISPLAY                    ((char)0xA6)
#define LCD_SET_REVERSE_DISPLAY                   ((char)0xA7)
#define LCD_SET_INDICATOR_OFF                     ((char)0xAC)
#define LCD_SET_INDICATOR_ON                      ((char)0xAD)
#define LCD_SET_DISPLAY_OFF                       ((char)0xAE)
#define LCD_SET_DISPLAY_ON                        ((char)0xAF)
#define LCD_SET_PAGE_ADDRESS                      ((char)0xB0)
#define LCD_SET_COM_OUTPUT_SCAN_DIRECTION         ((char)0xC0)
#define LCD_SET_DISPLAY_OFFSET                    ((char)0xD3)
#define LCD_SET_READ_MODIFY_WRITE_MODE            ((char)0xE0)
#define LCD_SOFTWARE_RESET                        ((char)0xE2)
#define LCD_NOP                                   ((char)0xE3)
#define LCD_SET_END_OF_READ_MODIFY_WRITE_MODE     ((char)0xEE)

/* LCD command codes */
#define LCD_CNTL_RESET          0xe2    // Software reset
#define LCD_CNTL_POWER          0x2f    // Power control
#define LCD_CNTL_CONTRAST       0x81    // Contrast
#define LCD_CNTL_OUTSCAN        0xc8    // Output scan direction
#define LCD_CNTL_SEGREMAP       0xa1    // Segment remap
#define LCD_CNTL_DISPON         0xaf    // Display on

#define LCD_CNTL_PAGE           0xb0    // Page address
#define LCD_CNTL_HIGHCOL        0x10    // Upper column address
#define LCD_CNTL_LOWCOL         0x00    // Lower column address


#endif /* CHARCELL or BITMAP */


/*** generic code ***/

#ifndef SIMULATOR
/*
 * About /CS,DS,SC,SD
 * ------------------
 *
 * LCD on JBP and JBR uses a SPI protocol to receive orders (SDA and SCK lines)
 *
 * - /CS -> Chip Selection line :
 *            0 : LCD chipset is activated.
 * -  DS -> Data Selection line, latched at the rising edge
 *          of the 8th serial clock (*) :
 *            0 : instruction register,
 *            1 : data register; 
 * -  SC -> Serial Clock line (SDA).
 * -  SD -> Serial Data line (SCK), latched at the rising edge
 *          of each serial clock (*).  
 *
 *    _                                                          _
 * /CS \                                                        /
 *      \______________________________________________________/
 *    _____  ____  ____  ____  ____  ____  ____  ____  ____  _____ 
 *  SD     \/ D7 \/ D6 \/ D5 \/ D4 \/ D3 \/ D2 \/ D1 \/ D0 \/
 *    _____/\____/\____/\____/\____/\____/\____/\____/\____/\_____
 *
 *    _____     _     _     _     _     _     _     _     ________ 
 *  SC     \   * \   * \   * \   * \   * \   * \   * \   *
 *          \_/   \_/   \_/   \_/   \_/   \_/   \_/   \_/
 *    _  _________________________________________________________ 
 *  DS \/                                                  
 *    _/\_________________________________________________________
 *
 */

/*
 * The only way to do logical operations in an atomic way
 * on SH1 is using :
 *
 *   or.b/and.b/tst.b/xor.b #imm,@(r0,gbr)
 *
 * but GCC doesn't generate them at all so some assembly
 * codes are needed here.
 *
 * The Global Base Register gbr is expected to be zero
 * and r0 is the address of one register in the on-chip
 * peripheral module.
 *
 */ 



static void lcd_write(bool command, int byte)

#ifdef ASM_IMPLEMENTATION
{
    asm("and.b %0, @(r0,gbr)"
         :
         : /* %0 */ "I"(~(LCD_CS|LCD_DS|LCD_SD|LCD_SC)),
           /* %1 */ "z"(LCDR));

    if (command)
        asm ("shll8 %0\n"
             "0:      \n\t"
             "and.b %2,@(r0,gbr)\n\t"
             "shll  %0\n\t"  
             "bf    1f\n\t"
             "or.b  %3,@(r0,gbr)\n"
             "1:      \n\t"
             "or.b  %4,@(r0,gbr)\n"
             "add   #-1,%1\n\t"
             "cmp/pl %1\n\t"
             "bt    0b"
             :
             : /* %0 */ "r"(((unsigned)byte)<<16),
             /* %1 */ "r"(8),
             /* %2 */ "I"(~(LCD_SC|LCD_SD)),
             /* %3 */ "I"(LCD_SD),
             /* %4 */ "I"(LCD_SC|LCD_DS),
             /* %5 */ "z"(LCDR));
    else
        asm ("shll8  %0\n"
             "0:       \n\t"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "add    #-1, %1\n\t"
             "cmp/pl %1\n\t"
             "bt     0b"
             :
             : /* %0 */ "r"(((unsigned)byte)<<16),
             /* %1 */ "r"(8),
             /* %2 */ "I"(~(LCD_SC|LCD_DS|LCD_SD)),
             /* %3 */ "I"(LCD_SD),
             /* %4 */ "I"(LCD_SC),
             /* %5 */ "z"(LCDR));

    asm("or.b %0, @(r0,gbr)"
         :
         : /* %0 */ "I"(LCD_CS|LCD_DS|LCD_SD|LCD_SC),
           /* %1 */ "z"(LCDR));
}
#else
{
    int i;
    char on,off;

    PBDR &= ~LCD_CS; /* enable lcd chip select */

    if ( command ) {
	on=~(LCD_SD|LCD_SC|LCD_DS);
	off=LCD_SC;
    }
    else {
	on=~(LCD_SD|LCD_SC);
	off=LCD_SC|LCD_DS;
    }

    /* clock out each bit, MSB first */
    for (i=0x80;i;i>>=1)
    {
	PBDR &= on;
	if (i & byte)
	    PBDR |= LCD_SD;
	PBDR |= off;
    }

    PBDR |= LCD_CS; /* disable lcd chip select */
}
#endif /* ASM_IMPLEMENTATION */

/*** BACKLIGHT ***/

void lcd_backlight(bool on)
{
    if ( on )
	PAIOR |= LCD_BL;
    else
	PAIOR &= ~LCD_BL;
}

#endif /* SIMULATOR */



/*** model specific code */

#ifdef HAVE_LCD_CHARCELLS

#ifdef HAVE_NEW_CHARCELL_LCD

static const unsigned char lcd_ascii[] = {
   0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
   0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
   0x10,0x11,0x05,0x13,0x14,0x15,0x16,0x17,
   0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
   0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
   0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
   0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
   0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
   0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
   0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
   0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
   0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
   0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
   0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
   0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
   0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
   0x41,0x41,0x41,0x41,0x41,0x41,0x20,0x43,
   0x45,0x45,0x45,0x45,0x49,0x49,0x49,0x49,
   0x44,0x4e,0x4f,0x4f,0x4f,0x4f,0x4f,0x20,
   0x20,0x55,0x55,0x55,0x55,0x59,0x20,0x20,
   0x61,0x61,0x61,0x61,0x61,0x61,0x20,0x63,
   0x65,0x65,0x65,0x65,0x69,0x69,0x69,0x69,
   0x6f,0x6e,0x6f,0x6f,0x6f,0x6f,0x6f,0x20,
   0x20,0x75,0x75,0x75,0x75,0x79,0x20,0x79
};

#else

static const unsigned char lcd_ascii[] = {
   0x00,0x01,0x02,0x03,0x00,0x84,0x85,0x89,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0xec,0xe3,0xe2,0xe1,0xe0,0xdf,0x15,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x24,0x25,0x26,0x37,0x06,0x29,0x2a,0x2b,
   0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,
   0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,
   0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,
   0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,
   0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,
   0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,
   0x5c,0x5d,0x5e,0xa9,0x33,0xce,0x00,0x15,
   0x00,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,
   0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,
   0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,
   0x7c,0x7d,0x7e,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
   0x45,0x45,0x45,0x45,0x45,0x45,0x24,0x47,
   0x49,0x49,0x49,0x49,0x4d,0x4d,0x4d,0x4d,
   0x48,0x52,0x53,0x53,0x53,0x53,0x53,0x24,
   0x24,0x59,0x59,0x59,0x59,0x5d,0x24,0x24,
   0x65,0x65,0x65,0x65,0x65,0x65,0x24,0x67,
   0x69,0x69,0x69,0x69,0x6d,0x6d,0x6d,0x6d,
   0x73,0x72,0x73,0x73,0x73,0x73,0x73,0x24,
   0x24,0x79,0x79,0x79,0x79,0x7d,0x24,0x7d
};
#endif /* HAVE_NEW_CHARCELL_LCD */

void lcd_clear_display(void)
{
    int i;
    lcd_write(TRUE,LCD_CURSOR(0,0));
    for (i=0;i<24;i++)
        lcd_write(FALSE,0);
}

void lcd_puts(int x, int y, char *string)
{
    lcd_write(TRUE,LCD_CURSOR(x,y));
    while (*string)
        lcd_write(FALSE,LCD_ASCII(*string++));
}

void lcd_define_pattern (int which,char *pattern,int length)
{
    int i;
    lcd_write(TRUE,LCD_PRAM|which);
    for (i=0;i<length;i++)
	lcd_write(FALSE,pattern[i]);
}

#elif HAVE_LCD_BITMAP /* not CHARCELLS */

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
unsigned char display[LCD_WIDTH][LCD_HEIGHT/8];

/*
 * ASCII character generation tables
 *
 * This contains only the printable characters (0x20-0x7f).
 * Each element in this table is a character pattern bitmap.
 */
#define	ASCII_MIN			0x20	/* First char in table */
#define	ASCII_MAX			0x7f	/* Last char in table */

extern unsigned char char_gen_6x8[][5][1];
extern unsigned char char_gen_8x12[][7][2];
extern unsigned char char_gen_12x16[][11][2];

/* All zeros and ones bitmaps for area filling */
static unsigned char zeros[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00 };
static unsigned char ones[]  = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				 0xff, 0xff };
static char fonts[] = { 6,8,12 };
static char fontheight[] = { 8,12,16 };

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
    for (y = 0; y < LCD_HEIGHT/8; y++)
    {
        lcd_write (TRUE, LCD_CNTL_PAGE | (y & 0xf));
        lcd_write (TRUE, LCD_CNTL_HIGHCOL);
        lcd_write (TRUE, LCD_CNTL_LOWCOL);

        for (x = 0; x < LCD_WIDTH; x++)
            lcd_write (FALSE, display[x][y]);
    }
}

#endif /* SIMULATOR */

/*
 * Clear the display
 */
void lcd_clear_display (void)
{
    memset (display, 0, sizeof display);
}

/*
 * Put a string at specified position and font
 */
void lcd_puts(int x, int y, char *str, int font)
{
    int nx = fonts[font];
    int ny = fontheight[font];
    int ch;
    unsigned char *src;
    int lcd_x = x;
    int lcd_y = y;

    while ((ch = *str++) != '\0')
    {
        if (ch == '\n' || lcd_x + nx > LCD_WIDTH)
        {
            /* Wrap to next line */
            lcd_x = x;
            lcd_y += ny;
        }

        if (lcd_y + ny > LCD_HEIGHT)
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
void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
                 bool clear)
{
    unsigned char *dst2;
    unsigned int mask1, mask2;
    int shift;
    int nny, _nnx = x, _nny = y;

    for (nny = ny; nny > 0; nny -= 8, _nny += 8)
    {
        x = _nnx; y = _nny; ny = (nny >= 8)?8:nny;

        dst2 = &display[x][y/8];
        shift = y & 7;

        mask1 = 0xFF >> (8 - shift);
        mask2 = ~mask1;

        ny += shift;

        /* Loop for each column */
        if (clear)
        {
            for (x = 0; x < nx; x++)
            {
                *dst2 = (*dst2 & mask1) | ((*(src)) << shift);
                *(dst2 + 1) = (*(dst2+1) & mask2) | ((*(src++)) >> (8 - shift));
                dst2 += LCD_HEIGHT/8;
            }
        }
        else
        {
            for (x = 0; x < nx; x++)
            {
                *dst2 |= (*(src)) << shift;
                *(dst2 + 1) |= (*(src++)) >> (8 - shift);
                dst2 += LCD_HEIGHT/8;
            }
        }
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

#define DRAW_PIXEL(x,y) display[x][y/8] |= (1<<(y&7))
#define CLEAR_PIXEL(x,y) display[x][y/8] &= ~(1<<(y&7))

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
 * Return width and height of a given font.
 */
void lcd_fontsize(char font, char *width, char *height)
{
    if(font < sizeof(fonts)) {
        *width =  fonts[font];
        *height = fontheight[font];
    }
}

#else
/* no LCD defined, no code to use */
#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */

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
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"

#ifdef LOADABLE_FONTS
#include "ajf.h"
#include "panic.h"
#endif

#if defined(SIMULATOR)
#include "sim_icons.h"
#endif


/*** definitions ***/

#define LCDR (PBDR_ADDR+1)

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

struct scrollinfo {
    char text[MAX_PATH];
    char line[32];
    int textlen;
    int offset;
    int startx;
    int starty;
    int space;
};

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static char scroll_name[] = "scroll";
static char scroll_speed = 8; /* updates per second */
static char scroll_spacing = 3; /* spaces between end and start of text */


static struct scrollinfo scroll; /* only one scroll line at the moment */
static int scroll_count = 0;

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

#define ASM_IMPLEMENTATION

static void lcd_write(bool command, int byte) __attribute__ ((section (".icode")));
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
             /* %2 */ "I"(~(LCD_SC|LCD_SD|LCD_DS)),
             /* %3 */ "I"(LCD_SD),
             /* %4 */ "I"(LCD_SC),
             /* %5 */ "z"(LCDR));
    else
#if 0
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
             /* %2 */ "I"(~(LCD_SC|LCD_SD)),
             /* %3 */ "I"(LCD_SD|LCD_DS),
             /* %4 */ "I"(LCD_SC|LCD_DS),
             /* %5 */ "z"(LCDR));
#else
        asm ("shll8  %0\n"
             "0:       \n\t"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             "and.b  %2, @(r0,gbr)\n\t"
             "shll   %0\n\t"  
             "bf     1f\n\t"
             "or.b   %3, @(r0,gbr)\n"
             "1:       \n\t"
             "or.b   %4, @(r0,gbr)\n"
             :
             : /* %0 */ "r"(((unsigned)byte)<<16),
             /* %1 */ "r"(8),
             /* %2 */ "I"(~(LCD_SC|LCD_SD)),
             /* %3 */ "I"(LCD_SD|LCD_DS),
             /* %4 */ "I"(LCD_SC|LCD_DS),
             /* %5 */ "z"(LCDR));
#endif

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
#endif /* !SIMULATOR */

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
   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
   0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
   0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
   0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
   0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
   0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
   0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
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
   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
   0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
   0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
   0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
   0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
   0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,
   0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
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

#ifndef SIMULATOR
void lcd_clear_display(void)
{
    int i;
    lcd_write(true,LCD_CURSOR(0,0));
    for (i=0;i<32;i++)
        lcd_write(false,lcd_ascii[' ']);
}

void lcd_puts(int x, int y, unsigned char *string)
{
    lcd_write(true,LCD_CURSOR(x,y));
    while (*string && x++<11)
        lcd_write(false,lcd_ascii[*(unsigned char*)string++]);
}

void lcd_putc(int x, int y, unsigned char ch)
{
    lcd_write(true,LCD_CURSOR(x,y));
    lcd_write(false,lcd_ascii[ch]);
}

void lcd_define_pattern (int which,char *pattern,int length)
{
    int i;
    lcd_write(true,LCD_PRAM|which);
    for (i=0;i<length;i++)
        lcd_write(false,pattern[i]);
}

void lcd_double_height(bool on)
{
    lcd_write(true,on?9:8);
}

static char icon_pos[] =
{
    0, 0, 0, 0, /* Battery */
    2, /* USB */
    3, /* Play */
    4, /* Record */
    5, /* Pause */
    5, /* Audio */
    6, /* Repeat */
    7, /* 1 */
    9,  /* Volume */
    9,  /* Volume 1 */
    9,  /* Volume 2 */
    10, /* Volume 3 */
    10, /* Volume 4 */
    10, /* Volume 5 */
    10, /* Param */
};

static char icon_mask[] =
{
    0x02, 0x08, 0x04, 0x10, /* Battery */
    0x04, /* USB */
    0x10, /* Play */
    0x10, /* Record */
    0x02, /* Pause */
    0x10, /* Audio */
    0x02, /* Repeat */
    0x01, /* 1 */
    0x04, /* Volume */
    0x02, /* Volume 1 */
    0x01, /* Volume 2 */
    0x08, /* Volume 3 */
    0x04, /* Volume 4 */
    0x01, /* Volume 5 */
    0x10, /* Param */
};

void lcd_icon(int icon, bool enable)
{
    static unsigned char icon_mirror[11] = {0};
    int pos, mask;

    pos = icon_pos[icon];
    mask = icon_mask[icon];
    
    lcd_write(true, LCD_ICON(pos));
    
    if(enable)
        icon_mirror[pos] |= mask;
    else
        icon_mirror[pos] &= ~mask;
    
    lcd_write(false, icon_mirror[pos]);
}
#endif /* !SIMULATOR */

#endif /* HAVE_LCD_CHARCELLS */

#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
void lcd_icon(int icon, bool enable)
{
    sim_lcd_icon(icon, enable);
}
#endif

#if defined(HAVE_LCD_CHARCELLS) || defined(SIMULATOR) /* not BITMAP */
void lcd_init (void)
{
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
#if  defined(LOADABLE_FONTS) && defined(SIMULATOR)
    lcd_init_fonts();
#endif
}
#endif

#ifdef SIMULATOR
void lcd_set_contrast(int val)
{
    val = val;
}
#else
#ifdef HAVE_LCD_BITMAP
void lcd_set_contrast(int val)
{
    lcd_write(true, LCD_CNTL_CONTRAST);
    lcd_write(true, val);
}
#else
void lcd_set_contrast(int val)
{
    lcd_write(true, LCD_CONTRAST_SET);
    lcd_write(false, 31-val);
}
#endif
#endif

#if defined(HAVE_LCD_BITMAP) || defined(SIMULATOR) /* not CHARCELLS */

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
unsigned char lcd_framebuffer[LCD_WIDTH][LCD_HEIGHT/8] __attribute__ ((section (".idata")));

static int font=0;
static int xmargin=0;
static int ymargin=0;

/*
 * ASCII character generation tables
 *
 * This contains only the printable characters (0x20-0x7f).
 * Each element in this table is a character pattern bitmap.
 */
#define    ASCII_MIN            0x20    /* First char in table */
#define    ASCII_MAX            0x7f    /* Last char in table */

extern unsigned char char_gen_6x8[][5];
extern unsigned char char_gen_8x12[][14];
extern unsigned char char_gen_12x16[][22];

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

    lcd_clear_display();
    lcd_update();
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

/*
 * Update the display.
 * This must be called after all other LCD functions that change the display.
 */
void lcd_update (void) __attribute__ ((section (".icode")));
void lcd_update (void)
{
    int x, y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_HEIGHT/8; y++)
    {
        lcd_write (true, LCD_CNTL_PAGE | (y & 0xf));
        lcd_write (true, LCD_CNTL_HIGHCOL);
        lcd_write (true, LCD_CNTL_LOWCOL);

        for (x = 0; x < LCD_WIDTH; x++)
            lcd_write (false, lcd_framebuffer[x][y]);
    }
}

/*
 * Update a fraction of the display.
 */
void lcd_update_rect (int, int, int, int) __attribute__ ((section (".icode")));
void lcd_update_rect (int x_start, int y,
                      int width, int height)
{
    int ymax;
    int xmax;
    int x;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height)/8;
    y /= 8;

    xmax = x_start + width;

    if(xmax > LCD_WIDTH)
        xmax = LCD_WIDTH;
    if(ymax >= LCD_HEIGHT/8)
        ymax = LCD_HEIGHT/8-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write (true, LCD_CNTL_PAGE | (y & 0xf));
        lcd_write (true, LCD_CNTL_HIGHCOL | ((x_start>>4) & 0xf));
        lcd_write (true, LCD_CNTL_LOWCOL | (x_start & 0xf));

        for (x = x_start; x < xmax; x++)
            lcd_write (false, lcd_framebuffer[x][y]);
    }
}

#endif /* SIMULATOR */

/*
 * Clear the display
 */
void lcd_clear_display (void)
{
#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
    /* Clear just the text portion of the display */
    int x, y;
    for (y = 8; y <= (32 + 7); ++y)
    {
        for (x = 0; x < LCD_WIDTH; x++)
            CLEAR_PIXEL(x, y);
    }
    /* this function is being used when simulating a charcell LCD and
       then we update immediately */
    lcd_update();
#else
    memset (lcd_framebuffer, 0, sizeof lcd_framebuffer);
#endif
}

void lcd_setfont(int newfont)
{
    font = newfont;
}

void lcd_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_getxmargin(void)
{
    return xmargin;
}

int lcd_getymargin(void)
{
    return ymargin;
}



#ifdef LOADABLE_FONTS

static unsigned char* _font = NULL;

int lcd_init_fonts(void)
{
    if (!_font)
        _font = ajf_read_font("/system.ajf");

    if (!_font)
    {
        lcd_putsxy(0,0,"No font", 0);
        return -1;
    }

    return 0;
}

void lcd_setldfont(unsigned char* f)
{
    _font = f;
}

unsigned char* lcd_getcurrentldfont()
{
    if (!_font)
        panicf("No font loaded!");
    return _font;
}

/*
 * Return width and height of a string with a given font.
 */
int lcd_getstringsize(unsigned char *str, unsigned char* font, int *w, int *h)
{
    int width=0;
    int height=0;
    unsigned char ch;

    if (!font)
        panicf("No font specified");

    while((ch = *str++)) 
    {
        int dw,dh;
        ajf_get_charsize(ch, font, &dw, &dh);
        if (dh>height)
            height = dh;
        width+=dw;
    }
    *w = width;
    *h = height;

    return width;
}

/*
 * Put a string at specified bit position
 */

void lcd_putsldfxy(int x, int y, unsigned char *str)
{
    unsigned char ch;
    int nx;
    int ny=8;
    int lcd_x = x;
    int lcd_y = y;
    if (!_font)
    {
        lcd_putsxy(0,0,"No font", 0);
        return;
    }
    ny = (int)_font[2];
    while (((ch = *str++) != '\0'))
    {
        unsigned char *char_buf = ajf_get_charbuf(ch, _font, &nx, &ny);
        if (!char_buf)
        {
            char_buf = ajf_get_charbuf('?', _font, &nx, &ny);
            if (!char_buf)
                panicf("Bad font");
        }
        if(lcd_x + nx > LCD_WIDTH)
            break;

        lcd_bitmap (&char_buf[0], lcd_x, lcd_y, nx, ny, true);
        lcd_x += nx;
    }
}
#endif


#ifdef LCD_PROPFONTS

extern unsigned char char_dw_8x8_prop[][9];

/*
 * Return width and height of a given font.
 */
int lcd_getstringsize(unsigned char *str, unsigned int font, int *w, int *h)
{
    int width=0;
    int height=0;
    unsigned char ch, byte;
    (void)font;

    while((ch = *str++)) {
        /* Limit to char generation table */
        if (ch < ASCII_MIN)
            /* replace unsupported letters with question marks */
            ch = ' '-ASCII_MIN;
        else
            ch -= ASCII_MIN;

        byte = char_dw_8x8_prop[ch][8];
        width += (byte>>4) + 1;
        if((byte & 0x0f) > height)
            height = byte & 0x0f;

    }
    *w = width;
    *h = height;

    return width;
}

/*
 * Put a string at specified bit position
 */

void lcd_putspropxy(int x, int y, unsigned char *str, int thisfont)
{
    unsigned int ch;
    int nx;
    int ny=8;
    unsigned char *src;
    int lcd_x = x;
    int lcd_y = y;

    (void)thisfont;

    while (((ch = *str++) != '\0'))
    {
        /* Limit to char generation table */
        if (ch < ASCII_MIN)
            /* replace unsupported letters with question marks */
            ch = ' '-ASCII_MIN;
        else
            ch -= ASCII_MIN;
        
        nx = char_dw_8x8_prop[ch][8] >> 4;

        if(lcd_x + nx > LCD_WIDTH)
            break;

        src = char_dw_8x8_prop[ch];
        lcd_clearrect (lcd_x+nx, lcd_y, 1, ny);
        lcd_bitmap (src, lcd_x, lcd_y, nx, ny, true);

        lcd_x += nx+1;
    }
}

#endif

/*
 * Put a string at specified character position
 */
void lcd_puts(int x, int y, unsigned char *str)
{
#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
    /* We make the simulator truncate the string if it reaches the right edge,
       as otherwise it'll wrap. The real target doesn't wrap. */

    char buffer[12];
    if(strlen(str)+x > 11 ) {
        strncpy(buffer, str, sizeof buffer);
        buffer[11-x]=0;
        str = buffer;
    }
    xmargin = 0;
    ymargin = 8;
#endif

    if(!str || !str[0])
        return;

#ifdef LCD_PROPFONTS
    lcd_putspropxy( xmargin + x*fonts[font],
                    ymargin + y*fontheight[font],
                    str, font );
#elif LOADABLE_FONTS
    {
        int w,h;
        lcd_getstringsize(str,_font,&w,&h);
        lcd_putsldfxy( xmargin + x*w/strlen(str), ymargin + y*h, str );
    }
#else
    lcd_putsxy( xmargin + x*fonts[font],
                ymargin + y*fontheight[font],
                str, font );
#endif
#if defined(SIMULATOR) && defined(HAVE_LCD_CHARCELLS)
    /* this function is being used when simulating a charcell LCD and
       then we update immediately */
    lcd_update();
#endif
}


/*
 * Put a string at specified bit position
 */
void lcd_putsxy(int x, int y, unsigned char *str, int thisfont)
{
#ifdef LCD_PROPFONTS
    lcd_putspropxy(x,y,str,thisfont);
#else

    int nx = fonts[thisfont];
    int ny = fontheight[thisfont];
    int ch;
    unsigned char *src;
    int lcd_x = x;
    int lcd_y = y;

#ifdef LOADABLE_FONTS
    if ( _font ) {
        lcd_putsldfxy(x,y,str);
        return;
    }
#endif

    while (((ch = *str++) != '\0') && (lcd_x + nx < LCD_WIDTH))
    {
        if (lcd_y + ny > LCD_HEIGHT)
            return;

        /* Limit to char generation table */
        if ((ch < ASCII_MIN) || (ch > ASCII_MAX))
            /* replace unsupported letters with question marks */
            ch = '?' - ASCII_MIN;
        else
            ch -= ASCII_MIN;
        
        if (thisfont == 2)
            src = char_gen_12x16[ch];
        else if (thisfont == 1)
            src = char_gen_8x12[ch];
        else
            src = char_gen_6x8[ch];
        
        lcd_bitmap (src, lcd_x, lcd_y, nx-1, ny, true);
        lcd_bitmap (zeros, lcd_x+nx-1, lcd_y, 1, ny, true);

        lcd_x += nx;

    }
#endif
}

/*
 * Display a bitmap at (x, y), size (nx, ny)
 * clear is true to clear destination area first
 */
void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
                 bool clear) __attribute__ ((section (".icode")));
void lcd_bitmap (unsigned char *src, int x, int y, int nx, int ny,
                 bool clear)
{
    unsigned char *dst;
    unsigned char *dst2;
    unsigned int data, mask, mask2, mask3, mask4;
    int shift;

    if (((unsigned)x >= LCD_WIDTH) || ((unsigned)y >= LCD_HEIGHT))
        return;
    if (((unsigned)(x + nx)) >= LCD_WIDTH)
       nx = LCD_WIDTH - x;
    if (((unsigned)(y + ny)) >= LCD_HEIGHT)
        ny = LCD_HEIGHT - y;      

    shift = y & 7;
    dst2 = &lcd_framebuffer[x][y/8];
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
        dst2 += LCD_HEIGHT/8;
        data = 0;
        y = 0;

        if (ny > 8)
        {
            /* First partial row */
            data = *src++ << shift;
            *dst = (*dst & mask) | data;
            data >>= 8;
            dst++;

            /* Intermediate rows */
            for (y = 8; y < ny-8; y += 8)
            {
                data |= *src++ << shift;
                *dst = (*dst & mask2) | data;
                data >>= 8;
                dst++;
            }
        }

        /* Last partial row */
        if (y + shift < ny)
            data |= *src++ << shift;
        *dst = (*dst & mask3) | (data & mask4);
    }
}

/* 
 * Draw a rectangle with point a (upper left) at (x, y)
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

/*
 * Clear a rectangular area at (x, y), size (nx, ny)
 */
void lcd_clearrect (int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_bitmap (zeros, x+i, y, 1, ny, true);
}

/*
 * Fill a rectangular area at (x, y), size (nx, ny)
 */
void lcd_fillrect (int x, int y, int nx, int ny)
{
    int i;
    for (i = 0; i < nx; i++)
        lcd_bitmap (ones, x+i, y, 1, ny, true);
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

/*
 * Return width and height of a given font.
 */
void lcd_getfontsize(unsigned int font, int *width, int *height)
{
    if(font < sizeof(fonts)) {
        *width =  fonts[font];
        *height = fontheight[font];
    }
}

#else
/* no LCD defined, no code to use */
#endif

void lcd_puts_scroll(int x, int y, unsigned char* string )
{
    struct scrollinfo* s = &scroll;
#ifdef HAVE_LCD_CHARCELLS
    s->space = 11 - x;
#else

#if defined(LCD_PROPFONTS) || defined(LOADABLE_FONTS)
    unsigned char ch[2];
    int w, h;
#endif
    int width, height;
    lcd_getfontsize(font, &width, &height);
#if defined(LCD_PROPFONTS) || defined(LOADABLE_FONTS)
    ch[1] = 0; /* zero terminate */
    ch[0] = string[0];
    width = 0;
    s->space = 0;
    while ( ch[0] &&
#ifdef LCD_PROPFONTS
            (width + lcd_getstringsize(ch, 0, &w, &h) <
             (LCD_WIDTH - x*8))) {
#else
            (width + lcd_getstringsize(ch, _font, &w, &h) <
             (LCD_WIDTH - x*8))) {
#endif
        width += w;
        s->space++;
        ch[0]=string[s->space];
    }
#else
    s->space = (LCD_WIDTH - xmargin - x*width) / width;
#endif
#endif

    lcd_puts(x,y,string);
    s->textlen = strlen(string);


#if defined(LCD_PROPFONTS)
    s->space += 2;
    lcd_getstringsize(string,0,&w,&h);
    if ( w > LCD_WIDTH - xmargin ) {
#elif defined(LOADABLE_FONTS)
    s->space += 2;
    lcd_getstringsize(string,_font,&w,&h);
    if ( w > LCD_WIDTH - xmargin ) {
#else
    if ( s->textlen > s->space ) {
#endif
        s->offset=s->space;
        s->startx=x;
        s->starty=y;
        strncpy(s->text,string,sizeof s->text);
        s->text[sizeof s->text - 1] = 0;
        memset(s->line, 0, sizeof s->line);
        strncpy(s->line,string,
                s->space > (int)sizeof s->line ?
                (int)sizeof s->line : s->space );
        s->line[sizeof s->line - 1] = 0;
        scroll_count = 1;
    }
}

void lcd_stop_scroll(void)
{
    if ( scroll_count ) {
        struct scrollinfo* s = &scroll;
        scroll_count = 0;
        
#ifdef LCD_PROPFONTS

        lcd_clearrect(xmargin + s->startx*fonts[font],
                      ymargin + s->starty*fontheight[font],
                      LCD_WIDTH - xmargin,
                      fontheight[font]);

#elif defined(LOADABLE_FONTS)
        {
            int w,h;
            lcd_getstringsize( s->text, _font, &w, &h);
            lcd_clearrect(xmargin + s->startx*w/s->textlen,
                          ymargin + s->starty*h,
                          LCD_WIDTH - xmargin,
                          h);

        }
#endif


        /* restore scrolled row */
        lcd_puts(s->startx,s->starty,s->text);
        lcd_update();
    }
}

void lcd_scroll_pause(void)
{
    scroll_count = 0;
}

void lcd_scroll_resume(void)
{
    scroll_count = 1;
}

void lcd_scroll_speed(int speed)
{
    scroll_speed = speed;
}

static void scroll_thread(void)
{
    struct scrollinfo* s = &scroll;

    while ( 1 ) {
        if ( !scroll_count ) {
            yield();
            continue;
        }
        /* wait 0.5s before starting scroll */
        if ( scroll_count < scroll_speed/2 )
            scroll_count++;
        else {
            int i;
            for ( i=0; i<s->space-1; i++ )
                s->line[i] = s->line[i+1];

            if ( s->offset < s->textlen ) {
                s->line[(int)s->space - 1] = s->text[(int)s->offset];
                s->offset++;
            }
            else {
                s->line[s->space - 1] = ' ';
                if ( s->offset < s->textlen + scroll_spacing - 1 )
                    s->offset++;
                else
                    s->offset = 0;
            }

#ifdef LCD_PROPFONTS
            lcd_clearrect(xmargin + s->startx*fonts[font],
                          ymargin + s->starty*fontheight[font],
                          LCD_WIDTH - xmargin,
                          fontheight[font]);
#elif defined(LOADABLE_FONTS)
            {
                int w,h;
                lcd_getstringsize( s->text, _font, &w, &h);
                lcd_clearrect(xmargin + s->startx*w/s->textlen,
                              ymargin + s->starty*h,
                              LCD_WIDTH - xmargin,
                              h);
            }
#endif
            lcd_puts(s->startx,s->starty,s->line);
            lcd_update();
        }
        sleep(HZ/scroll_speed);
    }
}


/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */

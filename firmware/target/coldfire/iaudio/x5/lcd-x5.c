/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "bidi.h"

/* Is the display turned on? */
static bool display_on = false;

/* Amount of vertical offset. Used for offset correction when flipped. */
static int y_offset = 0;

/* Amount of roll offset (0-127). */
static int roll_offset = 0;

/* A15(0x8000) && CS1->CS, A1(0x0002)->RS */
#define LCD_CMD  *(volatile unsigned short *)0xf0008000
#define LCD_DATA *(volatile unsigned short *)0xf0008002

/* register defines for the Renesas HD66773R */
#define R_START_OSC             0x00
#define R_DEVICE_CODE_READ      0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_AC_CONTROL        0x02
#define R_POWER_CONTROL1        0x03
#define R_POWER_CONTROL2        0x04
#define R_ENTRY_MODE            0x05
#define R_COMPARE_REG           0x06
#define R_DISP_CONTROL          0x07
#define R_FRAME_CYCLE_CONTROL   0x0b
#define R_POWER_CONTROL3        0x0c
#define R_POWER_CONTROL4        0x0d
#define R_POWER_CONTROL5        0x0e
#define R_GATE_SCAN_START_POS   0x0f
#define R_VERT_SCROLL_CONTROL   0x11
#define R_1ST_SCR_DRV_POS       0x14
#define R_2ND_SCR_DRV_POS       0x15
#define R_HORIZ_RAM_ADDR_POS    0x16
#define R_VERT_RAM_ADDR_POS     0x17
#define R_RAM_WRITE_DATA_MASK   0x20
#define R_RAM_ADDR_SET          0x21
#define R_WRITE_DATA_2_GRAM     0x22
#define R_RAM_READ_DATA         0x22
#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33
#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37
#define R_GAMMA_AMP_ADJ_POS     0x3a
#define R_GAMMA_AMP_ADJ_NEG     0x3b

/* called very frequently - inline! */
inline void lcd_write_reg(int reg, int val)
{
    LCD_CMD = 0x0000; /* MSB is ~always~ 0 */
    LCD_CMD = reg << 1;
    LCD_DATA = (val >> 8) << 1;
    LCD_DATA = (val & 0xff) << 1;
}

/* called very frequently - inline! */
inline void lcd_begin_write_gram(void)
{
    LCD_CMD = 0x0000;
    LCD_CMD = R_WRITE_DATA_2_GRAM << 1;
}

static inline void lcd_write_one(unsigned short px)
{
    unsigned short pxsr = px >> 8;
    LCD_DATA = pxsr + (pxsr & 0x1F8);
    LCD_DATA = px << 1;
}

/* Write two pixels to gram from a long */
/* called very frequently - inline! */
static inline void lcd_write_two(unsigned long px2)
{
    unsigned short px2sr = px2 >> 24;
    LCD_DATA = px2sr + (px2sr & 0x1F8);
    LCD_DATA = px2 >> 15;
    px2sr = px2 >> 8;
    LCD_DATA = px2sr + (px2sr & 0x1F8);
    LCD_DATA = px2 << 1;
}
 
/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_set_contrast(int val)
{
    /* Clamp val in range 0-14, 16-30 */
    if (val < 1)
        val = 0;
    else if (val <= 15)
        --val;
    else if (val > 30)
        val = 30;

    lcd_write_reg(R_POWER_CONTROL5, 0x2018 + (val << 8));
}

void lcd_set_invert_display(bool yesno)
{
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, yesno ? 0x0033 : 0x0037);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    y_offset = yesno ? 4 : 0;
    /* SCN4-0=000x0 (G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, yesno ? 0x0000 : 0x0002);
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, yesno ? 0x0013 : 0x0313);
    /* Vertical stripe */
    /* HEA7-0=0xxx, HSA7-0=0xxx */  
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0x7f00 + (y_offset << 8) + y_offset);
}

/* Rolls up the lcd display by the specified amount of lines.
 * Lines that are rolled out over the top of the screen are
 * rolled in from the bottom again. This is a hardware 
 * remapping only and all operations on the lcd are affected.
 * -> 
 * @param int lines - The number of lines that are rolled. 
 *  The value must be 0 <= pixels < LCD_HEIGHT.
 * Call lcd_update() afterwards */
void lcd_roll(int lines)
{
    /* Just allow any value mod LCD_HEIGHT-1. Assumes LCD_HEIGHT == 128. */
    if (lines < 0)
        lines = -lines & 127;
    else
        lines = (128 - (lines & 127)) & 127;

    roll_offset = lines;
}

/* LCD init */
void lcd_init_device(void)
{
    /* LCD Reset */
    and_l(~0x00000010, &GPIO1_OUT);
    or_l(0x00000010, &GPIO1_ENABLE);
    or_l(0x00000010, &GPIO1_FUNCTION);
    sleep(HZ/100);
    or_l(0x00000010, &GPIO1_OUT);

    sleep(HZ/100);

    /** Power ON Sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 99 */

    lcd_write_reg(R_START_OSC, 0x0001); /* Start Oscillation */
    /* 10ms or more for oscillation circuit to stabilize */
    sleep(HZ/50);
    /* Instruction (1) for power setting; VC2-0, VRH3-0, CAD,
       VRL3-0, VCM4-0, VDV4-0 */
    /* VC2-0=001 */
    lcd_write_reg(R_POWER_CONTROL3, 0x0001);
    /* VRL3-0=0100, PON=0, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0401);
    /* CAD=1 */
    lcd_write_reg(R_POWER_CONTROL2, 0x8000);
    /* VCOMG=0, VDV4-0=10011 (19), VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x1318);
    /* Instruction (2) for power setting; BT2-0, DC2-0, AP2-0 */
    /* BT2-0=000, DC2-0=001, AP2-0=011, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x002c);
    /* Instruction (3) for power setting; VCOMG = "1" */
    /* VCOMG=1, VDV4-0=10011 (19), VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x3318);

    /* 40ms or more; time for step-up circuits 1,2 to stabilize */
    sleep(HZ/25);

    /* Instruction (4) for power setting; PON = "1" */
    /* VRL3-0=0100, PON=1, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0411);
    
    /* 40ms or more; time for step-up circuit 4 to stabilize */
    sleep(HZ/25);

    /* Instructions for other mode settings (in register order). */    
    /* SM=0, GS=1, SS=1, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 0x313);
    /* FLD1-0=01 (1 field), B/C=1, EOR=1 (C-pat), NW5-0=000000 (1 row) */
    lcd_write_reg(R_DRV_AC_CONTROL, 0x0700);
    /* DIT=1, BGR=1, HWM=0, I/D1-0=11, AM=1, LG2-0=000 */
    lcd_write_reg(R_ENTRY_MODE, 0x9038);
    /* CP15-0=0000000000000000 */
    lcd_write_reg(R_COMPARE_REG, 0x0000);
    /* NO1-0=01, SDT1-0=00, EQ1-0=00, DIV1-0=00, RTN3-00000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4000);
    /* SCN4-0=00010 (G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, 0x0002);
    /* VL7-0=0x00 */
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    /* SE17-10(End)=0x9f (159), SS17-10(Start)=0x00 */
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    /* SE27-20(End)=0x5c (92), SS27-20(Start)=0x00 */
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x5c00);
    /* HEA7-0=0x7f, HSA7-0=0x00 */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 0x7f00); /* Vertical stripe */
    /* PKP12-10=0x0, PKP02-00=0x0 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x0003);
    /* PKP32-30=0x4, PKP22-20=0x0 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0400);
    /* PKP52-50=0x4, PKP42-40=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0407);
    /* PRP12-10=0x3, PRP02-00=0x5 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x0305);
    /* PKN12-10=0x0, PKN02-00=0x3 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0003);
    /* PKN32-30=0x7, PKN22-20=0x4 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 0x0704);
    /* PKN52-50=0x4, PRN42-40=0x7 */
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0407);
    /* PRN12-10=0x5, PRN02-00=0x3 */
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 0x0503);
    /* VRP14-10=0x14, VRP03-00=0x09 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_POS, 0x1409);
    /* VRN14-00=0x06, VRN03-00=0x02 */
    lcd_write_reg(R_GAMMA_AMP_ADJ_NEG, 0x0602);

    /* 100ms or more; time for step-up circuits to stabilize */
    sleep(HZ/10);

    /** Display ON Sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=1, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0005);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=1, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0025);
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=1, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0027);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=1, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0037);

    display_on = true;
    y_offset = 0;
    roll_offset = 0;
}

void lcd_enable(bool on)
{
    display_on = on;
}

/*** update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit(const fb_data* data, int x, int by, int width,
              int bheight, int stride)
{
    /* TODO: Implement lcd_blit() */
    (void)data;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
    /*if(display_on)*/ 
}


/* Update the display.
   This must be called after all other LCD functions that change the
   lcd frame buffer. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    /* Optimized for full-screen write. */
    const unsigned long *ptr, *ptr_end;

    if (!display_on)
        return;

    /* Set start position and window */
    /* Just add roll offset to start address. CP will roll back around. */
    lcd_write_reg(R_RAM_ADDR_SET, y_offset + roll_offset); /* X == 0 */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (LCD_WIDTH-1) << 8);

    lcd_begin_write_gram();

    ptr = (unsigned long *)lcd_framebuffer;
    ptr_end = ptr + (LCD_WIDTH*LCD_HEIGHT >> 1);

    do
    {
        /* 16 words per turns out to be about optimal according to
           test_fps. */
        lcd_write_two(*ptr++);
#ifndef BOOTLOADER
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
        lcd_write_two(*ptr++);
#endif
     }
     while (ptr < ptr_end);
} /* lcd_update */

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int y_end;
    int odd_lead, odd_trail;
    int duff;
    const unsigned long *ptr, *duff_end;
    int stride; /* Actually end of currline -> start of next */

    if (!display_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    y_end = y + height;
    if (y_end > LCD_HEIGHT)
        y_end = LCD_HEIGHT; /* Clip bottom */
    if (y < 0)
        y = 0; /* Clip top */
    if (y >= y_end)
        return; /* nothing left to do */

    ptr = (unsigned long *)&lcd_framebuffer[y][x];

    /* Set start position and window */
    lcd_write_reg(R_RAM_ADDR_SET, (x << 8) |
        (((y + roll_offset) & 127) + y_offset));
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((x + width - 1) << 8) | x);

    lcd_begin_write_gram();

    /* Aligning source reads to long boundaries helps 2% - 3% with IRAM
       buffer. DK with DRAM. */

    /* special case widths 1 and 2. */
    switch (width)
    {
    case 1:
        odd_lead = 1; /* odd_lead case writes pixels */
        odd_trail = 0;
        duff = 0; /* Squelch compiler warning. */
        duff_end = ptr;
        break;
    case 2: /* Just read as long */
        odd_lead = 0;
        odd_trail = 0;
        duff = 1;
        duff_end = ptr + 1;
        break;
    default:
        odd_lead = x & 1;

        if (odd_lead)
        {
            duff = width - 1;
            odd_trail = duff & 1;
            duff >>= 1;
        }
        else
        {
            duff = width >> 1;
            odd_trail = width & 1;
        }

        duff_end = ptr + duff;
#ifndef BOOTLOADER        
        duff &= 7;
#endif
    } /* end switch */

    stride = LCD_WIDTH - width + odd_trail; /* See odd_trail below */

    do
    {
        if (odd_lead)
        {
            /* Write odd start pixel. */
            lcd_write_one(*(unsigned short *)ptr);
            ptr = (unsigned long *)((short *)ptr + 1);
        }

        if (ptr < duff_end)
        {
#ifdef BOOTLOADER
            do
                lcd_write_two(*ptr);
            while (++ptr < duff_end);
#else           
            switch (duff)
            {
            do
            {
                case 0: lcd_write_two(*ptr++);
                case 7: lcd_write_two(*ptr++);
                case 6: lcd_write_two(*ptr++);
                case 5: lcd_write_two(*ptr++);
                case 4: lcd_write_two(*ptr++);
                case 3: lcd_write_two(*ptr++);
                case 2: lcd_write_two(*ptr++);
                case 1: lcd_write_two(*ptr++);
            }
            while (ptr < duff_end);
            } /* end switch */
#endif /* BOOTLOADER */

            duff_end += LCD_WIDTH/2;
        }

        if (odd_trail)
        {
            /* Finish remaining odd pixel. */
            lcd_write_one(*(unsigned short *)ptr);
            /* Stride increased by one pixel. */
        }

        ptr = (unsigned long *)((short *)ptr + stride);
    }
    while (++y < y_end);
}

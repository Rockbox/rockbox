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

/** Initialized in lcd_init_device() **/
/* Is the power turned on? */
static bool power_on;
/* Is the display turned on? */
static bool display_on;
/* Amount of vertical offset. Used for flip offset correction/detection. */
static int y_offset;
/* Amount of roll offset (0-127). */
static int roll_offset;
/* Reverse flag. Must be remembered when display is turned off. */
static unsigned short disp_control_rev;

/* Forward declarations */
static void lcd_display_off(void);

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
static inline void lcd_write_reg(int reg, int val)
{
    LCD_CMD = 0x0000; /* MSB is ~always~ 0 */
    LCD_CMD = reg << 1;
    LCD_DATA = (val >> 8) << 1;
    LCD_DATA = val << 1;
}

/* called very frequently - inline! */
static inline void lcd_begin_write_gram(void)
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
    if (yesno == (disp_control_rev == 0x0000))
        return;

    disp_control_rev = yesno ? 0x0000 : 0x0004;

    if (!display_on)
        return;

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | disp_control_rev);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno == (y_offset != 0))
        return;

    y_offset = yesno ? 4 : 0;

    if (!power_on)
        return;

    /* SCN4-0=000x0 (G1/G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, yesno ? 0x0000 : 0x0002);
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, yesno ? 0x0013 : 0x0313);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, yesno ? 0x8304 : 0x7f00);
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

static void lcd_power_on(void)
{
    /* Be sure standby bit is clear. */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);

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
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, y_offset ? 0x0013 : 0x0313);
    /* FLD1-0=01 (1 field), B/C=1, EOR=1 (C-pat), NW5-0=000000 (1 row) */
    lcd_write_reg(R_DRV_AC_CONTROL, 0x0700);
    /* DIT=1, BGR=1, HWM=0, I/D1-0=11, AM=1, LG2-0=000 */
    lcd_write_reg(R_ENTRY_MODE, 0x9038);
    /* CP15-0=0000000000000000 */
    lcd_write_reg(R_COMPARE_REG, 0x0000);
    /* NO1-0=01, SDT1-0=00, EQ1-0=00, DIV1-0=00, RTN3-00000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4000);
    /* SCN4-0=000x0 (G1/G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, y_offset ? 0x0000 : 0x0002);
    /* VL7-0=0x00 */
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    /* SE17-10(End)=0x9f (159), SS17-10(Start)=0x00 */
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    /* SE27-20(End)=0x5c (92), SS27-20(Start)=0x00 */
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x5c00);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, y_offset ? 0x8304 : 0x7f00);
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

    power_on = true;
}

static void lcd_power_off(void)
{
    /* Display must be off first */
    if (display_on)
        lcd_display_off();

    power_on = false;

    /** Power OFF sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 99 */

    /* Step-up1 halt setting bit */
    /* BT2-0=110, DC2-0=001, AP2-0=011, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x062c);
    /* Step-up3,4 halt setting bit */
    /* VRL3-0=0100, PON=0, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0401);
    /* VCOMG=0, VDV4-0=10011, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x1318);

    /* Wait 100ms or more */
    sleep(HZ/10);

    /* Step-up2,amp halt setting bit */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0000);
}

static void lcd_display_on(void)
{
    /* Be sure power is on first */
    if (!power_on)
        lcd_power_on();

    /** Display ON Sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0001);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=01 */
    lcd_write_reg(R_DISP_CONTROL, 0x0021 | disp_control_rev);
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0023 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | disp_control_rev);

    display_on = true;
}

static void lcd_display_off(void)
{
    display_on = false;

    /** Display OFF sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* EQ1-0=00 already */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0032 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0022 | disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x0000);
}

/* LCD init */
void lcd_init_device(void)
{
    /* Reset settings */
    power_on = false;
    display_on = false;
    y_offset = 0;
    roll_offset = 0;
    disp_control_rev = 0x0004;

    /* LCD Reset */
    and_l(~0x00000010, &GPIO1_OUT);
    or_l(0x00000010, &GPIO1_ENABLE);
    or_l(0x00000010, &GPIO1_FUNCTION);
    sleep(HZ/100);
    or_l(0x00000010, &GPIO1_OUT);

    sleep(HZ/100);

    lcd_display_on();
}

void lcd_enable(bool on)
{
    if (on == display_on)
        return;

    if (on)
    {
        lcd_display_on();
        /* Probably out of sync and we don't wanna pepper the code with
           lcd_update() calls for this. */
        lcd_update();
    }
    else
    {
        lcd_display_off();
    }
}

void lcd_sleep(void)
{
    if (power_on)
        lcd_power_off();

    /* Set standby mode */
    /* BT2-0=000, DC2-0=000, AP2-0=000, SLP=0, STB=1 */
    lcd_write_reg(R_POWER_CONTROL1, 0x0001);
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

/* Performance function to blit a YUV bitmap directly to the LCD */
/* Assumes YCrCb 4:2:0. */
/*
   See http://en.wikipedia.org/wiki/YCbCr
   ITU-R BT.601 (formerly CCIR 601):
   |Y'|   | 0.299000  0.587000  0.114000| |R|
   |Pb| = |-0.168736 -0.331264  0.500000| |G| or 0.564334 * (B - Y')
   |Pr|   | 0.500000 -0.418688  0.081312| |B| or 0.713267 * (R - Y')
   Scaled, normalized and rounded:
   |Y'|   | 65  129   25| |R| +  16 : 16->235
   |Cb| = |-38  -74  112| |G| + 128 : 16->240
   |Cr|   |112  -94  -18| |B| + 128 : 16->240

   The inverse:
   |R|   |1.000000 -0.000001  1.402000| |Y'|
   |G| = |1.000000 -0.334136 -0.714136| |Pb|
   |B|   |1.000000  1.772000  0.000000| |Pr|
   Scaled, normalized, rounded and tweaked to yield RGB 666:
   |R|   |298    0  409| |Y' -  16| / 1024
   |G| = |298 -100 -208| |Cb - 128| / 1024
   |B|   |298  516    0| |Cr - 128| / 1024
*/
void lcd_yuv_blit(unsigned char * const [3], int, int, int,
                  int, int, int, int) ICODE_ATTR;
void lcd_yuv_blit(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    int uv_stepper, uv_step, y_end;

    if (!display_on)
        return;

    width = (width + 1) & ~1;
    height = (height + 1) & ~1;
    y_end = y + height;

    /* Set start position and window */
    lcd_write_reg(R_RAM_ADDR_SET, (x << 8) |
        (((y + roll_offset) & 127) + y_offset));
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((x + width - 1) << 8) | x);

    lcd_begin_write_gram();

    ysrc = src[0] + src_y*stride + src_x;
    usrc = src[1] + (src_y*stride >> 2) + (src_x >> 1);
    vsrc = src[2] + (usrc - src[1]);

    stride = stride - width; /* Use end of current line->start of next */
    uv_stepper = (stride >> 1) - (width >> 1);
    uv_step = uv_stepper - (stride >> 1);

    do
    {
        const unsigned char *ysrc_end = ysrc + width;

        do
        {
            int lum, cb, cr;
            int rv, guv, bu;
            int r, g, b;

            lum = 298* *ysrc++ - 4768; /* 298*16 */
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;
            bu = 516*cb;
            guv = -100*cb - 208*cr;
            rv = 409*cr;

            r = (lum + rv) >> 10;
            g = (lum + guv) >> 10;
            b = (lum + bu) >> 10;

            if ((unsigned)r > 63)
                r = (r < 0) ? 0 : 63;
            if ((unsigned)g > 63)
                g = (g < 0) ? 0 : 63;
            if ((unsigned)b > 63)
                b = (b < 0) ? 0 : 63;

            LCD_DATA = (r << 3) | (g >> 3);
            LCD_DATA = (g << 6) | b;

            lum = 298* *ysrc++ - 4768; /* 298*16 */
            r = (lum + rv) >> 10;
            g = (lum + guv) >> 10;
            b = (lum + bu) >> 10;

            if ((unsigned)r > 63)
                r = (r < 0) ? 0 : 63;
            if ((unsigned)g > 63)
                g = (g < 0) ? 0 : 63;
            if ((unsigned)b > 63)
                b = (b < 0) ? 0 : 63;

            LCD_DATA = (r << 3) | (g >> 3);
            LCD_DATA = (g << 6) | b;
        }
        while (ysrc < ysrc_end);

        usrc += uv_step;
        vsrc += uv_step;
        uv_step = uv_stepper - uv_step;

        ysrc += stride;
    }
    while (++y < y_end);
} /* lcd_yuv_blit */

/* Update the display.
   This must be called after all other LCD functions that change the
   lcd frame buffer. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    if (!display_on)
        return;

    /* Set start position and window */
    /* Just add roll offset to start address. CP will roll back around. */
    lcd_write_reg(R_RAM_ADDR_SET, y_offset + roll_offset); /* X == 0 */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (LCD_WIDTH-1) << 8);

    lcd_begin_write_gram();

    lcd_write_data((unsigned short *)lcd_framebuffer,
        LCD_WIDTH*LCD_HEIGHT);
} /* lcd_update */

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;
    const unsigned short *ptr;

    if (!display_on)
        return;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* Clip right */
    if (x < 0)
        width += x, x = 0; /* Clip left */
    if (width <= 0)
        return; /* nothing left to do */

    ymax = y + height;
    if (ymax > LCD_HEIGHT)
        ymax = LCD_HEIGHT; /* Clip bottom */
    if (y < 0)
        y = 0; /* Clip top */
    if (y >= ymax)
        return; /* nothing left to do */

    /* Set start position and window */
    lcd_write_reg(R_RAM_ADDR_SET, (x << 8) |
        (((y + roll_offset) & 127) + y_offset));
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((x + width - 1) << 8) | x);

    lcd_begin_write_gram();

    ptr = (unsigned short *)&lcd_framebuffer[y][x];

    do
    {
        lcd_write_data(ptr, width);
        ptr += LCD_WIDTH;
    }
    while (++y < ymax);
} /* lcd_update_rect */

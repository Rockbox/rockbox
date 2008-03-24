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

/* Power and display status */
static bool power_on   = false; /* Is the power turned on?   */
static bool display_on = false; /* Is the display turned on? */

/* Contrast */
static int lcd_contrast = DEFAULT_CONTRAST_SETTING << 8;

/* Reverse Flag */
#define R_DISP_CONTROL_NORMAL   0x0004
#define R_DISP_CONTROL_REV      0x0000
static unsigned short r_disp_control_rev = R_DISP_CONTROL_NORMAL;

/* Flipping */
#define Y_OFFSET_NORMAL     0
#define Y_OFFSET_FLIPPED    4
static int            y_offset              = 0; /* y correction for flip */
static unsigned short r_gate_scan_start_pos = 0x0002;
static unsigned short r_drv_output_control  = 0x0313;
static unsigned short r_horiz_ram_addr_pos  = 0x7f00;

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

#define R_ENTRY_MODE_SOLID_VERT  0x1038
#define R_ENTRY_MODE_DIT_HORZ    0x9030

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

    lcd_contrast = val << 8;

    if (!power_on)
        return;

    /* VCOMG=1, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);
}

void lcd_set_invert_display(bool yesno)
{
    r_disp_control_rev = yesno ? R_DISP_CONTROL_REV :
                                 R_DISP_CONTROL_NORMAL;

    if (!display_on)
        return;

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | r_disp_control_rev);
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    if (yesno)
    {
        y_offset              = Y_OFFSET_FLIPPED;
        r_gate_scan_start_pos = 0x0000;
        r_drv_output_control  = 0x0013;
        r_horiz_ram_addr_pos  = 0x8304;
    }
    else
    {
        y_offset              = Y_OFFSET_NORMAL;
        r_gate_scan_start_pos = 0x0002;
        r_drv_output_control  = 0x0313;
        r_horiz_ram_addr_pos  = 0x7f00;
    }

    if (!power_on)
        return;

    /* SCN4-0=000x0 (G1/G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, r_gate_scan_start_pos);
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL,  r_drv_output_control);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS,  r_horiz_ram_addr_pos);
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
    /* VCOMG=0, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x0018 | lcd_contrast);
    /* Instruction (2) for power setting; BT2-0, DC2-0, AP2-0 */
    /* BT2-0=000, DC2-0=001, AP2-0=011, SLP=0, STB=0 */
    lcd_write_reg(R_POWER_CONTROL1, 0x002c);
    /* Instruction (3) for power setting; VCOMG = "1" */
    /* VCOMG=1, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x2018 | lcd_contrast);

    /* 40ms or more; time for step-up circuits 1,2 to stabilize */
    sleep(HZ/25);

    /* Instruction (4) for power setting; PON = "1" */
    /* VRL3-0=0100, PON=1, VRH3-0=0001 */
    lcd_write_reg(R_POWER_CONTROL4, 0x0411);

    /* 40ms or more; time for step-up circuit 4 to stabilize */
    sleep(HZ/25);

    /* Instructions for other mode settings (in register order). */
    /* SM=0, GS=x, SS=x, NL4-0=10011 (G1-G160)*/
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, r_drv_output_control);
    /* FLD1-0=01 (1 field), B/C=1, EOR=1 (C-pat), NW5-0=000000 (1 row) */
    lcd_write_reg(R_DRV_AC_CONTROL, 0x0700);
    /* DIT=x, BGR=1, HWM=0, I/D1-0=11, AM=1, LG2-0=000 */
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_SOLID_VERT);
    /* CP15-0=0000000000000000 */
    lcd_write_reg(R_COMPARE_REG, 0x0000);
    /* NO1-0=01, SDT1-0=00, EQ1-0=00, DIV1-0=00, RTN3-00000 */
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, 0x4000);
    /* SCN4-0=000x0 (G1/G160) */
    lcd_write_reg(R_GATE_SCAN_START_POS, r_gate_scan_start_pos);
    /* VL7-0=0x00 */
    lcd_write_reg(R_VERT_SCROLL_CONTROL, 0x0000);
    /* SE17-10(End)=0x9f (159), SS17-10(Start)=0x00 */
    lcd_write_reg(R_1ST_SCR_DRV_POS, 0x9f00);
    /* SE27-20(End)=0x5c (92), SS27-20(Start)=0x00 */
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0x5c00);
    /* HEA7-0=0xxx, HSA7-0=0xxx */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, r_horiz_ram_addr_pos);
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
    /* VCOMG=0, VDV4-0=xxxxx, VCM4-0=11000 */
    lcd_write_reg(R_POWER_CONTROL5, 0x0018 | lcd_contrast);

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
    lcd_write_reg(R_DISP_CONTROL, 0x0021 | r_disp_control_rev);
    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0023 | r_disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=11 */
    lcd_write_reg(R_DISP_CONTROL, 0x0033 | r_disp_control_rev);

    display_on = true;
}

static void lcd_display_off(void)
{
    display_on = false;

    /** Display OFF sequence **/
    /* Per datasheet Rev.1.10, Jun.21.2003, p. 97 */

    /* EQ1-0=00 already */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=1, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0032 | r_disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=1, DTE=0, REV=x, D1-0=10 */
    lcd_write_reg(R_DISP_CONTROL, 0x0022 | r_disp_control_rev);

    sleep(HZ/25); /* Wait 2 frames or more */

    /* PT1-0=00, VLE2-1=00, SPT=0, GON=0, DTE=0, REV=0, D1-0=00 */
    lcd_write_reg(R_DISP_CONTROL, 0x0000);
}

/* LCD init */
void lcd_init_device(void)
{
#ifdef BOOTLOADER
    /* Initial boot requires power on reset and setting up chip
       registers but a full reset is not needed again. */

    /* LCD Reset */
    and_l(~0x00000010, &GPIO1_OUT);
    or_l(0x00000010, &GPIO1_ENABLE);
    or_l(0x00000010, &GPIO1_FUNCTION);
    sleep(HZ/100);
    or_l(0x00000010, &GPIO1_OUT);

    sleep(HZ/100);

    lcd_display_on();
#else
    /* Power and display already ON - reset settings */
    power_on   = true;
    display_on = true;
    lcd_set_contrast(DEFAULT_CONTRAST_SETTING);
    lcd_set_invert_display(false);
    lcd_set_flip(false);
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_SOLID_VERT);
                 /* do this or all bootloaders will need reflashing */
#endif
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

bool lcd_enabled(void)
{
    return display_on;
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

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420.
 * y should have two lines of Y back to back, 2nd line first.
 * c should contain the Cb and Cr data for the two lines of Y back to back.
 * Needs EMAC set to saturated, signed integer mode.
 */
extern void lcd_write_yuv420_lines(const unsigned char *y,
                                   const unsigned char *c, int width);

/* Performance function to blit a YUV bitmap directly to the LCD
 * src_x, src_y, width and height should be even and within the LCD's
 * boundaries.
 */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    /* IRAM Y, Cb/bu, guv and Cb/rv buffers. */
    unsigned char y_ibuf[LCD_WIDTH*2];
    unsigned char c_ibuf[LCD_WIDTH];
    const unsigned char *ysrc, *usrc, *vsrc;
    const unsigned char *ysrc_max;

    if (!display_on)
        return;

    width &= ~1;  /* stay on the safe side */
    height &= ~1;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_DIT_HORZ);
    /* Set start position and window */
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (LCD_WIDTH-1) << 8);

    ysrc = src[0] + src_y * stride + src_x;
    usrc = src[1] + (src_y * stride >> 2) + (src_x >> 1);
    vsrc = src[2] + (src_y * stride >> 2) + (src_x >> 1);
    ysrc_max = ysrc + height * stride;

    coldfire_set_macsr(EMAC_SATURATE);
    do
    {
        lcd_write_reg(R_HORIZ_RAM_ADDR_POS, ((y + y_offset + 1) << 8) | (y + y_offset));
        lcd_write_reg(R_RAM_ADDR_SET, (x << 8) | (y + y_offset));
        lcd_begin_write_gram();

        memcpy(y_ibuf + width, ysrc, width);
        memcpy(y_ibuf, ysrc + stride, width);
        memcpy(c_ibuf, usrc, width >> 1);
        memcpy(c_ibuf + (width >> 1), vsrc, width >> 1);
        lcd_write_yuv420_lines(y_ibuf, c_ibuf, width >> 1);
        
        y += 2;
        ysrc += 2 * stride;
        usrc += stride >> 1;
        vsrc += stride >> 1;
    }
    while (ysrc < ysrc_max);
} /* lcd_yuv_blit */


/* Update the display.
   This must be called after all other LCD functions that change the
   lcd frame buffer. */
void lcd_update(void)
{
    if (!display_on)
        return;

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_SOLID_VERT);
    /* Set start position and window */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 
                  ((y_offset + LCD_HEIGHT-1) << 8) | y_offset);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, (LCD_WIDTH-1) << 8);
    lcd_write_reg(R_RAM_ADDR_SET, y_offset);

    lcd_begin_write_gram();

    lcd_write_data((unsigned short *)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
} /* lcd_update */

/* Update a fraction of the display. */
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

    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_SOLID_VERT);
    /* Set start position and window */
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, 
                  ((y_offset + LCD_HEIGHT-1) << 8) | y_offset);
    lcd_write_reg(R_VERT_RAM_ADDR_POS, ((x + width - 1) << 8) | x);
    lcd_write_reg(R_RAM_ADDR_SET, (x << 8) | (y + y_offset));

    lcd_begin_write_gram();

    ptr = (unsigned short *)&lcd_framebuffer[y][x];

    do
    {
        lcd_write_data(ptr, width);
        ptr += LCD_WIDTH;
    }
    while (++y < ymax);
} /* lcd_update_rect */

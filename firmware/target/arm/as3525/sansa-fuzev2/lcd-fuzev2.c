/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Dave Chapman
 * Copyright (C) 2010 by Thomas Martitz
 *
 * LCD driver for the Sansa Fuze - controller unknown
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#include "cpu.h"
#include "lcd.h"
#include "file.h"
#include "debug.h"
#include "system.h"
#include "clock-target.h"

/* The controller is unknown, but some registers appear to be the same as the
   HD66789R */
static bool display_on = false; /* is the display turned on? */

/* register defines */
#define R_START_OSC             0x00
#define R_DRV_OUTPUT_CONTROL    0x01
#define R_DRV_WAVEFORM_CONTROL  0x02
#define R_ENTRY_MODE            0x03
#define R_COMPARE_REG1          0x04
#define R_COMPARE_REG2          0x05

#define R_DISP_CONTROL1     0x07
#define R_DISP_CONTROL2     0x08
#define R_DISP_CONTROL3     0x09

#define R_FRAME_CYCLE_CONTROL 0x0b
#define R_EXT_DISP_IF_CONTROL 0x0c

#define R_POWER_CONTROL1    0x10
#define R_POWER_CONTROL2    0x11
#define R_POWER_CONTROL3    0x12
#define R_POWER_CONTROL4    0x13

#define R_RAM_ADDR_SET  0x21
#define R_WRITE_DATA_2_GRAM 0x22

#define R_GAMMA_FINE_ADJ_POS1   0x30
#define R_GAMMA_FINE_ADJ_POS2   0x31
#define R_GAMMA_FINE_ADJ_POS3   0x32
#define R_GAMMA_GRAD_ADJ_POS    0x33

#define R_GAMMA_FINE_ADJ_NEG1   0x34
#define R_GAMMA_FINE_ADJ_NEG2   0x35
#define R_GAMMA_FINE_ADJ_NEG3   0x36
#define R_GAMMA_GRAD_ADJ_NEG    0x37

#define R_GAMMA_AMP_ADJ_RES_POS     0x38
#define R_GAMMA_AMP_AVG_ADJ_RES_NEG 0x39

#define R_GATE_SCAN_POS         0x40
#define R_VERT_SCROLL_CONTROL   0x41
#define R_1ST_SCR_DRV_POS       0x42
#define R_2ND_SCR_DRV_POS       0x43
#define R_HORIZ_RAM_ADDR_POS    0x44
#define R_VERT_RAM_ADDR_POS     0x45

/* Flip Flag */
#define R_ENTRY_MODE_HORZ_NORMAL 0x7030
#define R_ENTRY_MODE_HORZ_FLIPPED 0x7000
static unsigned short r_entry_mode = R_ENTRY_MODE_HORZ_NORMAL;
#define R_ENTRY_MODE_VERT 0x7038
#define R_ENTRY_MODE_SOLID_VERT  0x1038
/* FIXME */
#define R_ENTRY_MODE_VIDEO_NORMAL 0x7038
#define R_ENTRY_MODE_VIDEO_FLIPPED 0x7018

/* Reverse Flag */
#define R_DISP_CONTROL_NORMAL 0x0004
#define R_DISP_CONTROL_REV    0x0000
static unsigned short r_disp_control_rev = R_DISP_CONTROL_NORMAL;

static const int xoffset = 20;

static inline void lcd_delay(int x)
{
    do {
        asm volatile ("nop\n");
    } while (x--);
}

#define REG(x) (*(volatile unsigned long*)(x))
typedef unsigned long reg;

static void as3525_dbop_init(void)
{
#if 0
    CGU_DBOP = (1<<3) | AS3525_DBOP_DIV;

    DBOP_TIMPOL_01 = 0xe167e167;
    DBOP_TIMPOL_23 = 0xe167006e;

    /* short count: 16 | output data width: 16 | readstrobe line */
    DBOP_CTRL = (1<<18|1<<12|1<<3);

    GPIOB_AFSEL = 0xfc;
    GPIOC_AFSEL = 0xff;

    DBOP_TIMPOL_23 = 0x6000e;

    /* short count: 16|enable write|output data width: 16|read strobe line */
    DBOP_CTRL = (1<<18|1<<16|1<<12|1<<3);
    DBOP_TIMPOL_01 = 0x6e167;
    DBOP_TIMPOL_23 = 0xa167e06f;

    /* TODO: The OF calls some other functions here, but maybe not important */
#endif
    REG(0xC810000C) |= 0x1000; /* CCU_IO |= 1<<12 */
    CGU_DBOP |= /*(1<<3)*/ 0x18 | AS3525_DBOP_DIV;
    DBOP_TIMPOL_01 = 0xE12FE12F;
    DBOP_TIMPOL_23 = 0xE12F0036;
    DBOP_CTRL = 0x41004;
    DBOP_TIMPOL_23 = 0x60036;
    DBOP_CTRL = 0x51004;
    DBOP_TIMPOL_01 = 0x60036;
    DBOP_TIMPOL_23 = 0xA12FE037;
    /* OF sets up dma and more after here */
}

static inline void dbop_set_mode(int mode)
{
    int delay = 10;
    if (mode == 32 && (!(DBOP_CTRL & (1<<13|1<<14))))
        DBOP_CTRL |= (1<<13|1<<14);
    else if (mode == 16 && (DBOP_CTRL & (1<<13|1<<14)))
        DBOP_CTRL &= ~(1<<14|1<<13);
    else
        return;
    while(delay--) asm volatile("nop");
}

static void dbop_write_data(const int16_t* p_bytes, int count)
{
    
    const int32_t *data;
    if ((intptr_t)p_bytes & 0x3 || count == 1)
    {   /* need to do a single 16bit write beforehand if the address is
         * not word aligned or count is 1, switch to 16bit mode if needed */
        dbop_set_mode(16);
        DBOP_DOUT16 = *p_bytes++;
        if (!(--count))
            return;
    }
    /* from here, 32bit transfers are save
     * set it to transfer 4*(outputwidth) units at a time,
     * if bit 12 is set it only does 2 halfwords though (we never set it)
     * switch to 32bit output if needed */
    dbop_set_mode(32);
    data = (int32_t*)p_bytes;
    while (count > 1)
    {
        DBOP_DOUT32 = *data++;
        count -= 2;

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    }
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    /* due to the 32bit alignment requirement or uneven count,
     * we possibly need to do a 16bit transfer at the end also */
    if (count > 0)
        dbop_write_data((int16_t*)data, 1);
}

static void lcd_write_cmd(short cmd)
{
#if 0
    /* Write register */
    DBOP_TIMPOL_23 = 0xa167006e;
    dbop_write_data(&cmd, 1);

    lcd_delay(4);

    DBOP_TIMPOL_23 = 0xa167e06f;
#elif 1
    volatile int i;
    for(i=0;i<20;i++) nop;

    int r3 = 0x2000;
    DBOP_CTRL |= r3;
    r3 >>= 1;
    DBOP_CTRL &= ~r3;
    r3 <<= 2;
    DBOP_CTRL &= ~r3;
    DBOP_TIMPOL_23 = 0xA12F0036;
    cmd = swap16(cmd);
    DBOP_DOUT16 = cmd;

    while ((DBOP_STAT & (1<<10)) == 0);
    for(i=0;i<20;i++) nop;
    DBOP_TIMPOL_23 = 0xA12FE037;
#else
    int i;
    DBOP_TIMPOL_23 = 0xA12F0036;
    for(i=0;i<20;i++) nop;
    dbop_write_data(&cmd, 1);
    for(i=0;i<20;i++) nop;
    DBOP_TIMPOL_23 = 0xA12FE037;
#endif
}

static void lcd_write_reg(int reg, int value)
{
    int16_t data = value;
    lcd_write_cmd(reg);
    dbop_write_data(&data, 1);
}

/*** hardware configuration ***/

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    r_disp_control_rev = yesno ? R_DISP_CONTROL_REV :
                                 R_DISP_CONTROL_NORMAL;

    if (display_on)
    {
        lcd_write_reg(R_DISP_CONTROL1, 0x0013 | r_disp_control_rev);
    }

}

#ifdef HAVE_LCD_FLIP
static bool display_flipped = false;

/* turn the display upside down  */
void lcd_set_flip(bool yesno)
{
    display_flipped = yesno;

    r_entry_mode = yesno ? R_ENTRY_MODE_HORZ_FLIPPED :
                           R_ENTRY_MODE_HORZ_NORMAL;
}
#endif

static void _display_on(void)
{
    /* Initialise in the same way as the original firmare */

    lcd_write_reg(R_DISP_CONTROL1, 0);
    lcd_write_reg(R_POWER_CONTROL4, 0);

    lcd_write_reg(R_POWER_CONTROL2, 0x3704);
    lcd_write_reg(0x14, 0x1a1b);
    lcd_write_reg(R_POWER_CONTROL1, 0x3860);
    lcd_write_reg(R_POWER_CONTROL4, 0x40);

    lcd_write_reg(R_POWER_CONTROL4, 0x60);

    lcd_write_reg(R_POWER_CONTROL4, 0x70);
    lcd_write_reg(R_DRV_OUTPUT_CONTROL, 277);
    lcd_write_reg(R_DRV_WAVEFORM_CONTROL, (7<<8));
    lcd_write_reg(R_ENTRY_MODE, r_entry_mode);
    lcd_write_reg(R_DISP_CONTROL2, 0x01);
    lcd_write_reg(R_FRAME_CYCLE_CONTROL, (1<<10));
    lcd_write_reg(R_EXT_DISP_IF_CONTROL, 0);

    lcd_write_reg(R_GAMMA_FINE_ADJ_POS1, 0x40);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS2, 0x0687);
    lcd_write_reg(R_GAMMA_FINE_ADJ_POS3, 0x0306);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_POS, 0x104);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG1, 0x0585);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG2, 255+66);
    lcd_write_reg(R_GAMMA_FINE_ADJ_NEG3, 0x0687+128);
    lcd_write_reg(R_GAMMA_GRAD_ADJ_NEG, 259);
    lcd_write_reg(R_GAMMA_AMP_ADJ_RES_POS, 0);
    lcd_write_reg(R_GAMMA_AMP_AVG_ADJ_RES_NEG, 0);

    lcd_write_reg(R_1ST_SCR_DRV_POS, (LCD_WIDTH - 1));
    lcd_write_reg(R_2ND_SCR_DRV_POS, 0);
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS, (LCD_WIDTH - 1));
    lcd_write_reg(R_VERT_RAM_ADDR_POS, 0);
    lcd_write_reg(0x46, (((LCD_WIDTH - 1) + xoffset) << 8) | xoffset);
    lcd_write_reg(0x47, (LCD_HEIGHT - 1));
    lcd_write_reg(0x48, 0x0);

    lcd_write_reg(R_DISP_CONTROL1, 0x11);
    lcd_write_reg(R_DISP_CONTROL1, 0x13 | r_disp_control_rev);

    display_on = true;  /* must be done before calling lcd_update() */
    lcd_update();
}

void lcd_init_device(void)
{
    as3525_dbop_init();

    GPIOA_DIR |= (0x20|0x1);
    GPIOA_DIR &= ~(1<<3);
    GPIOA_PIN(3) = 0;
    GPIOA_PIN(0) = 1;
    GPIOA_PIN(4) = 0;

    CCU_IO &= ~(0x1000);
    GPIOB_DIR |= 0x2f;
    GPIOB_PIN(0) = 1<<0;
    GPIOB_PIN(1) = 1<<1;
    GPIOB_PIN(2) = 1<<2;
    GPIOB_PIN(3) = 1<<3;
    GPIOA_PIN(4) = 1<<4;
    GPIOA_PIN(5) = 1<<5;

    _display_on();
}

#if defined(HAVE_LCD_ENABLE)
void lcd_enable(bool on)
{
    if (display_on == on)
        return;

    if(on)
    {
        lcd_write_reg(R_START_OSC, 1);
        lcd_write_reg(R_POWER_CONTROL1, 0);
        lcd_write_reg(R_POWER_CONTROL2, 0x3704);
        lcd_write_reg(0x14, 0x1a1b);
        lcd_write_reg(R_POWER_CONTROL1, 0x3860);
        lcd_write_reg(R_POWER_CONTROL4, 0x40);
        lcd_write_reg(R_POWER_CONTROL4, 0x60);
        lcd_write_reg(R_POWER_CONTROL4, 112);
        lcd_write_reg(R_DISP_CONTROL1, 0x11);
        lcd_write_reg(R_DISP_CONTROL1, 0x13 | r_disp_control_rev);
        display_on = true;
        lcd_update();      /* Resync display */
        send_event(LCD_EVENT_ACTIVATION, NULL);
        sleep(0);

    }
    else
    {
        lcd_write_reg(R_DISP_CONTROL1, 0x22);
        lcd_write_reg(R_DISP_CONTROL1, 0);
        lcd_write_reg(R_POWER_CONTROL1, 1);
        display_on = false;
    }
}
#endif

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
bool lcd_active(void)
{
    return display_on;
}
#endif

/*** update functions ***/

/* FIXME : find the datasheet for this RENESAS controller so we identify the
 * registers used in windowing code (not present in HD66789R) */

/* Set horizontal window addresses */
static void lcd_window_x(int xmin, int xmax)
{
    xmin += xoffset;
    xmax += xoffset;
    lcd_write_reg(R_HORIZ_RAM_ADDR_POS + 2, (xmax << 8) | xmin);
    lcd_write_reg(R_RAM_ADDR_SET - 1, xmin);
}

/* Set vertical window addresses */
static void lcd_window_y(int ymin, int ymax)
{
    lcd_write_reg(R_VERT_RAM_ADDR_POS + 2, ymax);
    lcd_write_reg(R_VERT_RAM_ADDR_POS + 3, ymin);
    lcd_write_reg(R_RAM_ADDR_SET, ymin);
}

static unsigned lcd_yuv_options = 0;

void lcd_yuv_set_options(unsigned options)
{
    lcd_yuv_options = options;
}

/* Line write helper function for lcd_yuv_blit. Write two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   int width,
                                   int stride);
extern void lcd_write_yuv420_lines_odither(unsigned char const * const src[3],
                                   int width,
                                   int stride,
                                   int x_screen, /* To align dither pattern */
                                   int y_screen);

/* Performance function to blit a YUV bitmap directly to the LCD
 * src_x, src_y, width and height should be even
 * x, y, width and height have to be within LCD bounds
 */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned char const * yuv_src[3];
    off_t z;

    /* Sorry, but width and height must be >= 2 or else */
    width &= ~1;
    height >>= 1;

    z = stride*src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

#ifdef HAVE_LCD_FLIP
    lcd_write_reg(R_ENTRY_MODE,
        display_flipped ? R_ENTRY_MODE_VIDEO_FLIPPED : R_ENTRY_MODE_VIDEO_NORMAL
    );
#else
    lcd_write_reg(R_ENTRY_MODE, R_ENTRY_MODE_VIDEO_NORMAL);
#endif

    lcd_window_x(x, x + width - 1);

    if (lcd_yuv_options & LCD_YUV_DITHER)
    {
        do
        {
            lcd_window_y(y, y + 1);

            lcd_write_cmd(R_WRITE_DATA_2_GRAM);

            lcd_write_yuv420_lines_odither(yuv_src, width, stride, x, y);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
        while (--height > 0);
    }
    else
    {
        do
        {
            lcd_window_y(y, y + 1);

            lcd_write_cmd(R_WRITE_DATA_2_GRAM);

            lcd_write_yuv420_lines(yuv_src, width, stride);
            yuv_src[0] += stride << 1; /* Skip down two luma lines */
            yuv_src[1] += stride >> 1; /* Skip down one chroma line */
            yuv_src[2] += stride >> 1;
            y += 2;
        }
        while (--height > 0);
    }
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void)
{
    if (!display_on)
        return;

    lcd_write_reg(R_ENTRY_MODE, r_entry_mode);

    lcd_window_x(0, LCD_WIDTH - 1);
    lcd_window_y(0, LCD_HEIGHT - 1);

    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    lcd_update_rect(0,0, LCD_WIDTH, LCD_HEIGHT);
    //dbop_write_data((fb_data*)lcd_framebuffer, LCD_WIDTH*LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int x, int y, int width, int height)
{
    const fb_data *ptr;

    if (!display_on)
        return;

    /* nothing to draw? */
    if ((width <= 0) || (height <= 0) || (x >= LCD_WIDTH) || 
        (y >= LCD_HEIGHT) || (x + width <= 0) || (y + height <= 0))
        return;

    if (x < 0)
    {   /* clip left */
        width += x;
        x = 0;
    }
    if (y < 0)
    {   /* clip top */
        height += y;
        y = 0;
    }
    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x; /* clip right */
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y; /* clip bottom */

    lcd_write_reg(R_ENTRY_MODE, r_entry_mode);

    /* we need to make x and width even to enable 32bit transfers */
    width = (width + (x & 1) + 1) & ~1;
    x &= ~1;

    lcd_window_x(x, x + width - 1);
    lcd_window_y(y, y + height -1);

    lcd_write_cmd(R_WRITE_DATA_2_GRAM);

    ptr = &lcd_framebuffer[y][x];

    do
    {
        dbop_write_data(ptr, width);
        ptr += LCD_WIDTH;
    }
    while (--height > 0);
}

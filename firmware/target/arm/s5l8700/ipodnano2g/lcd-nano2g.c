/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Dave Chapman
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

#include "hwcompat.h"
#include "kernel.h"
#include "lcd.h"
#include "system.h"
#include "cpu.h"
#include "pmu-target.h"
#include "power.h"


/* The Nano 2G has two different LCD types.  What we call "type 0"
   appears to be similar to the ILI9320 and "type 1" is similar to the
   LDS176.
*/

/* LCD type 0 register defines */

#define R_ENTRY_MODE              0x03
#define R_DISPLAY_CONTROL_1       0x07
#define R_POWER_CONTROL_1         0x10
#define R_POWER_CONTROL_2         0x12
#define R_POWER_CONTROL_3         0x13
#define R_HORIZ_GRAM_ADDR_SET     0x20
#define R_VERT_GRAM_ADDR_SET      0x21
#define R_WRITE_DATA_TO_GRAM      0x22
#define R_HORIZ_ADDR_START_POS    0x50
#define R_HORIZ_ADDR_END_POS      0x51
#define R_VERT_ADDR_START_POS     0x52
#define R_VERT_ADDR_END_POS       0x53


/* LCD type 1 register defines */

#define R_SLEEP_IN                0x10
#define R_DISPLAY_OFF             0x28
#define R_COLUMN_ADDR_SET         0x2a
#define R_ROW_ADDR_SET            0x2b
#define R_MEMORY_WRITE            0x2c

/** globals **/

int lcd_type; /* also needed in debug-s5l8700.c */
static int xoffset; /* needed for flip */
static bool lcd_ispowered;

#ifdef HAVE_LCD_SLEEP

#define SLEEP   0
#define CMD16   1
#define DATA16  2

unsigned short lcd_init_sequence_0[] = {
    CMD16,  0x00a4, DATA16, 0x0001,
    SLEEP,  0x0000,
    CMD16,  0x0001, DATA16, 0x0100,
    CMD16,  0x0002, DATA16, 0x0300,
    CMD16,  0x0003, DATA16, 0x1230,
    CMD16,  0x0008, DATA16, 0x0404,
    CMD16,  0x0008, DATA16, 0x0404,
    CMD16,  0x000e, DATA16, 0x0010,
    CMD16,  0x0070, DATA16, 0x1000,
    CMD16,  0x0071, DATA16, 0x0001,
    CMD16,  0x0030, DATA16, 0x0002,
    CMD16,  0x0031, DATA16, 0x0400,
    CMD16,  0x0032, DATA16, 0x0007,
    CMD16,  0x0033, DATA16, 0x0500,
    CMD16,  0x0034, DATA16, 0x0007,
    CMD16,  0x0035, DATA16, 0x0703,
    CMD16,  0x0036, DATA16, 0x0507,
    CMD16,  0x0037, DATA16, 0x0005,
    CMD16,  0x0038, DATA16, 0x0407,
    CMD16,  0x0039, DATA16, 0x000e,
    CMD16,  0x0040, DATA16, 0x0202,
    CMD16,  0x0041, DATA16, 0x0003,
    CMD16,  0x0042, DATA16, 0x0000,
    CMD16,  0x0043, DATA16, 0x0200,
    CMD16,  0x0044, DATA16, 0x0707,
    CMD16,  0x0045, DATA16, 0x0407,
    CMD16,  0x0046, DATA16, 0x0505,
    CMD16,  0x0047, DATA16, 0x0002,
    CMD16,  0x0048, DATA16, 0x0004,
    CMD16,  0x0049, DATA16, 0x0004,
    CMD16,  0x0060, DATA16, 0x0202,
    CMD16,  0x0061, DATA16, 0x0003,
    CMD16,  0x0062, DATA16, 0x0000,
    CMD16,  0x0063, DATA16, 0x0200,
    CMD16,  0x0064, DATA16, 0x0707,
    CMD16,  0x0065, DATA16, 0x0407,
    CMD16,  0x0066, DATA16, 0x0505,
    CMD16,  0x0068, DATA16, 0x0004,
    CMD16,  0x0069, DATA16, 0x0004,
    CMD16,  0x0007, DATA16, 0x0001,
    CMD16,  0x0018, DATA16, 0x0001,
    CMD16,  0x0010, DATA16, 0x1690,
    CMD16,  0x0011, DATA16, 0x0100,
    CMD16,  0x0012, DATA16, 0x0117,
    CMD16,  0x0013, DATA16, 0x0f80,
    CMD16,  0x0012, DATA16, 0x0137,
    CMD16,  0x0020, DATA16, 0x0000,
    CMD16,  0x0021, DATA16, 0x0000,
    CMD16,  0x0050, DATA16, 0x0000,
    CMD16,  0x0051, DATA16, 0x00af,
    CMD16,  0x0052, DATA16, 0x0000,
    CMD16,  0x0053, DATA16, 0x0083,
    CMD16,  0x0090, DATA16, 0x0003,
    CMD16,  0x0091, DATA16, 0x0000,
    CMD16,  0x0092, DATA16, 0x0101,
    CMD16,  0x0098, DATA16, 0x0400,
    CMD16,  0x0099, DATA16, 0x1302,
    CMD16,  0x009a, DATA16, 0x0202,
    CMD16,  0x009b, DATA16, 0x0200,
    SLEEP,  0x0000,
    CMD16,  0x0007, DATA16, 0x0021,
    CMD16,  0x0012, DATA16, 0x0137,
    SLEEP,  0x0000,
    CMD16,  0x0007, DATA16, 0x0021,
    CMD16,  0x0012, DATA16, 0x1137,
    SLEEP,  0x0000,
    CMD16,  0x0007, DATA16, 0x0233,
};

unsigned short lcd_init_sequence_1[] = {
    CMD16,  0x0011, DATA16, 0x0000,
    CMD16,  0x0029, DATA16, 0x0000,
    SLEEP,  0x0000,
};



#endif /* HAVE_LCD_SLEEP */

static inline void s5l_lcd_write_cmd_data(int cmd, int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;

    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_data(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data;
}

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
    (void)val;
}

void lcd_set_invert_display(bool yesno)
{
    (void)yesno;
}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    /* TODO: flip mode isn't working.  The commands in the else part of
       this function are how the original firmware inits the LCD */

    if (yesno)
    {
        xoffset = 132 - LCD_WIDTH; /* 132 colums minus the 128 we have */
    }
    else 
    {
        xoffset = 0;
    }
}

bool lcd_active(void)
{
    return lcd_ispowered;
}

#ifdef HAVE_LCD_SLEEP

static void lcd_wakeup(void)
{
    unsigned short *lcd_init_sequence;
    unsigned int lcd_init_sequence_length;

    PWRCONEXT &= ~0x80;
    PCON13 &= ~0xf;    /* Set pin 0 to input */
    PCON14 &= ~0xf0;   /* Set pin 1 to input */

    pmu_write(0x2b, 1);

    if (lcd_type == 0)
    {
        /* reset the lcd chip */

        LCD_RST_TIME = 0x7FFF;
        LCD_DRV_RST = 0;
        sleep(0);
        LCD_DRV_RST = 1;
        sleep(HZ / 100);

        lcd_init_sequence = lcd_init_sequence_0;
        lcd_init_sequence_length = (sizeof(lcd_init_sequence_0) - 1)/sizeof(unsigned short);
    }
    else
    {
        lcd_init_sequence = lcd_init_sequence_1;
        lcd_init_sequence_length = (sizeof(lcd_init_sequence_1) - 1)/sizeof(unsigned short);
    }

    for(unsigned int i=0;i<lcd_init_sequence_length;i+=2)
    {
        switch(lcd_init_sequence[i])
        {
            case CMD16:
                s5l_lcd_write_cmd(lcd_init_sequence[i+1]);
                break;
            case DATA16:
                s5l_lcd_write_data(lcd_init_sequence[i+1]);
                break;
            case SLEEP:
                sleep(lcd_init_sequence[i+1]);
                break;
            default:
                break;
        }
    }
    lcd_ispowered = true;
    send_event(LCD_EVENT_ACTIVATION, NULL);
}

void lcd_awake(void)
{
    if(!lcd_active()) lcd_wakeup();
}
#endif

void lcd_shutdown(void)
{
    pmu_write(0x2b, 0);  /* Kill the backlight, instantly. */
    pmu_write(0x29, 0);

    if (lcd_type == 0)
    {
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x0232);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3  , 0x1137); 
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x0201);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3  , 0x0137);
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x0200);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_1  , 0x0680);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_2  , 0x0160);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3  , 0x0127);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_1  , 0x0600);
    }
    else
    {
        s5l_lcd_write_cmd(R_DISPLAY_OFF);
        s5l_lcd_write_data(0);
        s5l_lcd_write_data(0);
        s5l_lcd_write_cmd(R_SLEEP_IN);
        s5l_lcd_write_data(0);
        s5l_lcd_write_data(0);
    }

    PWRCONEXT |= 0x80;

    lcd_ispowered = false;
}

void lcd_sleep(void)
{
    lcd_shutdown();
}

/* LCD init */
void lcd_init_device(void)
{
    /* Detect lcd type */

    PCON13 &= ~0xf;    /* Set pin 0 to input */
    PCON14 &= ~0xf0;   /* Set pin 1 to input */

    if (((PDAT13 & 1) == 0) && ((PDAT14 & 2) == 2)) {
        lcd_type   = 0;     /* Similar to ILI9320 - aka "type 2" */
        LCD_CON   |= 0x180; /* use 16 bit bus width, big endian */
    } else {
        lcd_type   = 1;     /* Similar to LDS176  - aka "type 7" */
        LCD_CON   |= 0x100; /* use 16 bit bus width, little endian */
    }
    
    LCD_PHTIME = 0x00; /* Set Phase Time (faster LCD IF than Apple OF) */

    lcd_ispowered = true;
}

/*** Update functions ***/

static inline void lcd_write_pixel(fb_data pixel)
{
    LCD_WDATA = pixel;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Helper function to set up drawing region and start drawing */
static void lcd_setup_drawing_region(int, int, int, int) ICODE_ATTR;
static void lcd_setup_drawing_region(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    
    x0 = x;                         /* start horiz */
    y0 = y;                         /* start vert */
    x1 = (x + width) - 1;           /* max horiz */
    y1 = (y + height) - 1;          /* max vert */

    if (lcd_type==0) {
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_START_POS, x0);
        s5l_lcd_write_cmd_data(R_HORIZ_ADDR_END_POS,   x1);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_START_POS,  y0);
        s5l_lcd_write_cmd_data(R_VERT_ADDR_END_POS,    y1);

        s5l_lcd_write_cmd_data(R_HORIZ_GRAM_ADDR_SET,  (x1 << 8) | x0);
        s5l_lcd_write_cmd_data(R_VERT_GRAM_ADDR_SET,   (y1 << 8) | y0);

        s5l_lcd_write_cmd(0);
        s5l_lcd_write_cmd(R_WRITE_DATA_TO_GRAM);
    } else {
        s5l_lcd_write_cmd(R_COLUMN_ADDR_SET);
        s5l_lcd_write_data(x0);            /* Start column */
        s5l_lcd_write_data(x1);            /* End column */

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_data(y0);            /* Start row */
        s5l_lcd_write_data(y1);            /* End row */

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }
}

/* Line write helper function. */
extern void lcd_write_line(const fb_data *addr, 
                           int pixelcount,
                           const unsigned int lcd_base_addr);

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    fb_data* p;
    
    /* Both x and width need to be preprocessed due to asm optimizations */
    x     = x & ~1;                 /* ensure x is even */
    width = (width + 3) & ~3;       /* ensure width is a multiple of 4 */

    lcd_setup_drawing_region(x, y, width, height);

    /* Copy display bitmap to hardware */
    p = FBADDR(x,y);
    if (LCD_WIDTH == width) {
        /* Write all lines at once */
        lcd_write_line(p, height*LCD_WIDTH, LCD_BASE);
    } else {
        do {
            /* Write a single line */
            lcd_write_line(p, width, LCD_BASE);
            p += LCD_WIDTH;
        } while (--height > 0 );
    }
}

/* Line write helper function for lcd_yuv_blit. Writes two lines of yuv420. */
extern void lcd_write_yuv420_lines(unsigned char const * const src[3],
                                   const unsigned int lcd_baseadress,
                                   int width,
                                   int stride);

/* Blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    unsigned int z;
    unsigned char const * yuv_src[3];
    
    width = (width + 1) & ~1;       /* ensure width is even */

    lcd_setup_drawing_region(x, y, width, height);

    z = stride * src_y;
    yuv_src[0] = src[0] + z + src_x;
    yuv_src[1] = src[1] + (z >> 2) + (src_x >> 1);
    yuv_src[2] = src[2] + (yuv_src[1] - src[1]);

    height >>= 1;

    do {
        lcd_write_yuv420_lines(yuv_src, LCD_BASE, width, stride);
        yuv_src[0] += stride << 1;
        yuv_src[1] += stride >> 1; /* Skip down one chroma line */
        yuv_src[2] += stride >> 1;
    } while (--height > 0);
}

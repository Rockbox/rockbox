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
#define CMD8    1
#define CMD16   2
#define DATA8   3
#define DATA16  4

unsigned short lcd_init_sequence_0[] = {
    CMD16,  0x00a4,
    DATA16, 0x0001,
    SLEEP,  0x0000,
    CMD16,  0x0001,
    DATA16, 0x0100,
    CMD16,  0x0002,
    DATA16, 0x0300,
    CMD16,  0x0003,
    DATA16, 0x1230,
    CMD16,  0x0008,
    DATA16, 0x0404,
    CMD16,  0x0008,
    DATA16, 0x0404,
    CMD16,  0x000e,
    DATA16, 0x0010,
    CMD16,  0x0070,
    DATA16, 0x1000,
    CMD16,  0x0071,
    DATA16, 0x0001,
    CMD16,  0x0030,
    DATA16, 0x0002,
    CMD16,  0x0031,
    DATA16, 0x0400,
    CMD16,  0x0032,
    DATA16, 0x0007,
    CMD16,  0x0033,
    DATA16, 0x0500,
    CMD16,  0x0034,
    DATA16, 0x0007,
    CMD16,  0x0035,
    DATA16, 0x0703,
    CMD16,  0x0036,
    DATA16, 0x0507,
    CMD16,  0x0037,
    DATA16, 0x0005,
    CMD16,  0x0038,
    DATA16, 0x0407,
    CMD16,  0x0039,
    DATA16, 0x000e,
    CMD16,  0x0040,
    DATA16, 0x0202,
    CMD16,  0x0041,
    DATA16, 0x0003,
    CMD16,  0x0042,
    DATA16, 0x0000,
    CMD16,  0x0043,
    DATA16, 0x0200,
    CMD16,  0x0044,
    DATA16, 0x0707,
    CMD16,  0x0045,
    DATA16, 0x0407,
    CMD16,  0x0046,
    DATA16, 0x0505,
    CMD16,  0x0047,
    DATA16, 0x0002,
    CMD16,  0x0048,
    DATA16, 0x0004,
    CMD16,  0x0049,
    DATA16, 0x0004,
    CMD16,  0x0060,
    DATA16, 0x0202,
    CMD16,  0x0061,
    DATA16, 0x0003,
    CMD16,  0x0062,
    DATA16, 0x0000,
    CMD16,  0x0063,
    DATA16, 0x0200,
    CMD16,  0x0064,
    DATA16, 0x0707,
    CMD16,  0x0065,
    DATA16, 0x0407,
    CMD16,  0x0066,
    DATA16, 0x0505,
    CMD16,  0x0068,
    DATA16, 0x0004,
    CMD16,  0x0069,
    DATA16, 0x0004,
    CMD16,  0x0007,
    DATA16, 0x0001,
    CMD16,  0x0018,
    DATA16, 0x0001,
    CMD16,  0x0010,
    DATA16, 0x1690,
    CMD16,  0x0011,
    DATA16, 0x0100,
    CMD16,  0x0012,
    DATA16, 0x0117,
    CMD16,  0x0013,
    DATA16, 0x0f80,
    CMD16,  0x0012,
    DATA16, 0x0137,
    CMD16,  0x0020,
    DATA16, 0x0000,
    CMD16,  0x0021,
    DATA16, 0x0000,
    CMD16,  0x0050,
    DATA16, 0x0000,
    CMD16,  0x0051,
    DATA16, 0x00af,
    CMD16,  0x0052,
    DATA16, 0x0000,
    CMD16,  0x0053,
    DATA16, 0x0083,
    CMD16,  0x0090,
    DATA16, 0x0003,
    CMD16,  0x0091,
    DATA16, 0x0000,
    CMD16,  0x0092,
    DATA16, 0x0101,
    CMD16,  0x0098,
    DATA16, 0x0400,
    CMD16,  0x0099,
    DATA16, 0x1302,
    CMD16,  0x009a,
    DATA16, 0x0202,
    CMD16,  0x009b,
    DATA16, 0x0200,
    SLEEP,  0x0000,
    CMD16,  0x0007,
    DATA16, 0x0021,
    CMD16,  0x0012,
    DATA16, 0x0137,
    SLEEP,  0x0000,
    CMD16,  0x0007,
    DATA16, 0x0021,
    CMD16,  0x0012,
    DATA16, 0x1137,
    SLEEP,  0x0000,
    CMD16,  0x0007,
    DATA16, 0x0233,
};

unsigned short lcd_init_sequence_1[] = {
    CMD8,  0x11,
    DATA16, 0x00,
    CMD8,  0x29,
    DATA16, 0x00,
};



#endif /* HAVE_LCD_SLEEP */

static inline void s5l_lcd_write_cmd_data(int cmd, int data)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd & 0xff;

    while (LCD_STATUS & 0x10);
    LCD_WDATA = data >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data & 0xff;
}

static inline void s5l_lcd_write_cmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd;
}

static inline void s5l_lcd_write_wcmd(unsigned short cmd)
{
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WCMD = cmd & 0xff;
}

static inline void s5l_lcd_write_data(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data & 0xff;
}

static inline void s5l_lcd_write_wdata(unsigned short data)
{
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data >> 8;
    while (LCD_STATUS & 0x10);
    LCD_WDATA = data & 0xff;
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

void lcd_wakeup(void)
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
            case CMD8:
                s5l_lcd_write_cmd(lcd_init_sequence[i+1]);
                break;
            case DATA8:
                s5l_lcd_write_data(lcd_init_sequence[i+1]);
                break;
            case CMD16:
                s5l_lcd_write_wcmd(lcd_init_sequence[i+1]);
                break;
            case DATA16:
                s5l_lcd_write_wdata(lcd_init_sequence[i+1]);
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
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x232);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3, 0x1137); 
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x201);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3, 0x137);
        s5l_lcd_write_cmd_data(R_DISPLAY_CONTROL_1, 0x200);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_1, 0x680);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_2, 0x160);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_3, 0x127);
        s5l_lcd_write_cmd_data(R_POWER_CONTROL_1, 0x600);
    }
    else
    {
        s5l_lcd_write_cmd(R_DISPLAY_OFF);
        s5l_lcd_write_wdata(0);
        s5l_lcd_write_wdata(0);
        s5l_lcd_write_cmd(R_SLEEP_IN);
        s5l_lcd_write_wdata(0);
        s5l_lcd_write_wdata(0);
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

    if (((PDAT13 & 1) == 0) && ((PDAT14 & 2) == 2))
        lcd_type = 0;  /* Similar to ILI9320 - aka "type 2" */
    else
        lcd_type = 1;  /* Similar to LDS176 - aka "type 7" */

    lcd_ispowered = true;
}

/*** Update functions ***/

static inline void lcd_write_pixel(fb_data pixel)
{
    LCD_WDATA = pixel >> 8;
    LCD_WDATA = pixel; /* no need to &0xff here, only lower 8 bit used */
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    lcd_update_rect(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int y0, x0, y1, x1;
    fb_data* p;
    
    width = (width + 1) & ~1;       /* ensure width is even */

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
        s5l_lcd_write_wdata(x0);            /* Start column */
        s5l_lcd_write_wdata(x1);            /* End column */

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_wdata(y0);            /* Start row */
        s5l_lcd_write_wdata(y1);            /* End row */

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }

    /* Copy display bitmap to hardware */
    p = &lcd_framebuffer[y0][x0];
    if (LCD_WIDTH == width)
    {
        x1 = height*LCD_WIDTH/4;
        do {
            while (LCD_STATUS & 0x08); /* wait while FIFO is half full */
            lcd_write_pixel(*(p++));
            lcd_write_pixel(*(p++));
            lcd_write_pixel(*(p++));
            lcd_write_pixel(*(p++));
        } while (--x1 > 0);
    } else {
        y1 = height;
        do {
            x1 = width/2; /* width is forced to even to allow speed up */
            do {
                while (LCD_STATUS & 0x08); /* wait while FIFO is half full */
                lcd_write_pixel(*(p++));
                lcd_write_pixel(*(p++));
            } while (--x1 > 0 );
            p += LCD_WIDTH - width;
        } while (--y1 > 0 );
    }
}

/*** update functions ***/

#define CSUB_X 2
#define CSUB_Y 2

/*   YUV- > RGB565 conversion
 *   |R|   |1.000000 -0.000001  1.402000| |Y'|
 *   |G| = |1.000000 -0.334136 -0.714136| |Pb|
 *   |B|   |1.000000  1.772000  0.000000| |Pr|
 *   Scaled, normalized, rounded and tweaked to yield RGB 565:
 *   |R|   |74   0 101| |Y' -  16| >> 9
 *   |G| = |74 -24 -51| |Cb - 128| >> 8
 *   |B|   |74 128   0| |Cr - 128| >> 9
*/

#define RGBYFAC   74   /*  1.0      */
#define RVFAC    101   /*  1.402    */
#define GVFAC   (-51)  /* -0.714136 */
#define GUFAC   (-24)  /* -0.334136 */
#define BUFAC    128   /*  1.772    */

/* ROUNDOFFS contain constant for correct round-offs as well as
   constant parts of the conversion matrix (e.g. (Y'-16)*RGBYFAC
   -> constant part = -16*RGBYFAC). Through extraction of these
   constant parts we save at leat 4 substractions in the conversion
   loop */
#define ROUNDOFFSR (256 - 16*RGBYFAC - 128*RVFAC)
#define ROUNDOFFSG (128 - 16*RGBYFAC - 128*GVFAC - 128*GUFAC)
#define ROUNDOFFSB (256 - 16*RGBYFAC             - 128*BUFAC)

#define MAX_5BIT 0x1f
#define MAX_6BIT 0x3f

/* Performance function to blit a YUV bitmap directly to the LCD */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    int h;
    int y0, x0, y1, x1;

    width = (width + 1) & ~1;

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
        s5l_lcd_write_wdata(x0);            /* Start column */
        s5l_lcd_write_wdata(x1);            /* End column */

        s5l_lcd_write_cmd(R_ROW_ADDR_SET);
        s5l_lcd_write_wdata(y0);            /* Start row */
        s5l_lcd_write_wdata(y1);            /* End row */

        s5l_lcd_write_cmd(R_MEMORY_WRITE);
    }

    const int stride_div_csub_x = stride/CSUB_X;

    h = height;
    while (h > 0) {
        /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */
        const unsigned char *ysrc = src[0] + stride * src_y + src_x;

        const int uvoffset = stride_div_csub_x * (src_y/CSUB_Y) +
                             (src_x/CSUB_X);

        const unsigned char *usrc = src[1] + uvoffset;
        const unsigned char *vsrc = src[2] + uvoffset;
        const unsigned char *row_end = ysrc + width;

        int yp, up, vp;
        int red1, green1, blue1;
        int red2, green2, blue2;

        int rc, gc, bc;

        do
        {
            up = *usrc++;
            vp = *vsrc++;
            rc = RVFAC * vp              + ROUNDOFFSR;
            gc = GVFAC * vp + GUFAC * up + ROUNDOFFSG;
            bc =              BUFAC * up + ROUNDOFFSB;
            
            /* Pixel 1 -> RGB565 */
            yp = *ysrc++ * RGBYFAC;
            red1   = (yp + rc) >> 9;
            green1 = (yp + gc) >> 8;
            blue1  = (yp + bc) >> 9;

            /* Pixel 2 -> RGB565 */
            yp = *ysrc++ * RGBYFAC;
            red2   = (yp + rc) >> 9;
            green2 = (yp + gc) >> 8;
            blue2  = (yp + bc) >> 9;

            /* Since out of bounds errors are relatively rare, we check two
               pixels at once to see if any components are out of bounds, and
               then fix whichever is broken. This works due to high values and
               negative values both being !=0 when bitmasking them.
               We first check for red and blue components (5bit range). */
            if ((red1 | blue1 | red2 | blue2) & ~MAX_5BIT)
            {
                if (red1  & ~MAX_5BIT)
                    red1  = (red1  >> 31) ? 0 : MAX_5BIT;
                if (blue1 & ~MAX_5BIT)
                    blue1 = (blue1 >> 31) ? 0 : MAX_5BIT;
                if (red2  & ~MAX_5BIT)
                    red2  = (red2  >> 31) ? 0 : MAX_5BIT;
                if (blue2 & ~MAX_5BIT)
                    blue2 = (blue2 >> 31) ? 0 : MAX_5BIT;
            }
            /* We second check for green component (6bit range) */
            if ((green1 | green2) & ~MAX_6BIT)
            {
                if (green1 & ~MAX_6BIT)
                    green1 = (green1 >> 31) ? 0 : MAX_6BIT;
                if (green2 & ~MAX_6BIT)
                    green2 = (green2 >> 31) ? 0 : MAX_6BIT;
            }

            /* output 2 pixels */
            while (LCD_STATUS & 0x08); /* wait while FIFO is half full */
            lcd_write_pixel((red1 << 11) | (green1 << 5) | blue1);
            lcd_write_pixel((red2 << 11) | (green2 << 5) | blue2);
        }
        while (ysrc < row_end);

        src_y++;
        h--;
    }
}

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

/*** definitions ***/


#define SPIDELAY(_x) void(_x);

#define SETMOSI() (PDAT4 |= (1 << 3))
#define CLRMOSI() (PDAT4 &= ~(1 << 3))

#define MISO ((PDAT1 >> 1) & 1)

#define SETCLK() (PDAT4 |= (1 << 2))
#define CLRCLK() (PDAT4 &= ~(1 << 2))

#define SETSS() (PDAT7 |= (1 << 1))
#define CLRSS() (PDAT7 &= ~(1 << 1))

void init_lcd_spi(void)
{
    int oldval;

    oldval = PCON1;
    //Set P1.1 to input
    PCON1 = ((oldval & ~(0xf << 4)));

    oldval = PCON4;
    //Set P4.2 and 4.3 to output
    PCON4 = ((oldval & ~(0xf << 8 | 0xf << 12)) | (1 << 8 | 1 << 12));

    oldval = PCON4;
    //Set P4.2 and 4.3 to output
    PCON4 = ((oldval & ~(0xf << 8 | 0xf << 12)) | (1 << 8 | 1 << 12));

    oldval = PCON7;
    //Set P7.0 and 7.1 to output
    PCON7 = ((oldval & ~(0xf << 0 | 0xf << 4)) | (1 << 0 | 1 << 4));

    SETSS();
    SETCLK();
}

static inline void delay(int duration)
{
    volatile int i;
    for(i=0;i<duration;i++);
}

unsigned int lcd_spi_io(unsigned int output,unsigned int bits,unsigned int inskip)
{
    unsigned int input = 0;
    unsigned int i;

    //delay(10);  // < 470 us

    CLRSS();
    //delay(13); // > 22 us

    for (i = 0; i < bits+inskip; i++) {

        CLRCLK();

        if(i<bits)
        {
            if (output & (1 << (bits-1)))
                SETMOSI();
            else
                CLRMOSI();
            output <<= 1;
        }
        else
        {
            CLRMOSI();
        }

        //delay(20); // >> 6.7 us

        SETCLK();

        if(i>=inskip)
        {
            input <<= 1;
            input |= MISO;
        }

        //delay(20); // >> 6.7 us
    }

    SETSS();

    return (input);
}


void spi_set_reg(unsigned char reg,unsigned short value)
{
    lcd_spi_io(0x700000|reg,24,0); // possibly 0x74
    lcd_spi_io(0x720000|value,24,0); // possibly 0x77
}

unsigned int lcd_read_id(void)
{
    unsigned int device_code;
    lcd_spi_io(0x700000,24,0); // possibly 0x74

    device_code=lcd_spi_io(0x7300,16,24); // possibly 0x77
    return device_code;
}

/** globals **/

static int xoffset; /* needed for flip */

/*** hardware configuration ***/

int lcd_default_contrast(void)
{
    return 0x1f;
}

void lcd_set_contrast(int val)
{
}

void lcd_set_invert_display(bool yesno)
{
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


/* LCD init */
void lcd_init_device(void)
{
}

/*** Update functions ***/

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    /* Copy display bitmap to hardware */
    while (bheight--)
    {
    }
}


/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase_blit(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    (void)values;
    (void)phases;
    (void)x;
    (void)by;
    (void)width;
    (void)bheight;
    (void)stride;
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
    int y;

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_FBHEIGHT; y++)
    {
    }
}

/* Update a fraction of the display. */
void lcd_update_rect(int, int, int, int) ICODE_ATTR;
void lcd_update_rect(int x, int y, int width, int height)
{
    int ymax;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 3;
    y >>= 3;

    if(x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_FBHEIGHT)
        ymax = LCD_FBHEIGHT-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
    }
}

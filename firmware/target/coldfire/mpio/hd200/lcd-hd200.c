/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2010 Marcin Bukat
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

#include "system.h"
#include "kernel.h"
#include "lcd.h"

/*** definitions ***/
/* TOMATO LSI 0350 - definitions and slightly tweaked functions
 * taken from lcd-remote-iaudio.c 
 */

#define LCD_SET_DUTY_RATIO 0x48
#define LCD_SELECT_ADC     0xa0
#define LCD_SELECT_SHL     0xc0
#define LCD_SET_COM0       0x44
#define LCD_OSC_ON         0xab
#define LCD_SELECT_DCDC    0x64
#define LCD_SELECT_RES     0x20
#define LCD_SET_VOLUME     0x81
#define LCD_SET_BIAS       0x50
#define LCD_CONTROL_POWER  0x28
#define LCD_DISPLAY_ON     0xae
#define LCD_SET_INITLINE   0x40
#define LCD_SET_COLUMN     0x10
#define LCD_SET_PAGE       0xb0
#define LCD_SET_GRAY       0x88
#define LCD_SET_PWM_FRC    0x90
#define LCD_SET_POWER_SAVE 0xa8
#define LCD_REVERSE        0xa6
#define LCD_RESET	   0xe2

/* cached settings */
static bool cached_invert = false;
static bool cached_flip = false;
static int cached_contrast = DEFAULT_CONTRAST_SETTING;

/*** hardware configuration ***/
int lcd_default_contrast(void)
{
    return DEFAULT_CONTRAST_SETTING;
}

void lcd_powersave(bool on)
{
/* What is the point of having else construct here? */
    if (on)
        lcd_write_command(LCD_SET_POWER_SAVE | 1);
    else
        lcd_write_command(LCD_SET_POWER_SAVE | 1);
}

void lcd_set_contrast(int val)
{
    if (val < MIN_CONTRAST_SETTING)
        val = MIN_CONTRAST_SETTING;
    else if (val > MAX_CONTRAST_SETTING)
        val = MAX_CONTRAST_SETTING;

    cached_contrast = val;
    lcd_write_command_e(LCD_SET_VOLUME, val);
}

void lcd_set_invert_display(bool yesno)
{
    cached_invert = yesno;
    lcd_write_command(LCD_REVERSE | yesno);

}

/* turn the display upside down (call lcd_update() afterwards) */
void lcd_set_flip(bool yesno)
{
    cached_flip = yesno;
    if(yesno)
    {
        lcd_write_command(LCD_SELECT_ADC | 1);
        lcd_write_command(LCD_SELECT_SHL | 0);
        lcd_write_command_e(LCD_SET_COM0, 0);
    }
    else
    {
        lcd_write_command(LCD_SELECT_ADC | 0);
        lcd_write_command(LCD_SELECT_SHL | 8);
        lcd_write_command_e(LCD_SET_COM0, 0);
    }
}

void lcd_shutdown(void)
{
    /* Set power save -> Power OFF (VDD - VSS) .. that's it */
    lcd_write_command(LCD_SET_POWER_SAVE | 1);
}

void lcd_init_device(void)
{
    and_l(~0x00000800, &GPIO_FUNCTION); /* CS3 line */ 

    /* LCD Reset GPO34 */
    or_l(0x00000004, &GPIO1_ENABLE);	/* set as output */
    or_l(0x00000004, &GPIO1_FUNCTION);  /* switch to secondary function - GPIO */
   
    and_l(~0x00000004, &GPIO1_OUT);	/* RESET low */
    sleep(1);				/* delay at least 1000 ns */
    or_l(0x00000004, &GPIO1_OUT);	/* RESET high */
    sleep(1);

    /* parameters setup taken from original firmware */
    lcd_write_command(LCD_RESET);
    lcd_write_command_e(LCD_SET_DUTY_RATIO,0x80); /* 1/128 */
    lcd_write_command(LCD_OSC_ON);
    lcd_write_command(LCD_SELECT_DCDC | 3); /* DC/DC 6xboost */
    lcd_write_command(LCD_SELECT_RES | 7); /* Regulator resistor: 7.2 */
    lcd_write_command(LCD_SET_BIAS | 6); /* 1/11 */
    lcd_write_command(LCD_SET_PWM_FRC | 6); /* 3FRC + 12PWM */
    lcd_write_command_e(LCD_SET_GRAY | 0, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 1, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 2, 0x0c);
    lcd_write_command_e(LCD_SET_GRAY | 3, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 4, 0xc4);
    lcd_write_command_e(LCD_SET_GRAY | 5, 0x00);
    lcd_write_command_e(LCD_SET_GRAY | 6, 0xcc);
    lcd_write_command_e(LCD_SET_GRAY | 7, 0x00);

    lcd_write_command(LCD_CONTROL_POWER | 7); /* All circuits ON */
    lcd_write_command(LCD_DISPLAY_ON | 1); /* display on */

    lcd_set_flip(cached_flip);
    lcd_set_contrast(cached_contrast);
    lcd_set_invert_display(cached_invert);

    lcd_update();
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_update(void) ICODE_ATTR;
void lcd_update(void)
{
   int y;
    
        for(y = 0;y < LCD_FBHEIGHT;y++)
        {
            lcd_write_command(LCD_SET_PAGE | y);
            lcd_write_command_e(LCD_SET_COLUMN, 0);
            lcd_write_data(lcd_framebuffer[y], LCD_WIDTH);
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

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;

    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */

    if (ymax >= LCD_FBHEIGHT)
        ymax = LCD_FBHEIGHT-1;

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_write_command(LCD_SET_PAGE | y ); 
        lcd_write_command_e(LCD_SET_COLUMN | ((x >> 4) & 0xf), x & 0x0f);
        lcd_write_data (&lcd_framebuffer[y][x], width);
    }
    
}

/* Helper function. */
void lcd_mono_data(const unsigned char *data, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_mono(const unsigned char *data, int x, int by, int width,
                   int bheight, int stride)
{
    while (bheight--)
    {
        lcd_write_command(LCD_SET_PAGE | (by & 0xf));
        lcd_write_command_e(LCD_SET_COLUMN | ((x >> 4) & 0xf), x & 0xf);

        lcd_mono_data(data, width);
        data += stride;
        by++;
    }
}

/* Helper function for lcd_grey_phase_blit(). */
void lcd_grey_data(unsigned char *values, unsigned char *phases, int count);

/* Performance function that works with an external buffer
   note that by and bheight are in 8-pixel units! */
void lcd_blit_grey_phase(unsigned char *values, unsigned char *phases,
                         int x, int by, int width, int bheight, int stride)
{
    stride <<= 3; /* 8 pixels per block */

    while (bheight--)
    {
        lcd_write_command(LCD_SET_PAGE | (by & 0xf));
        lcd_write_command_e(LCD_SET_COLUMN | ((x >> 4) & 0xf), x & 0xf);

        lcd_grey_data(values, phases, width);
        values += stride;
        phases += stride;
        by++;
    }
}


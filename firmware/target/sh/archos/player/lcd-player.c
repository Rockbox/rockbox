/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 * Based on the work of Alan Korr, Kjell Ericson and others
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
#include <string.h>
#include "hwcompat.h"
#include "system.h"
#include "lcd.h"
#include "lcd-charcell.h"

#define OLD_LCD_DDRAM        ((char)0xB0) /* Display data (characters) */
#define OLD_LCD_CGRAM        ((char)0x80) /* Character generator (patterns) */
#define OLD_LCD_ICONRAM      ((char)0xE0)
#define OLD_LCD_CONTRAST_SET ((char)0xA8)
#define OLD_LCD_NOP          ((char)0x00)
#define OLD_LCD_SYSTEM_SET                 ((char)0x60)
#define OLD_LCD_SET_POWER_SAVE_OSC_CONTROL ((char)0x40)
#define OLD_LCD_SET_POWER_CONTROL          ((char)0x50)
#define OLD_LCD_SET_DISPLAY_CONTROL        ((char)0x30)

#define NEW_LCD_DDRAM        ((char)0x80) /* Display data (characters) */
#define NEW_LCD_CGRAM        ((char)0xC0) /* Character generator (patterns) */
#define NEW_LCD_ICONRAM      ((char)0x40)
#define NEW_LCD_CONTRAST_SET ((char)0x50)
#define NEW_LCD_NOP          ((char)0x00)
#define NEW_LCD_FUNCTION_SET               ((char)0x10)
#define NEW_LCD_SET_POWER_SAVE_OSC_CONTROL ((char)0x0c)
#define NEW_LCD_SET_POWER_CONTROL_REG      ((char)0x20)
#define NEW_LCD_SET_DISPLAY_CONTROL        ((char)0x28)
#define NEW_LCD_SET_DOUBLE_HEIGHT          ((char)0x08)

#define LCD_CURSOR(x,y)      ((char)(lcd_ddram+((y)*16+(x))))
#define LCD_ICON(i)          ((char)(lcd_iconram+i))

static bool new_lcd;
static char lcd_contrast_set;
static char lcd_ddram;
static char lcd_cgram;
static char lcd_iconram;

/* hardware configuration */

int lcd_default_contrast(void)
{
    return 30;
}

void lcd_set_contrast(int val)
{
    lcd_write_command_e(lcd_contrast_set, 31 - val);
}

/* charcell specific */

void lcd_double_height(bool on)
{
    if(new_lcd)
        lcd_write_command(on ? (NEW_LCD_SET_DOUBLE_HEIGHT|1)
                             : NEW_LCD_SET_DOUBLE_HEIGHT);
}

void lcd_icon(int icon, bool enable)
{
    static const struct {
        char pos;
        char mask;
    } icontab[] = {
        { 0, 0x02}, { 0, 0x08}, { 0, 0x04}, { 0, 0x10}, /* Battery */
        { 2, 0x04}, /* USB */
        { 3, 0x10}, /* Play */
        { 4, 0x10}, /* Record */
        { 5, 0x02}, /* Pause */
        { 5, 0x10}, /* Audio */
        { 6, 0x02}, /* Repeat */
        { 7, 0x01}, /* 1 */
        { 9, 0x04}, /* Volume */
        { 9, 0x02}, { 9, 0x01}, {10, 0x08}, {10, 0x04}, {10, 0x01}, /* Vol 1-5 */
        {10, 0x10}, /* Param */
    };
    static char icon_mirror[11] = {0};

    int pos, mask;

    pos = icontab[icon].pos;
    mask = icontab[icon].mask;
      
    if (enable)
        icon_mirror[pos] |= mask;
    else
        icon_mirror[pos] &= ~mask;
    
    lcd_write_command_e(LCD_ICON(pos), icon_mirror[pos]);
}

/* device specific init */
void lcd_init_device(void)
{
    unsigned char data_vector[64];

    /* LCD init for cold start */
    PBCR2 &= 0xff00;                    /* Set PB0..PB3 to GPIO */
    or_b(0x0f, &PBDRL);                 /* ... high */
    or_b(0x0f, &PBIORL);                /* ... and output */

    new_lcd = is_new_player();

    if (new_lcd)
    {
        lcd_contrast_set = NEW_LCD_CONTRAST_SET;
        lcd_ddram = NEW_LCD_DDRAM;
        lcd_cgram = NEW_LCD_CGRAM;
        lcd_iconram = NEW_LCD_ICONRAM;

        lcd_write_command(NEW_LCD_FUNCTION_SET|1); /* CGRAM selected */
        lcd_write_command_e(NEW_LCD_CONTRAST_SET, 0x08);
        lcd_write_command(NEW_LCD_SET_POWER_SAVE_OSC_CONTROL|2);
                                            /* oscillator on */
        lcd_write_command(NEW_LCD_SET_POWER_CONTROL_REG|7);
        /* opamp buffer + voltage booster on */

        memset(data_vector, 0x20, 64);
        lcd_write_command(NEW_LCD_DDRAM);   /* Set DDRAM address */
        lcd_write_data(data_vector, 64);    /* all spaces */

        memset(data_vector, 0, 64);
        lcd_write_command(NEW_LCD_CGRAM);   /* Set CGRAM address */
        lcd_write_data(data_vector, 64);    /* zero out */
        lcd_write_command(NEW_LCD_ICONRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 16);    /* zero out */

        lcd_write_command(NEW_LCD_SET_DISPLAY_CONTROL|1); /* display on */
    }
    else
    {
        lcd_contrast_set = OLD_LCD_CONTRAST_SET;
        lcd_ddram = OLD_LCD_DDRAM;
        lcd_cgram = OLD_LCD_CGRAM;
        lcd_iconram = OLD_LCD_ICONRAM;

        lcd_write_command(OLD_LCD_NOP);
        lcd_write_command(OLD_LCD_SYSTEM_SET|1);  /* CGRAM selected */
        lcd_write_command(OLD_LCD_SET_POWER_SAVE_OSC_CONTROL|2); 
                                            /* oscillator on */
        lcd_write_command(OLD_LCD_SET_POWER_CONTROL|7);
        /* voltage regulator, voltage follower and booster on */

        memset(data_vector, 0x24, 13);
        lcd_write_command(OLD_LCD_DDRAM);   /* Set DDRAM address */
        lcd_write_data(data_vector, 13);    /* all spaces */
        lcd_write_command(OLD_LCD_DDRAM + 0x10);
        lcd_write_data(data_vector, 13);
        lcd_write_command(OLD_LCD_DDRAM + 0x20);
        lcd_write_data(data_vector, 13);

        memset(data_vector, 0, 32);
        lcd_write_command(OLD_LCD_CGRAM);   /* Set CGRAM address */
        lcd_write_data(data_vector, 32);    /* zero out */
        lcd_write_command(OLD_LCD_ICONRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 13);    /* zero out */
        lcd_write_command(OLD_LCD_ICONRAM + 0x10);
        lcd_write_data(data_vector, 13);

        sleep(HZ/10);
        lcd_write_command(OLD_LCD_SET_DISPLAY_CONTROL|1); /* display on */
    }
    lcd_set_contrast(lcd_default_contrast());
}

/*** Update functions ***/

void lcd_update(void)
{
    int y;
    
    for (y = 0; y < lcd_pattern_count; y++)
    {
        if (lcd_patterns[y].count > 0)
        {
            lcd_write_command(lcd_cgram | (y << 3));
            lcd_write_data(lcd_patterns[y].pattern, 7);
        }
    }
    for (y = 0; y < LCD_HEIGHT; y++)
    {
        lcd_write_command(LCD_CURSOR(0, y));
        lcd_write_data(lcd_charbuffer[y], LCD_WIDTH);
    }
    if (lcd_cursor.visible)
    {
        lcd_write_command_e(LCD_CURSOR(lcd_cursor.x, lcd_cursor.y),
                            lcd_cursor.hw_char);
    }
}

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-player.c 12835 2007-03-18 17:58:49Z amiconn $
 *
 * Copyright (C) 2007 by Jens Arnold
 * Based on the work of Alan Korr, Kjell Ericson and others
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

#define OLD_LCD_CRAM         ((char)0xB0) /* Characters */
#define OLD_LCD_PRAM         ((char)0x80) /*  Patterns  */
#define OLD_LCD_IRAM         ((char)0xE0) /*    Icons   */
#define OLD_LCD_CONTRAST_SET ((char)0xA8)

#define NEW_LCD_CRAM         ((char)0x80) /* Characters */
#define NEW_LCD_PRAM         ((char)0xC0) /*  Patterns  */
#define NEW_LCD_IRAM         ((char)0x40) /*    Icons   */
#define NEW_LCD_CONTRAST_SET ((char)0x50)
#define NEW_LCD_FUNCTION_SET                    ((char)0x10)
#define NEW_LCD_POWER_SAVE_MODE_OSC_CONTROL_SET ((char)0x0c)
#define NEW_LCD_POWER_CONTROL_REGISTER_SET      ((char)0x20)
#define NEW_LCD_DISPLAY_CONTROL_SET             ((char)0x28)
#define NEW_LCD_SET_DOUBLE_HEIGHT               ((char)0x08)

#define LCD_CURSOR(x,y)      ((char)(lcd_cram+((y)*16+(x))))
#define LCD_ICON(i)          ((char)(lcd_iram+i))

static bool new_lcd;
static char lcd_contrast_set;
static char lcd_cram;
static char lcd_pram;
static char lcd_iram;

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

    new_lcd = is_new_player();

    if (new_lcd)
    {
        lcd_contrast_set = NEW_LCD_CONTRAST_SET;
        lcd_cram = NEW_LCD_CRAM;
        lcd_pram = NEW_LCD_PRAM;
        lcd_iram = NEW_LCD_IRAM;

        /* LCD init for cold start */
        PBCR2 &= 0xff00;                 /* Set PB0..PB3 to GPIO */
        or_b(0x0f, &PBDRL);              /* ... high */
        or_b(0x0f, &PBIORL);             /* ... and output */

        lcd_write_command(NEW_LCD_FUNCTION_SET + 1); /* CGRAM selected */
        lcd_write_command_e(NEW_LCD_CONTRAST_SET, 0x08);
        lcd_write_command(NEW_LCD_POWER_SAVE_MODE_OSC_CONTROL_SET + 2);
                                         /* oscillator on */
        lcd_write_command(NEW_LCD_POWER_CONTROL_REGISTER_SET + 7);
                                         /* opamp buffer + voltage booster on*/

        memset(data_vector, 0x20, 64);
        lcd_write_command(NEW_LCD_CRAM); /* Set DDRAM address */
        lcd_write_data(data_vector, 64); /* all spaces */

        memset(data_vector, 0, 64);
        lcd_write_command(NEW_LCD_PRAM); /* Set CGRAM address */
        lcd_write_data(data_vector, 64); /* zero out */
        lcd_write_command(NEW_LCD_IRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 16); /* zero out */

        lcd_write_command(NEW_LCD_DISPLAY_CONTROL_SET + 1); /* display on */
    }
    else
    {
        lcd_contrast_set = OLD_LCD_CONTRAST_SET;
        lcd_cram = OLD_LCD_CRAM;
        lcd_pram = OLD_LCD_PRAM;
        lcd_iram = OLD_LCD_IRAM;

#if 1
        /* LCD init for cold start */
        PBCR2 &= 0xff00;                 /* Set PB0..PB3 to GPIO */
        or_b(0x0f, &PBDRL);              /* ... high */
        or_b(0x0f, &PBIORL);             /* ... and output */

        lcd_write_command(0x61);
        lcd_write_command(0x42);
        lcd_write_command(0x57);

        memset(data_vector, 0x24, 13);
        lcd_write_command(OLD_LCD_CRAM); /* Set DDRAM address */
        lcd_write_data(data_vector, 13); /* all spaces */
        lcd_write_command(OLD_LCD_CRAM + 0x10);
        lcd_write_data(data_vector, 13);
        lcd_write_command(OLD_LCD_CRAM + 0x20);
        lcd_write_data(data_vector, 13);

        memset(data_vector, 0, 32);
        lcd_write_command(OLD_LCD_PRAM); /* Set CGRAM address */
        lcd_write_data(data_vector, 32); /* zero out */
        lcd_write_command(OLD_LCD_IRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 13); /* zero out */
        lcd_write_command(OLD_LCD_IRAM + 0x10);
        lcd_write_data(data_vector, 13);

        lcd_write_command(0x31);
#else  
        /* archos look-alike code, left here for reference. As soon as the
         * rockbox version is confirmed working, this will go away */
        {
        int i;

        PBCR2 &= 0xc000;
        PBIOR |= 0x000f;
        PBDR |= 0x0002;
        PBDR |= 0x0001;
        PBDR |= 0x0004;
        PBDR |= 0x0008;

        for (i=0; i<200; i++) asm volatile ("nop"); /* wait 100 us */

        PBDR &= 0xfffd; /* CS low (assert) */

        for (i=0; i<100; i++) asm volatile ("nop"); /* wait 50 us */

        lcd_write_command(0x61);
        lcd_write_command(0x42);
        lcd_write_command(0x57);

        memset(data_vector, 0x24, 13);
        lcd_write_command(0xb0);         /* Set DDRAM address */
        lcd_write_data(data_vector, 13); /* all spaces */
        lcd_write_command(0xc0);
        lcd_write_data(data_vector, 13);
        lcd_write_command(0xd0);
        lcd_write_data(data_vector, 13);

        memset(data_vector, 0, 32);
        lcd_write_command(0x80);         /* Set CGRAM address */
        lcd_write_data(data_vector, 32); /* zero out */
        lcd_write_command(0xe0);         /* Set ICONRAM address */
        lcd_write_data(data_vector, 13); /* zero out */
        lcd_write_command(0xf0);
        lcd_write_data(data_vector, 13);

        for (i=0; i<300000; i++) asm volatile ("nop"); /* wait 150 ms */

        lcd_write_command(0x31);
        lcd_write_command_e(0xa8, 0);    /* Set contrast control */
        }
#endif
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
            lcd_write_command(lcd_pram | (y << 3));
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

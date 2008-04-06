/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Richard S. La Charité III
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "file.h"
#include "lcd-remote.h"
#include "adc.h"
#include "scroll_engine.h"

/*** definitions ***/

#define LCD_REMOTE_CNTL_ADC_NORMAL          0xa0
#define LCD_REMOTE_CNTL_ADC_REVERSE         0xa1
#define LCD_REMOTE_CNTL_SHL_NORMAL          0xc0
#define LCD_REMOTE_CNTL_SHL_REVERSE         0xc8
#define LCD_REMOTE_CNTL_DISPLAY_ON_OFF      0xae
#define LCD_REMOTE_CNTL_ENTIRE_ON_OFF       0xa4
#define LCD_REMOTE_CNTL_REVERSE_ON_OFF      0xa6
#define LCD_REMOTE_CNTL_NOP                 0xe3
#define LCD_REMOTE_CNTL_POWER_CONTROL       0x2b
#define LCD_REMOTE_CNTL_SELECT_REGULATOR    0x20
#define LCD_REMOTE_CNTL_SELECT_BIAS         0xa2
#define LCD_REMOTE_CNTL_SELECT_VOLTAGE      0x81
#define LCD_REMOTE_CNTL_INIT_LINE           0x40
#define LCD_REMOTE_CNTL_SET_PAGE_ADDRESS    0xB0

#define LCD_REMOTE_CNTL_HIGHCOL             0x10    /* Upper column address */
#define LCD_REMOTE_CNTL_LOWCOL              0x00    /* Lower column address */

#define CS_LO      and_l(~0x00000004, &GPIO1_OUT)
#define CS_HI      or_l(0x00000004, &GPIO1_OUT)
#define CLK_LO     and_l(~0x10000000, &GPIO_OUT)
#define CLK_HI     or_l(0x10000000, &GPIO_OUT)
#define DATA_LO    and_l(~0x00040000, &GPIO1_OUT)
#define DATA_HI    or_l(0x00040000, &GPIO1_OUT)
#define RS_LO      and_l(~0x00010000, &GPIO_OUT)
#define RS_HI      or_l(0x00010000, &GPIO_OUT)

static int xoffset; /* needed for flip */

/* timeout counter for deasserting /CS after access, <0 means not counting */
int remote_cs_countdown IDATA_ATTR = 0;
#define CS_TIMEOUT (HZ/10)

#ifdef HAVE_REMOTE_LCD_TICKING
/* If set to true, will prevent "ticking" to headphones. */
static bool emireduce = false;
int remote_byte_delay = 0; /* used in lcd-remote-as-iriver.S */
#endif

bool remote_initialized = false;
static int _remote_type = REMOTETYPE_UNPLUGGED;

/* cached settings values */
static bool cached_invert = false;
static bool cached_flip = false;
static int cached_contrast = DEFAULT_REMOTE_CONTRAST_SETTING;


/*** hardware configuration ***/

int lcd_remote_default_contrast(void)
{
    return DEFAULT_REMOTE_CONTRAST_SETTING;
}

#ifdef HAVE_REMOTE_LCD_TICKING
void lcd_remote_emireduce(bool state)
{
    emireduce = state;
}
#endif

void lcd_remote_powersave(bool on)
{
    if (remote_initialized)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | (on ? 0 : 1));
        lcd_remote_write_command(LCD_REMOTE_CNTL_ENTIRE_ON_OFF | (on ? 1 : 0));
    }
}

void lcd_remote_set_contrast(int val)
{
    cached_contrast = val;
    if (remote_initialized)
        lcd_remote_write_command_ex(LCD_REMOTE_CNTL_SELECT_VOLTAGE, val);
}

void lcd_remote_set_invert_display(bool yesno)
{
    cached_invert = yesno;
    if (remote_initialized)
        lcd_remote_write_command(LCD_REMOTE_CNTL_REVERSE_ON_OFF | (yesno?1:0));
}

/* turn the display upside down (call lcd_remote_update() afterwards) */
void lcd_remote_set_flip(bool yesno)
{
    cached_flip = yesno;
    if (yesno)
    {
        xoffset = 0;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_NORMAL);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_NORMAL);
        }
    }
    else
    {
        xoffset = 132 - LCD_REMOTE_WIDTH;
        if (remote_initialized)
        {
            lcd_remote_write_command(LCD_REMOTE_CNTL_ADC_REVERSE);
            lcd_remote_write_command(LCD_REMOTE_CNTL_SHL_REVERSE);
        }
    }
}

bool remote_detect(void)
{
    return (GPIO_READ & 0x40000000)?false:true;
}

int remote_type(void)
{
    return _remote_type;
}

void lcd_remote_on(void)
{
    CS_HI;
    CLK_LO;
    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_BIAS | 0x0);

    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x5);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x6);
    sleep(1);
    lcd_remote_write_command(LCD_REMOTE_CNTL_POWER_CONTROL | 0x7);

    lcd_remote_write_command(LCD_REMOTE_CNTL_SELECT_REGULATOR | 0x4); // 0x4 Select regulator @ 5.0 (default);

    sleep(1);

    lcd_remote_write_command(LCD_REMOTE_CNTL_INIT_LINE | 0x0); // init line
    lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | 0x0); // page address
    lcd_remote_write_command_ex(0x10, 0x00); // Column MSB + LSB

    lcd_remote_write_command(LCD_REMOTE_CNTL_DISPLAY_ON_OFF | 1);

    remote_initialized = true;

    lcd_remote_set_flip(cached_flip);
    lcd_remote_set_contrast(cached_contrast);
    lcd_remote_set_invert_display(cached_invert);
}

void lcd_remote_off(void)
{
    remote_initialized = false;
    CLK_LO;
    CS_HI;
}

#ifndef BOOTLOADER
/* Monitor remote hotswap */
static void remote_tick(void)
{
    static bool last_status = false;
    static int countdown = 0;
    static int init_delay = 0;
    bool current_status;
    int val;
    int level;

    current_status = remote_detect();
    /* Only report when the status has changed */
    if (current_status != last_status)
    {
        last_status = current_status;
        countdown = current_status ? 20*HZ : 1;
    }
    else
    {
        /* Count down until it gets negative */
        if (countdown >= 0)
            countdown--;

        if (current_status)
        {
            if (!(countdown % 8))
            {
                /* Determine which type of remote it is */
                level = disable_irq_save();
                val = adc_scan(ADC_REMOTEDETECT);
                restore_irq(level);

                if (val < ADCVAL_H100_LCD_REMOTE_HOLD)
                {
                    if (val < ADCVAL_H100_LCD_REMOTE)
                        if (val < ADCVAL_H300_LCD_REMOTE)
                            _remote_type = REMOTETYPE_H300_LCD;  /* hold off */
                        else
                            _remote_type = REMOTETYPE_H100_LCD;  /* hold off */
                    else
                        if (val < ADCVAL_H300_LCD_REMOTE_HOLD)
                            _remote_type = REMOTETYPE_H300_LCD;  /* hold on */
                        else
                            _remote_type = REMOTETYPE_H100_LCD;  /* hold on */

                    if (--init_delay <= 0)
                    {
                        queue_broadcast(SYS_REMOTE_PLUGGED, 0);
                        init_delay = 6;
                    }
                }
                else
                {
                    _remote_type = REMOTETYPE_H300_NONLCD; /* hold on or off */
                }
            }
        }
        else
        {
            if (countdown == 0)
            {
                _remote_type = REMOTETYPE_UNPLUGGED;

                queue_broadcast(SYS_REMOTE_UNPLUGGED, 0);
            }
        }
    }

    /* handle chip select timeout */
    if (remote_cs_countdown >= 0)
        remote_cs_countdown--;
    if (remote_cs_countdown == 0)
        CS_HI;
}
#endif

void lcd_remote_init_device(void)
{
#ifdef IRIVER_H300_SERIES
    or_l(0x10010000, &GPIO_FUNCTION); /* GPIO16: RS
                                         GPIO28: CLK */

    or_l(0x00040006, &GPIO1_FUNCTION); /* GPO33:  Backlight
                                          GPIO34: CS
                                          GPIO50: Data */
    or_l(0x10010000, &GPIO_ENABLE);
    or_l(0x00040006, &GPIO1_ENABLE);
#else
    or_l(0x10010800, &GPIO_FUNCTION); /* GPIO11: Backlight
                                         GPIO16: RS
                                         GPIO28: CLK */

    or_l(0x00040004, &GPIO1_FUNCTION); /* GPIO34: CS
                                          GPIO50: Data */
    or_l(0x10010800, &GPIO_ENABLE);
    or_l(0x00040004, &GPIO1_ENABLE);
#endif

    lcd_remote_clear_display();
    if (remote_detect())
        lcd_remote_on();
#ifndef BOOTLOADER
    tick_add_task(remote_tick);
#endif
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void)
{
    int y;

    if (!remote_initialized)
        return;

#ifdef HAVE_REMOTE_LCD_TICKING
    /* Adjust byte delay for emi reduction. */
    remote_byte_delay = emireduce ? cpu_frequency / 192800 - 100: 0;
#endif

    /* Copy display bitmap to hardware */
    for (y = 0; y < LCD_REMOTE_FBHEIGHT; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL | ((xoffset >> 4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL | (xoffset & 0xf));
        lcd_remote_write_data(lcd_remote_framebuffer[y], LCD_REMOTE_WIDTH);
    }
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int x, int y, int width, int height)
{
    int ymax;

    if (!remote_initialized)
        return;

    /* The Y coordinates have to work on even 8 pixel rows */
    ymax = (y + height-1) >> 3;
    y >>= 3;

    if(x + width > LCD_REMOTE_WIDTH)
        width = LCD_REMOTE_WIDTH - x;
    if (width <= 0)
        return; /* nothing left to do, 0 is harmful to lcd_write_data() */
    if(ymax >= LCD_REMOTE_FBHEIGHT)
        ymax = LCD_REMOTE_FBHEIGHT-1;

#ifdef HAVE_REMOTE_LCD_TICKING
    /* Adjust byte delay for emi reduction */
    remote_byte_delay = emireduce ? cpu_frequency / 192800 - 100: 0;
#endif

    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        lcd_remote_write_command(LCD_REMOTE_CNTL_SET_PAGE_ADDRESS | y);
        lcd_remote_write_command(LCD_REMOTE_CNTL_HIGHCOL | (((x+xoffset) >> 4) & 0xf));
        lcd_remote_write_command(LCD_REMOTE_CNTL_LOWCOL | ((x+xoffset) & 0xf));
        lcd_remote_write_data(&lcd_remote_framebuffer[y][x], width);
    }
}

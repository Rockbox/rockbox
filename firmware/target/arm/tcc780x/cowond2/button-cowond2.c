/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
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
#include "button.h"
#include "adc.h"
#include "pcf50606.h"

#define TOUCH_MARGIN 8

static enum touchpad_mode current_mode = TOUCHPAD_POINT;

static short last_x, last_y;
static bool touch_available = false;

static int touchpad_buttons[3][3] =
{
    {BUTTON_TOPLEFT,    BUTTON_TOPMIDDLE,    BUTTON_TOPRIGHT},
    {BUTTON_MIDLEFT,    BUTTON_CENTER,       BUTTON_MIDRIGHT},
    {BUTTON_BOTTOMLEFT, BUTTON_BOTTOMMIDDLE, BUTTON_BOTTOMRIGHT},
};

void touchpad_set_mode(enum touchpad_mode mode)
{
    current_mode = mode;
}

enum touchpad_mode touchpad_get_mode(void)
{
    return current_mode;
}

void button_set_touch_available(void)
{
    touch_available = true;
}

struct touch_calibration_point {
    short px_x; /* known pixel value */
    short px_y;
    short val_x; /* touchpad value at the known pixel */
    short val_y;
};

static struct touch_calibration_point topleft, bottomright;

static int touch_to_pixels(short val_x, short val_y)
{
    short x,y;

    x=val_x;
    y=val_y;

    x = (x-topleft.val_x)*(bottomright.px_x - topleft.px_x)
            / (bottomright.val_x - topleft.val_x) + topleft.px_x;

    y = (y-topleft.val_y)*(bottomright.px_y - topleft.px_y)
            / (bottomright.val_y - topleft.val_y) + topleft.px_y;

    if (x < 0)
        x = 0;
    else if (x>=LCD_WIDTH)
        x=LCD_WIDTH-1;

    if (y < 0)
        y = 0;
    else if (y>=LCD_HEIGHT)
        y=LCD_HEIGHT-1;

    return (x<<16)|y;
}

void button_init_device(void)
{
    /* Configure GPIOA 4 (POWER) and 8 (HOLD) for input */
    GPIOA_DIR &= ~0x110;
    
    /* Configure GPIOB 4 (button pressed) for input */
    GPIOB_DIR &= ~0x10;

    touch_available = false;

    /* Arbitrary touchscreen calibration */
    topleft.px_x = 0;
    topleft.px_y = 0;
    topleft.val_x = 50;
    topleft.val_y = 50;

    bottomright.px_x = LCD_WIDTH;
    bottomright.px_y = LCD_HEIGHT;
    bottomright.val_x = 980;
    bottomright.val_y = 980;
}

bool button_hold(void)
{
    return (GPIOA & 0x8) ? false : true;
}

int button_read_device(int *data)
{
    int btn = BUTTON_NONE;
    int adc;
    *data = 0;

    if (GPIOB & 0x4)
    {
        adc = adc_read(ADC_BUTTONS);

        /* The following contains some arbitrary, but working, guesswork */
        if (adc < 0x038) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x048) {
            btn |= (BUTTON_MINUS | BUTTON_PLUS);
        } else if (adc < 0x058) {
            btn |= (BUTTON_PLUS | BUTTON_MENU);
        } else if (adc < 0x070) {
            btn |= BUTTON_PLUS;
        } else if (adc < 0x090) {
            btn |= (BUTTON_MINUS  | BUTTON_MENU);
        } else if (adc < 0x150) {
            btn |= BUTTON_MINUS;
        } else if (adc < 0x200) {
            btn |= BUTTON_MENU;
        }
    }

    if (touch_available)
    {
        short x = 0, y = 0;
        static long last_touch = 0;
        bool send_touch = false;

        int irq_level = disable_irq_save();
        if (pcf50606_read(PCF5060X_ADCC1) & 0x80) /* Pen down */
        {
            unsigned char buf[3];
            pcf50606_write(PCF5060X_ADCC2, (0xE<<1) | 1); /* ADC start X+Y */
            pcf50606_read_multiple(PCF5060X_ADCS1, buf, 3);
            pcf50606_write(PCF5060X_ADCC2, 0);            /* ADC stop */

            x = (buf[0] << 2) | (buf[1] & 3);
            y = (buf[2] << 2) | ((buf[1] & 0xC) >> 2);

            if (TIME_BEFORE(last_touch + HZ/5, current_tick))
            {
                if ((x > last_x + TOUCH_MARGIN) ||
                    (x < last_x - TOUCH_MARGIN) ||
                    (y > last_y + TOUCH_MARGIN) ||
                    (y < last_y - TOUCH_MARGIN))
                {
                    send_touch = true;
                }
            }
            else
            {
                send_touch = true;
            }
        }
        restore_irq(irq_level);

        if (send_touch)
        {
            last_x = x;
            last_y = y;
            *data = touch_to_pixels(x, y);
            switch (current_mode)
            {
                case TOUCHPAD_POINT:
                    btn |= BUTTON_TOUCHPAD;
                    break;
                case TOUCHPAD_BUTTON:
                {
                    int px_x = (*data&0xffff0000)>>16;
                    int px_y = (*data&0x0000ffff);
                    btn |= touchpad_buttons[px_y/(LCD_HEIGHT/3)]
                                           [px_x/(LCD_WIDTH/3)];
                    break;
                }
            }
        }
        last_touch = current_tick;
        touch_available = false;
    }

    if (!(GPIOA & 0x4))
        btn |= BUTTON_POWER;

    return btn;
}

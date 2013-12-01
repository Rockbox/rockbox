/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#include "button-target.h"
#include "system.h"
#include "system-target.h"
#include "pinctrl-imx233.h"
#include "generic_i2c.h"
#include "synaptics-rmi.h"
#include "lcd.h"
#include "string.h"
#include "usb.h"
#include "button-imx233.h"
#include "touchpad.h"
#include "stdio.h"

struct imx233_button_map_t imx233_button_map[] =
{
    IMX233_BUTTON(VOL_DOWN, GPIO(1, 30), "vol_down", INVERTED),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON(VOL_UP, PSWITCH(3), "vol_up"),
    IMX233_BUTTON_(END, END(), "")
};

#ifndef BOOTLOADER

bool button_debug_screen(void)
{
    char product_id[RMI_PRODUCT_ID_LEN];
    rmi_read(RMI_PRODUCT_ID, RMI_PRODUCT_ID_LEN, product_id);
    uint8_t product_info[RMI_PRODUCT_INFO_LEN];
    rmi_read(RMI_PRODUCT_INFO, RMI_PRODUCT_INFO_LEN, product_info);
    char product_info_str[RMI_PRODUCT_INFO_LEN * 2 + 1];
    for(int i = 0; i < RMI_PRODUCT_INFO_LEN; i++)
        snprintf(product_info_str + 2 * i, 3, "%02x", product_info[i]);
    int x_max = rmi_read_single(RMI_2D_SENSOR_XMAX_MSB(0)) << 8 | rmi_read_single(RMI_2D_SENSOR_XMAX_LSB(0));
    int y_max = rmi_read_single(RMI_2D_SENSOR_YMAX_MSB(0)) << 8 | rmi_read_single(RMI_2D_SENSOR_YMAX_LSB(0));
    int sensor_resol = rmi_read_single(RMI_2D_SENSOR_RESOLUTION(0));
    int min_dist = rmi_read_single(RMI_2D_MIN_DIST);
    int gesture_settings = rmi_read_single(RMI_2D_GESTURE_SETTINGS);
    int volkeys_delay_counter = 0;
    union
    {
        unsigned char data;
        signed char value;
    }sensitivity;
    rmi_read(RMI_2D_SENSITIVITY_ADJ, 1, &sensitivity.data);

    /* Device to screen */
    int zone_w = LCD_WIDTH;
    int zone_h = (zone_w * y_max + x_max - 1) / x_max;
    int zone_x = 0;
    int zone_y = LCD_HEIGHT - zone_h;
    #define DX2SX(x) (((x) * zone_w ) / x_max)
    #define DY2SY(y) (zone_h - ((y) * zone_h ) / y_max)
    struct viewport report_vp;
    memset(&report_vp, 0, sizeof(report_vp));
    report_vp.x = zone_x;
    report_vp.y = zone_y;
    report_vp.width = zone_w;
    report_vp.height = zone_h;
    struct viewport gesture_vp;
    memset(&gesture_vp, 0, sizeof(gesture_vp));
    gesture_vp.x = LCD_WIDTH / 2;
    gesture_vp.y = zone_y - 80;
    gesture_vp.width = LCD_WIDTH / 2;
    gesture_vp.height = 80;

    while(1)
    {
        unsigned char sleep_mode = rmi_read_single(RMI_DEVICE_CONTROL) & RMI_SLEEP_MODE_BM;
        lcd_set_viewport(NULL);
        lcd_clear_display();
        int btns = button_read_device();
        lcd_putsf(0, 0, "button bitmap: %x", btns);
        lcd_putsf(0, 1, "RMI: id=%s info=%s", product_id, product_info_str);
        lcd_putsf(0, 2, "xmax=%d ymax=%d res=%d", x_max, y_max, sensor_resol);
        lcd_putsf(0, 3, "attn=%d ctl=%x int=%x",
            imx233_pinctrl_get_gpio(0, 27) ? 0 : 1,
            rmi_read_single(RMI_DEVICE_CONTROL),
            rmi_read_single(RMI_INTERRUPT_REQUEST));
        lcd_putsf(0, 4, "sensi: %d min_dist: %d", (int)sensitivity.value, min_dist);
        lcd_putsf(0, 5, "gesture: %x", gesture_settings);

        union
        {
            unsigned char data[10];
            struct
            {
                struct rmi_2d_absolute_data_t absolute;
                struct rmi_2d_relative_data_t relative;
                struct rmi_2d_gesture_data_t gesture;
            }s;
        }u;
        int absolute_x = u.s.absolute.x_msb << 8 | u.s.absolute.x_lsb;
        int absolute_y = u.s.absolute.y_msb << 8 | u.s.absolute.y_lsb;
        int nr_fingers = u.s.absolute.misc & 7;
        bool gesture = (u.s.absolute.misc & 8) == 8;
        int palm_width = u.s.absolute.misc >> 4;

        rmi_read(RMI_DATA_REGISTER(0), 10, u.data);
        lcd_putsf(0, 6, "abs: %d %d %d", absolute_x, absolute_y, (int)u.s.absolute.z);
        lcd_putsf(0, 7, "rel: %d %d", (int)u.s.relative.x, (int)u.s.relative.y);
        lcd_putsf(0, 8, "gesture: %x %x", u.s.gesture.misc, u.s.gesture.flick);
        lcd_putsf(0, 9, "misc: w=%d g=%d f=%d", palm_width, gesture, nr_fingers);
        lcd_putsf(30, 7, "sleep_mode: %d", 1 - sleep_mode);
        lcd_set_viewport(&report_vp);
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0, 0), LCD_BLACK);
        lcd_drawrect(0, 0, zone_w, zone_h);
        if(nr_fingers == 1)
        {
            lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0, 0, 0xff), LCD_BLACK);
            lcd_drawline(DX2SX(absolute_x) - u.s.relative.x,
                DY2SY(absolute_y) + u.s.relative.y,
                DX2SX(absolute_x), DY2SY(absolute_y));
            lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0, 0xff, 0), LCD_BLACK);
            lcd_fillrect(DX2SX(absolute_x) - 1, DY2SY(absolute_y) - 1, 3, 3);
        }
        lcd_set_viewport(&gesture_vp);
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0xff, 0), LCD_BLACK);
        if(u.s.gesture.misc & RMI_2D_GEST_MISC_CONFIRMED)
        {
            switch(u.s.gesture.misc & RMI_2D_GEST_MISC_TAP_CODE_BM)
            {
                case RMI_2D_GEST_MISC_NO_TAP: break;
                case RMI_2D_GEST_MISC_SINGLE_TAP:
                    lcd_putsf(0, 0, "TAP!");
                    break;
                case RMI_2D_GEST_MISC_DOUBLE_TAP:
                    lcd_putsf(0, 0, "DOUBLE TAP!");
                    break;
                case RMI_2D_GEST_MISC_TAP_AND_HOLD:
                    lcd_putsf(0, 0, "TAP & HOLD!");
                    break;
                default: break;
            }

            if(u.s.gesture.misc & RMI_2D_GEST_MISC_FLICK)
            {
                lcd_putsf(0, 1, "FLICK!");
                int flick_x = u.s.gesture.flick & RMI_2D_GEST_FLICK_X_BM;
                int flick_y = (u.s.gesture.flick & RMI_2D_GEST_FLICK_Y_BM) >> RMI_2D_GEST_FLICK_Y_BP;
                #define SIGN4EXT(a) \
                    if(a & 8) a = -((a ^ 0xf) + 1);
                SIGN4EXT(flick_x);
                SIGN4EXT(flick_y);

                int center_x = (LCD_WIDTH * 2) / 3;
                int center_y = 40;
                lcd_drawline(center_x, center_y, center_x + flick_x * 5, center_y - flick_y * 5);
            }
        }
        lcd_update();
        if(btns & BUTTON_POWER)
            break;
        if(btns & (BUTTON_VOL_DOWN|BUTTON_VOL_UP))
        {
            volkeys_delay_counter++;
            if(volkeys_delay_counter == 15)
            {
                if(btns & BUTTON_VOL_UP)
                    if(sleep_mode > RMI_SLEEP_MODE_FORCE_FULLY_AWAKE)
                        sleep_mode--;
                if(btns & BUTTON_VOL_DOWN)
                    if(sleep_mode < RMI_SLEEP_MODE_SENSOR_SLEEP)
                        sleep_mode++;
                rmi_set_sleep_mode(sleep_mode);
                volkeys_delay_counter = 0;
            }
        }

        yield();
    }

    return true;
}

/* we emulate a 3x3 grid, this gives the button mapping */
int button_mapping[3][3] =
{
    {BUTTON_BOTTOMLEFT, BUTTON_LEFT, BUTTON_BACK},
    {BUTTON_DOWN, BUTTON_SELECT, BUTTON_UP},
    {BUTTON_BOTTOMRIGHT, BUTTON_RIGHT, BUTTON_PLAYPAUSE},
};

#define RMI_INTERRUPT       1
#define RMI_SET_SENSITIVITY 2
#define RMI_SET_SLEEP_MODE  3
/* timeout before lowering touchpad power from lack of activity */
#define ACTIVITY_TMO        (5 * HZ)
#define TOUCHPAD_WIDTH      3010
#define TOUCHPAD_HEIGHT     1975
#define DEADZONE_MULTIPLIER 2 /* deadzone multiplier */

static int touchpad_btns = 0;
static long rmi_stack [DEFAULT_STACK_SIZE/sizeof(long)];
static const char rmi_thread_name[] = "rmi";
static struct event_queue rmi_queue;
static unsigned last_activity = 0;
static bool t_enable = true;
static int deadzone;

/* Ignore deadzone function. If outside of the pad, project to border. */
static int find_button_no_deadzone(int x, int y)
{
    /* compute grid coordinate */
    int gx = MAX(MIN(x * 3 / TOUCHPAD_WIDTH, 2), 0);
    int gy = MAX(MIN(y * 3 / TOUCHPAD_HEIGHT, 2), 0);

    return button_mapping[gx][gy];
}

static int find_button(int x, int y)
{
    /* find button ignoring deadzones */
    int btn = find_button_no_deadzone(x, y);
    /* To check if we are in a deadzone, we try to shift the coordinates
     * and see if we get the same button. Not that we do not want to apply
     * the deadzone in the borders ! The code works even in the borders because
     * the find_button_no_deadzone() project out-of-bound coordinates to the
     * borders */
    if(find_button_no_deadzone(x + deadzone, y) != btn ||
            find_button_no_deadzone(x - deadzone, y) != btn ||
            find_button_no_deadzone(x, y + deadzone) != btn ||
            find_button_no_deadzone(x, y - deadzone) != btn)
        return 0;
    return btn;
}

void touchpad_set_deadzone(int touchpad_deadzone)
{
    deadzone = touchpad_deadzone * DEADZONE_MULTIPLIER;
}

static int touchpad_read_device(void)
{
    return touchpad_btns;
}

static void rmi_attn_cb(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;
    /* the callback will not be fired until interrupt is enabled back so
     * the queue will not overflow or contain multiple RMI_INTERRUPT events */
    queue_post(&rmi_queue, RMI_INTERRUPT, 0);
}

static void do_interrupt(void)
{
    /* rmi_set_sleep_mode() does not do anything if the value
     * it is given is already the one setted */
    rmi_set_sleep_mode(RMI_SLEEP_MODE_LOW_POWER);
    last_activity = current_tick;
    /* clear interrupt */
    rmi_read_single(RMI_INTERRUPT_REQUEST);
    /* read data */
    union
    {
        unsigned char data[10];
        struct
        {
            struct rmi_2d_absolute_data_t absolute;
            struct rmi_2d_relative_data_t relative;
            struct rmi_2d_gesture_data_t gesture;
        }s;
    }u;
    rmi_read(RMI_DATA_REGISTER(0), 10, u.data);
    int absolute_x = u.s.absolute.x_msb << 8 | u.s.absolute.x_lsb;
    int absolute_y = u.s.absolute.y_msb << 8 | u.s.absolute.y_lsb;
    int nr_fingers = u.s.absolute.misc & 7;

    if(nr_fingers == 1)
        touchpad_btns = find_button(absolute_x, absolute_y);
    else
        touchpad_btns = 0;

    /* enable interrupt */
    imx233_pinctrl_setup_irq(0, 27, true, true, false, &rmi_attn_cb, 0);
}

void touchpad_enable_device(bool en)
{
    t_enable = en;
    queue_post(&rmi_queue, RMI_SET_SLEEP_MODE, en ? RMI_SLEEP_MODE_LOW_POWER : RMI_SLEEP_MODE_SENSOR_SLEEP);
}

void touchpad_set_sensitivity(int level)
{
    queue_post(&rmi_queue, RMI_SET_SENSITIVITY, level);
}

static void rmi_thread(void)
{
    struct queue_event ev;

    while(1)
    {
        /* make sure to timeout often enough for the activity timeout to take place */
        queue_wait_w_tmo(&rmi_queue, &ev, HZ);
        /* handle usb connect and ignore all messages except rmi interrupts */
        switch(ev.id)
        {
            case SYS_USB_CONNECTED:
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                break;
            case RMI_SET_SENSITIVITY:
                /* handle negative values as well ! */
                rmi_write_single(RMI_2D_SENSITIVITY_ADJ, (unsigned char)(int8_t)ev.data);
                break;
            case RMI_SET_SLEEP_MODE:
                /* reset activity */
                last_activity = current_tick;
                rmi_set_sleep_mode(ev.data);
                break;
            case RMI_INTERRUPT:
                do_interrupt();
                break;
            default:
                /* activity timeout */
                if(TIME_AFTER(current_tick, last_activity + ACTIVITY_TMO))
                {
                    /* don't change power mode if touchpad is disabled, it's already in sleep mode */
                    if(t_enable)
                        rmi_set_sleep_mode(RMI_SLEEP_MODE_VERY_LOW_POWER);
                }
                break;
        }
    }
}

static void touchpad_init(void)
{
    /* Synaptics TouchPad information:
     * - product id: 1533
     * - nr function: 1 (0x10 = 2D touchpad)
     * 2D Touchpad information (function 0x10)
     * - nr data sources: 3
     * - standard layout
     * - extra data registers: 7
     * - nr sensors: 1
     * 2D Touchpad Sensor #0 information:
     * - has relative data: yes
     * - has palm detect: yes
     * - has multi finger: yes
     * - has enhanced gesture: yes
     * - has scroller: no
     * - has 2D scrollers: no
     * - Maximum X: 3009
     * - Maxumum Y: 1974
     * - Resolution: 82
     */

    imx233_pinctrl_acquire(0, 26, "touchpad power");
    imx233_pinctrl_set_function(0, 26, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 26, false);
    imx233_pinctrl_set_drive(0, 26, PINCTRL_DRIVE_8mA);

    rmi_init(0x40);

    char product_id[RMI_PRODUCT_ID_LEN];
    rmi_read(RMI_PRODUCT_ID, RMI_PRODUCT_ID_LEN, product_id);
    /* The OF adjust the sensitivity based on product_id[1] compared to 2.
     * Since it doesn't to work great, just hardcode the sensitivity to
     * some reasonable value for now. */
    rmi_write_single(RMI_2D_SENSITIVITY_ADJ, 13);

    rmi_write_single(RMI_2D_GESTURE_SETTINGS,
        RMI_2D_GESTURE_PRESS_TIME_300MS |
        RMI_2D_GESTURE_FLICK_DIST_4MM << RMI_2D_GESTURE_FLICK_DIST_BP |
        RMI_2D_GESTURE_FLICK_TIME_700MS << RMI_2D_GESTURE_FLICK_TIME_BP);

    queue_init(&rmi_queue, true);
    create_thread(rmi_thread, rmi_stack, sizeof(rmi_stack), 0,
            rmi_thread_name IF_PRIO(, PRIORITY_USER_INTERFACE) IF_COP(, CPU));
    /* low power mode seems to be enough for normal use */
    rmi_set_sleep_mode(RMI_SLEEP_MODE_LOW_POWER);
    /* enable interrupt */
    imx233_pinctrl_acquire(0, 27, "touchpad int");
    imx233_pinctrl_set_function(0, 27, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 27, false);
    imx233_pinctrl_setup_irq(0, 27, true, true, false, &rmi_attn_cb, 0);
}

#else
int touchpad_read_device(void)
{
    return 0;
}
#endif

void button_init_device(void)
{
#ifndef BOOTLOADER
    touchpad_init();
#endif
    /* generic */
    imx233_button_init();
}

int button_read_device(void)
{
    return imx233_button_read(touchpad_filter(touchpad_read_device()));
}

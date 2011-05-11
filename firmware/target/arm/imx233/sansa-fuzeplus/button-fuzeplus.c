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

static void i2c_scl_dir(bool out)
{
    imx233_enable_gpio_output(0, 30, out);
}

static void i2c_sda_dir(bool out)
{
    imx233_enable_gpio_output(0, 31, out);
}

static void i2c_scl_out(bool high)
{
    imx233_set_gpio_output(0, 30, high);
}

static void i2c_sda_out(bool high)
{
    imx233_set_gpio_output(0, 31, high);
}

static bool i2c_scl_in(void)
{
    return imx233_get_gpio_input_mask(0, 1 << 30);
}

static bool i2c_sda_in(void)
{
    return imx233_get_gpio_input_mask(0, 1 << 31);
}

static void i2c_delay(int d)
{
    udelay(d);
}

struct i2c_interface btn_i2c =
{
    .scl_dir = i2c_scl_dir,
    .sda_dir = i2c_sda_dir,
    .scl_out = i2c_scl_out,
    .sda_out = i2c_sda_out,
    .scl_in = i2c_scl_in,
    .sda_in = i2c_sda_in,
    .delay = i2c_delay,
    .delay_hd_sta = 4,
    .delay_hd_dat = 5,
    .delay_su_dat = 1,
    .delay_su_sto = 4,
    .delay_su_sta = 5,
    .delay_thigh = 4
};

int rmi_i2c_bus = -1;

void button_debug_screen(void)
{
    char product_id[RMI_PRODUCT_ID_LEN];
    rmi_read(RMI_PRODUCT_ID, RMI_PRODUCT_ID_LEN, product_id);

    while(1)
    {
        lcd_clear_display();
        int btns = button_read_device();
        lcd_putsf(0, 0, "button bitmap: %x", btns);
        lcd_putsf(0, 1, "RMI: product=%s", product_id);
        lcd_putsf(0, 2, "touchpad presence: %x", rmi_read_single(RMI_FUNCTION_PRESENCE(RMI_2D_TOUCHPAD_FUNCTION)));
        lcd_putsf(0, 3, "sensor prop: %x", rmi_read_single(RMI_2D_SENSOR_PROP2(0)));
        int x_max = rmi_read_single(RMI_2D_SENSOR_XMAX_MSB(0)) << 8 | rmi_read_single(RMI_2D_SENSOR_XMAX_LSB(0));
        int y_max = rmi_read_single(RMI_2D_SENSOR_YMAX_MSB(0)) << 8 | rmi_read_single(RMI_2D_SENSOR_YMAX_LSB(0));
        lcd_putsf(0, 4, "xmax=%d ymax=%d res=%d", x_max, y_max,
            rmi_read_single(RMI_2D_SENSOR_RESOLUTION(0)));
        lcd_putsf(0, 5, "din0=%x", imx233_get_gpio_input_mask(0, 0x08000000));
        lcd_putsf(0, 6, "dev ctl: %x", rmi_read_single(RMI_DEVICE_CONTROL));
        lcd_putsf(0, 7, "int en: %x", rmi_read_single(RMI_INTERRUPT_ENABLE));
        lcd_putsf(0, 8, "int req: %x", rmi_read_single(RMI_INTERRUPT_REQUEST));
        union
        {
            unsigned char data[10];
            struct
            {
                unsigned char absolute_misc;
                unsigned char absolute_z;
                unsigned char absolute_x_msb;
                unsigned char absolute_x_lsb;
                unsigned char absolute_y_msb;
                unsigned char absolute_y_lsb;
                signed char relative_x;  /* signed */
                signed char relative_y; /* signed */
                unsigned char gesture_misc;
                unsigned char gesture_flick;
            } __attribute__((packed)) s;
        }u;
        int absolute_x = u.s.absolute_x_msb << 8 | u.s.absolute_x_lsb;
        int absolute_y = u.s.absolute_y_msb << 8 | u.s.absolute_y_lsb;
        rmi_read(RMI_DATA_REGISTER(0), 10, u.data);
        lcd_putsf(0, 9, "abs: %d %d", absolute_x, absolute_y);
        lcd_putsf(0, 10, "rel: %d %d", (int)u.s.relative_x, (int)u.s.relative_y);
        lcd_putsf(0, 11, "gesture: %x %x", u.s.gesture_misc, u.s.gesture_flick);
        
        lcd_update();
        
        if(btns & BUTTON_POWER)
            break;
        yield();
    }
}

void button_init_device(void)
{
    rmi_i2c_bus = i2c_add_node(&btn_i2c);
    rmi_init(rmi_i2c_bus, 0x40);

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
     *
     * ATTENTION line: B0P27 asserted low
     */
}

int button_read_device(void)
{
    int res = 0;
    if(!imx233_get_gpio_input_mask(1, 0x40000000))
        res |= BUTTON_VOL_DOWN;
    /* The imx233 uses the voltage on the PSWITCH pin to detect power up/down
     * events as well as recovery mode. Since the power button is the power button
     * and the volume up button is recovery, it is not possible to know whether
     * power button is down when volume up is down (except if there is another
     * method but volume up and power don't seem to be wired to GPIO pins). */
    switch((HW_POWER_STS & HW_POWER_STS__PSWITCH_BM) >> HW_POWER_STS__PSWITCH_BP)
    {
        case 1: res |= BUTTON_POWER; break;
        case 3: res |= BUTTON_VOL_UP; break;
        default: break;
    }
    return res;
}

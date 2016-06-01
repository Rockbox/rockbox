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
#include "i2c-imx233.h"
#include "synaptics-rmi.h"
#include "lcd.h"
#include "string.h"
#include "usb.h"
#include "button-imx233.h"
#include "touchpad.h"
#include "stdio.h"
#include "font.h"

struct imx233_button_map_t imx233_button_map[] =
{
    IMX233_BUTTON(VOL_DOWN, GPIO(1, 30), "vol_down", INVERTED),
    IMX233_BUTTON(POWER, PSWITCH(1), "power"),
    IMX233_BUTTON(VOL_UP, PSWITCH(3), "vol_up"),
    IMX233_BUTTON_(END, END(), "")
};

#ifndef BOOTLOADER
/**
 * RMI API
 */

#define RMI_I2C_ADDR    0x40

static unsigned char dev_ctl_reg; /* cached value of control register */

/* NOTE:
 * RMI over i2c supports some special aliases on page 0x2 but this driver don't
 * use them */

struct rmi_xfer_t;
typedef void (*rmi_xfer_cb_t)(struct rmi_xfer_t *xfer);

/* Represent a typical RMI transaction: a first transfer to select the page
 * and a second transfer to read/write registers. The API takes care of annoying
 * details and will simply call the callback at the end of the transfer. */
struct rmi_xfer_t
{
    struct imx233_i2c_xfer_t xfer_page; /* first transfer: page select */
    struct imx233_i2c_xfer_t xfer_rw; /* second transfer: read/write */
    uint8_t sel_page[2]; /* write command to select page */
    uint8_t sel_reg; /* write command to select register */
    volatile enum imx233_i2c_error_t status; /* transfer status */
    rmi_xfer_cb_t callback; /* callback */
};

/* Synchronous transfer: add a semaphore to block */
struct rmi_xfer_sync_t
{
    struct rmi_xfer_t xfer;
    struct semaphore sema; /* semaphore for completion */
};

/* callback for first transfer: record error if any */
static void rmi_i2c_first_callback(struct imx233_i2c_xfer_t *xfer, enum imx233_i2c_error_t status)
{
    struct rmi_xfer_t *rxfer = container_of(xfer, struct rmi_xfer_t, xfer_page);
    /* record status */
    rxfer->status = status;
}

/* callback for first transfer: handle error and callback */
static void rmi_i2c_second_callback(struct imx233_i2c_xfer_t *xfer, enum imx233_i2c_error_t status)
{
    struct rmi_xfer_t *rxfer = container_of(xfer, struct rmi_xfer_t, xfer_rw);
    /* record status, only if not skipping (ie the error was in first transfer) */
    if(status != I2C_SKIP)
        rxfer->status = status;
    /* callback */
    if(rxfer->callback)
        rxfer->callback(rxfer);
}

/* build a rmi transaction to read/write registers; do NOT cross page boundary ! */
static void rmi_build_xfer(struct rmi_xfer_t *xfer, bool read, int address,
    int byte_count, unsigned char *buffer, rmi_xfer_cb_t callback)
{
    /* first transfer: change page */
    xfer->xfer_page.next = &xfer->xfer_rw;
    xfer->xfer_page.fast_mode = true;
    xfer->xfer_page.dev_addr = RMI_I2C_ADDR;
    xfer->xfer_page.mode = I2C_WRITE;
    xfer->xfer_page.count[0] = 2;
    xfer->xfer_page.data[0] = &xfer->sel_page;
    xfer->xfer_page.count[1] = 0;
    xfer->xfer_page.tmo_ms = 1000;
    xfer->xfer_page.callback = &rmi_i2c_first_callback;
    /* second transfer: read/write */
    xfer->xfer_rw.next = NULL;
    xfer->xfer_rw.fast_mode = true;
    xfer->xfer_rw.dev_addr = RMI_I2C_ADDR;
    xfer->xfer_rw.mode = read ? I2C_READ : I2C_WRITE;
    xfer->xfer_rw.count[0] = 1;
    xfer->xfer_rw.data[0] = &xfer->sel_reg;
    xfer->xfer_rw.count[1] = byte_count;
    xfer->xfer_rw.data[1] = buffer;
    xfer->xfer_rw.tmo_ms = 1000;
    xfer->xfer_rw.callback = &rmi_i2c_second_callback;
    /* general things */
    xfer->callback = callback;
    xfer->sel_page[0] = RMI_PAGE_SELECT;
    xfer->sel_page[1] = address >> 8;
    xfer->sel_reg = address & 0xff;
}

/** IMPORTANT NOTE
 *
 * All transfers are built using rmi_build_xfer which constructs a transaction
 * consisting in a page select and register read/writes. Since transactions are
 * executed "atomically" and are queued, it is safe to call transfers functions
 * concurrently. However only asynchronous transfers can be used in IRQ context.
 * In all cases, make sure the the rmi_xfer_t structure lives at least until the
 * completion of the transfer (callback).
 */

/* queue transfer to change sleep mode, return true if transfer was queued
 * and false if ignored because requested mode is already the current one.
 * call must provide a transfer structure that must exist until completion */
static bool rmi_set_sleep_mode_async(struct rmi_xfer_t *xfer, uint8_t *buf,
    unsigned char sleep_mode, rmi_xfer_cb_t callback)
{
    /* avoid any race with concurrent changes to the mode */
    unsigned long cpsr = disable_irq_save();
    /* valid value different from the actual one */
    if((dev_ctl_reg & RMI_SLEEP_MODE_BM) != sleep_mode)
    {
        /* change cached version */
        dev_ctl_reg &= ~RMI_SLEEP_MODE_BM;
        dev_ctl_reg |= sleep_mode;
        *buf = dev_ctl_reg;
        restore_irq(cpsr);
        /* build transfer and kick */
        rmi_build_xfer(xfer, false, RMI_DEVICE_CONTROL, 1, buf, callback);
        imx233_i2c_transfer(&xfer->xfer_page);
        return true;
    }
    else
    {
        restore_irq(cpsr);
        return false;
    }
}

/* synchronous callback: release semaphore */
static void rmi_i2c_sync_callback(struct rmi_xfer_t *xfer)
{
    struct rmi_xfer_sync_t *sxfer = (void *)xfer;
    semaphore_release(&sxfer->sema);
}

/* synchronous read/write */
static void rmi_rw(bool read, int address, int byte_count, unsigned char *buffer)
{
    struct rmi_xfer_sync_t xfer;
    rmi_build_xfer(&xfer.xfer, read, address, byte_count, buffer, rmi_i2c_sync_callback);
    semaphore_init(&xfer.sema, 1, 0);
    /* kick and wait */
    imx233_i2c_transfer(&xfer.xfer.xfer_page);
    semaphore_wait(&xfer.sema, TIMEOUT_BLOCK);
    if(xfer.xfer.status != I2C_SUCCESS)
        panicf("rmi: i2c err %d", xfer.xfer.status);
}

/* read registers synchronously */
static void rmi_read(int address, int byte_count, unsigned char *buffer)
{
    rmi_rw(true, address, byte_count, buffer);
}

/* read single register synchronously */
static int rmi_read_single(int address)
{
    unsigned char c;
    rmi_rw(true, address, 1, &c);
    return c;
}

/* write single register synchronously */
static void rmi_write_single(int address, unsigned char byte)
{
    return rmi_rw(false, address, 1, &byte);
}

/* synchronously change sleep mode, this is a nop if current mode is the same as requested */
static void rmi_set_sleep_mode(unsigned char sleep_mode)
{
    struct rmi_xfer_sync_t xfer;
    uint8_t buf;
    semaphore_init(&xfer.sema, 1, 0);
    /* kick asynchronous transfer and only wait if mode was actually changed */
    if(rmi_set_sleep_mode_async(&xfer.xfer, &buf, sleep_mode, &rmi_i2c_sync_callback))
    {
        semaphore_wait(&xfer.sema, TIMEOUT_BLOCK);
        if(xfer.xfer.status != I2C_SUCCESS)
            panicf("rmi: i2c err %d", xfer.xfer.status);
    }
}

static void rmi_init(void)
{
    /* cache control register */
    dev_ctl_reg = rmi_read_single(RMI_DEVICE_CONTROL);
}


/**
 * Touchpad API
 */

/* we emulate a 3x3 grid, this gives the button mapping */
int button_mapping[3][3] =
{
    {BUTTON_BOTTOMLEFT, BUTTON_LEFT, BUTTON_BACK},
    {BUTTON_DOWN, BUTTON_SELECT, BUTTON_UP},
    {BUTTON_BOTTOMRIGHT, BUTTON_RIGHT, BUTTON_PLAYPAUSE},
};

/* timeout before lowering touchpad power from lack of activity */
#define ACTIVITY_TMO        (5 * HZ)
#define TOUCHPAD_WIDTH      3010
#define TOUCHPAD_HEIGHT     1975
#define DEADZONE_MULTIPLIER 2 /* deadzone multiplier */

/* power level when touchpad is active: experiments show that "low power" reduce
 * power consumption and hardly makes a difference in quality. */
#define ACTIVE_POWER_LEVEL  RMI_SLEEP_MODE_LOW_POWER

static int touchpad_btns = 0; /* button bitmap for the touchpad */
static unsigned last_activity = 0; /* tick of the last touchpad activity */
static bool t_enable = true; /* is touchpad enabled? */
static int deadzone; /* deadzone size */
static struct timeout activity_tmo; /* activity timeout */

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

/* i2c transfer only used for irq processing
 * NOTE we use two sets of transfers because we setup one in the callback of the
 * other, using one would be unsafe */
static struct rmi_xfer_t rmi_irq_xfer[2];
static uint8_t rmi_irq_buf; /* buffer to hold irq status register and sleep mode */
static union
{
    unsigned char data[10];
    struct
    {
        struct rmi_2d_absolute_data_t absolute;
        struct rmi_2d_relative_data_t relative;
        struct rmi_2d_gesture_data_t gesture;
    }s;
}rmi_irq_data; /* buffer to hold touchpad data */

static void rmi_attn_cb(int bank, int pin, intptr_t user);

/* callback for i2c transfer to change power level after irq */
static void rmi_power_irq_cb(struct rmi_xfer_t *xfer)
{
    /* we do not recover from error for now */
    if(xfer->status != I2C_SUCCESS)
        panicf("rmi: clear i2c err %d", xfer->status);
    /* now that interrupt is cleared, we can renable attention irq */
    imx233_pinctrl_setup_irq(0, 27, true, true, false, &rmi_attn_cb, 0);
}

/* callback for i2c transfer to read/clear interrupt status register */
static void rmi_clear_irq_cb(struct rmi_xfer_t *xfer)
{
    /* we do not recover from error for now */
    if(xfer->status != I2C_SUCCESS)
        panicf("rmi: clear i2c err %d", xfer->status);
    /* at this point, we might have processed an event and the touchpad still be
     * in very low power mode because of some previous inactivity; if it's the case,
     * schedule another transfer to switch to a higher power mode before accepting the
     * next event */
    /* kick asynchronous transfer and only wait if mode was actually changed */
    if(!rmi_set_sleep_mode_async(&rmi_irq_xfer[0], &rmi_irq_buf, ACTIVE_POWER_LEVEL,
            &rmi_power_irq_cb))
        /* now that interrupt is cleared, we can renable attention irq */
        imx233_pinctrl_setup_irq(0, 27, true, true, false, &rmi_attn_cb, 0);
}

/* callback for i2c transfer to read touchpad data registers */
static void rmi_data_irq_cb(struct rmi_xfer_t *xfer)
{
    /* we do not recover from error for now */
    if(xfer->status != I2C_SUCCESS)
        panicf("rmi: data i2c err %d", xfer->status);
    /* now that we have the data, setup another transfer to clear interrupt */
    rmi_build_xfer(&rmi_irq_xfer[1], true, RMI_INTERRUPT_REQUEST, 1,
        &rmi_irq_buf, &rmi_clear_irq_cb);
    /* kick transfer */
    imx233_i2c_transfer(&rmi_irq_xfer[1].xfer_page);
    /* now process touchpad data */
    int absolute_x = rmi_irq_data.s.absolute.x_msb << 8 | rmi_irq_data.s.absolute.x_lsb;
    int absolute_y = rmi_irq_data.s.absolute.y_msb << 8 | rmi_irq_data.s.absolute.y_lsb;
    int nr_fingers = rmi_irq_data.s.absolute.misc & 7;
    if(nr_fingers == 1)
        touchpad_btns = find_button(absolute_x, absolute_y);
    else
        touchpad_btns = 0;
}

/* touchpad attention line interrupt */
static void rmi_attn_cb(int bank, int pin, intptr_t user)
{
    (void) bank;
    (void) pin;
    (void) user;
   /* build transfer to read data registers */
    rmi_build_xfer(&rmi_irq_xfer[0], true, RMI_DATA_REGISTER(0),
        sizeof(rmi_irq_data.data), rmi_irq_data.data, &rmi_data_irq_cb);
    /* kick transfer */
    imx233_i2c_transfer(&rmi_irq_xfer[0].xfer_page);
    /* update last activity */
    last_activity = current_tick;
}

void touchpad_enable_device(bool en)
{
    t_enable = en;
    rmi_set_sleep_mode(en ? ACTIVE_POWER_LEVEL : RMI_SLEEP_MODE_SENSOR_SLEEP);
}

void touchpad_set_sensitivity(int level)
{
    /* handle negative values as well ! */
    rmi_write_single(RMI_2D_SENSITIVITY_ADJ, (unsigned char)(int8_t)level);
}

/* transfer used by the activity timeout to change power level */
static struct rmi_xfer_t rmi_tmo_xfer;
static uint8_t rmi_tmo_buf;

/* activity timeout: lower power level after some inactivity */
static int activity_monitor(struct timeout *tmo)
{
    (void) tmo;
    if(TIME_AFTER(current_tick, last_activity + ACTIVITY_TMO))
    {
        /* don't change power mode if touchpad is disabled, it's already in sleep mode */
        if(t_enable)
            rmi_set_sleep_mode_async(&rmi_tmo_xfer, &rmi_tmo_buf,
                RMI_SLEEP_MODE_VERY_LOW_POWER, NULL);
    }

    return HZ; /* next check in 1 second */
}

void touchpad_init(void)
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

    /* use a timer to monitor touchpad activity and manage power level */
    last_activity = current_tick;
    timeout_register(&activity_tmo, activity_monitor, HZ, 0);

    rmi_init();

    char product_id[RMI_PRODUCT_ID_LEN];
    rmi_read(RMI_PRODUCT_ID, RMI_PRODUCT_ID_LEN, product_id);
    /* The OF adjust the sensitivity based on product_id[1] compared to 2.
     * Since it doesn't seem to work great, just hardcode the sensitivity to
     * some reasonable value for now. */
    rmi_write_single(RMI_2D_SENSITIVITY_ADJ, 13);

    rmi_write_single(RMI_2D_GESTURE_SETTINGS,
        RMI_2D_GESTURE_PRESS_TIME_300MS |
        RMI_2D_GESTURE_FLICK_DIST_4MM << RMI_2D_GESTURE_FLICK_DIST_BP |
        RMI_2D_GESTURE_FLICK_TIME_700MS << RMI_2D_GESTURE_FLICK_TIME_BP);

    /* we don't know in which mode the touchpad start so use a sane default */
    rmi_set_sleep_mode(ACTIVE_POWER_LEVEL);
    /* enable interrupt */
    imx233_pinctrl_acquire(0, 27, "touchpad int");
    imx233_pinctrl_set_function(0, 27, PINCTRL_FUNCTION_GPIO);
    imx233_pinctrl_enable_gpio(0, 27, false);
    imx233_pinctrl_setup_irq(0, 27, true, true, false, &rmi_attn_cb, 0);
}

/**
 * Debug screen
 */

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
    /* remember tick of last gestures */
    #define GESTURE_TMO     HZ / 2
    int tick_last_tap = current_tick - GESTURE_TMO;
    int tick_last_doubletap = current_tick - GESTURE_TMO;
    int tick_last_taphold = current_tick - GESTURE_TMO;
    int tick_last_flick = current_tick - GESTURE_TMO;
    int flick_x = 0, flick_y = 0;

    /* BUG the data register are usually read by the IRQ already and it is
     * important to not read them again, otherwise we could miss some events
     * (most notable gestures). However, we only read registers when the
     * touchpad is active so the data might be outdated if touchpad is
     * inactive. We should implement a continuous reading mode for the debug
     * screen. */

    lcd_setfont(FONT_SYSFIXED);
    while(1)
    {
        /* call button_get() to avoid an overflow in the button queue */
        button_get(false);

        unsigned char sleep_mode = rmi_read_single(RMI_DEVICE_CONTROL) & RMI_SLEEP_MODE_BM;
        lcd_set_viewport(NULL);
        lcd_clear_display();
        int btns = button_read_device();
        lcd_putsf(0, 0, "button bitmap: %x", btns);
        lcd_putsf(0, 1, "RMI: id=%s info=%s", product_id, product_info_str);
        lcd_putsf(0, 2, "xmax=%d ymax=%d res=%d", x_max, y_max, sensor_resol);
        lcd_putsf(0, 3, "attn=%d ctl=%x",
            imx233_pinctrl_get_gpio(0, 27) ? 0 : 1,
            rmi_read_single(RMI_DEVICE_CONTROL));
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

        /* Disable IRQs when reading to avoid reading incorrect data */
        unsigned long cpsr = disable_irq_save();
        memcpy(&u, &rmi_irq_data, sizeof(u));
        restore_irq(cpsr);

        int absolute_x = u.s.absolute.x_msb << 8 | u.s.absolute.x_lsb;
        int absolute_y = u.s.absolute.y_msb << 8 | u.s.absolute.y_lsb;
        int nr_fingers = u.s.absolute.misc & 7;
        bool gesture = (u.s.absolute.misc & 8) == 8;
        int palm_width = u.s.absolute.misc >> 4;

        lcd_putsf(0, 6, "abs: %d %d %d", absolute_x, absolute_y, (int)u.s.absolute.z);
        lcd_putsf(0, 7, "rel: %d %d", (int)u.s.relative.x, (int)u.s.relative.y);
        lcd_putsf(0, 8, "gesture: %x %x", u.s.gesture.misc, u.s.gesture.flick);
        lcd_putsf(0, 9, "misc: w=%d g=%d f=%d", palm_width, gesture, nr_fingers);
        lcd_putsf(0, 10, "sleep_mode: %d", sleep_mode);
        lcd_putsf(0, 11, "deadzone: %d", deadzone);
        /* display virtual touchpad with deadzones */
        lcd_set_viewport(&report_vp);
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0xff, 0), LCD_BLACK);
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
            {
                int x = j * x_max / 3;
                if(j != 0)
                    x += deadzone;
                int x2 = (j + 1) * x_max / 3;
                if(j != 2)
                    x2 -= deadzone;
                int y = i * y_max / 3;
                if(i != 0)
                    y += deadzone;
                int y2 = (i + 1) * y_max / 3;
                if(i != 2)
                    y2 -= deadzone;
                x = DX2SX(x); x2 = DX2SX(x2); y = DY2SY(y); y2 = DY2SY(y2);
                lcd_drawrect(x, y2, x2 - x + 1, y - y2 + 1);
            }
        lcd_set_drawinfo(DRMODE_SOLID, LCD_RGBPACK(0xff, 0, 0), LCD_BLACK);
        lcd_drawrect(0, 0, zone_w, zone_h);
        /* put a done at the reported position of the finger
         * also display relative motion by a line as reported by the device */
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
                    tick_last_tap = current_tick;
                    break;
                case RMI_2D_GEST_MISC_DOUBLE_TAP:
                    tick_last_doubletap = current_tick;
                    break;
                case RMI_2D_GEST_MISC_TAP_AND_HOLD:
                    tick_last_taphold = current_tick;
                    break;
                default: break;
            }

            if(u.s.gesture.misc & RMI_2D_GEST_MISC_FLICK)
            {
                tick_last_flick = current_tick;
                flick_x = u.s.gesture.flick & RMI_2D_GEST_FLICK_X_BM;
                flick_y = (u.s.gesture.flick & RMI_2D_GEST_FLICK_Y_BM) >> RMI_2D_GEST_FLICK_Y_BP;
                #define SIGN4EXT(a) \
                    if(a & 8) a = -((a ^ 0xf) + 1);
                SIGN4EXT(flick_x);
                SIGN4EXT(flick_y);
            }

            if(TIME_BEFORE(current_tick, tick_last_tap + GESTURE_TMO))
                lcd_putsf(0, 0, "TAP!");
            if(TIME_BEFORE(current_tick, tick_last_doubletap + GESTURE_TMO))
                lcd_putsf(0, 1, "DOUBLE TAP!");
            if(TIME_BEFORE(current_tick, tick_last_taphold + GESTURE_TMO))
                lcd_putsf(0, 2, "TAP & HOLD!");
            if(TIME_BEFORE(current_tick, tick_last_flick + GESTURE_TMO))
            {
                lcd_putsf(0, 3, "FLICK!");
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
    }

    lcd_set_viewport(NULL);
    lcd_setfont(FONT_UI);
    return true;
}

#else /* BOOTLOADER */
int touchpad_read_device(void)
{
    return 0;
}
#endif

/**
 * Button API
 */
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

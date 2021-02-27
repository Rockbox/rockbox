/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "panic.h"
#include "lcd.h"
#include "gpio-x1000.h"
#include "i2c-x1000.h"
#include <string.h>
#include <stdbool.h>

#ifndef BOOTLOADER
# include "font.h"
#endif

#define FT_RST_PIN      (1 << 15)
#define FT_INT_PIN      (1 << 12)
#define ft_interrupt    GPIOB12

/* Touch event types */
#define EVENT_NONE      (-1)
#define EVENT_PRESS     0
#define EVENT_RELEASE   1
#define EVENT_CONTACT   2

/* FSM states */
#define STATE_IDLE          0
#define STATE_PRESS         1
#define STATE_REPORT        2
#define STATE_SCROLL_PRESS  3
#define STATE_SCROLLING     4

/* Assume there's no active touch if no event is reported in this time */
#define AUTORELEASE_TIME    (10000 * OST_TICKS_PER_US)

/* If there's no significant motion on the scrollbar for this time,
 * then report it as a button press instead */
#define SCROLL_PRESS_TIME   (100000 * OST_TICKS_PER_US)

/* If a press on the scrollbar moves more than this during SCROLL_PRESS_TIME,
 * then we enter scrolling mode. */
#define MIN_SCROLL_THRESH   15

/* If OST tick a is after OST tick b, then returns the number of ticks
 * in the interval between a and b; otherwise undefined. */
#define TICKS_SINCE(a, b)   ((a) - (b))

/* Number of touch samples to smooth before reading */
#define TOUCH_SAMPLES   3

static struct ft_driver {
    int i2c_cookie;
    i2c_descriptor i2c_desc;
    uint8_t raw_data[6];
    bool active;

    /* Number of pixels squared which must be moved before
     * a scrollbar pulse is generated */
    int scroll_thresh_sqr;
} ftd;

static struct ft_state_machine {
    /* Current button state, used by button_read_device() */
    int buttons;

    /* FSM state */
    int state;

    /* Time of the last touch event, as 32-bit OST timestamp. The kernel
     * tick is simply too low-resolution to work reliably, especially as
     * we handle touchpad events asynchronously. */
    uint32_t last_event_t;

    /* Time of entering the SCROLL_PRESS state, used to differentiate
     * between a press, hold, or scrolling motion */
    uint32_t scroll_press_t;

    /* Number of CONTACT events sampled in the PRESS state.
     * Must reach TOUCH_SAMPLES before we move forward. */
    int samples;

    /* Filter for smoothing touch points */
    int sum_x, sum_y;

    /* Position of the original touch */
    int orig_x, orig_y;

    /* Current touch position */
    int cur_x, cur_y;
} fsm;

static int touch_to_button(int x, int y)
{
    if(x == 900) {
        /* Right strip */
        if(y == 80)
            return BUTTON_BACK;
        else if(y == 240)
            return BUTTON_RIGHT;
        else
            return 0;
    } else if(x < 80) {
        /* Left strip */
        if(y < 80)
            return BUTTON_MENU;
        else if(y > 190)
            return BUTTON_LEFT;
        else
            return 0;
    } else {
        /* Middle strip */
        if(y < 100)
            return BUTTON_UP;
        else if(y > 220)
            return BUTTON_DOWN;
        else
            return BUTTON_SELECT;
    }
}

static bool ft_accum_touch(uint32_t t, int tx, int ty)
{
    /* Record event time */
    fsm.last_event_t = t;

    if(fsm.samples < TOUCH_SAMPLES) {
        /* Continue "priming" the filter */
        fsm.sum_x += tx;
        fsm.sum_y += ty;
        fsm.samples += 1;

        /* Return if filter is not ready */
        if(fsm.samples < TOUCH_SAMPLES)
            return false;
    } else {
        /* Update filter */
        fsm.sum_x += tx - fsm.sum_x / TOUCH_SAMPLES;
        fsm.sum_y += ty - fsm.sum_y / TOUCH_SAMPLES;
    }

    /* Filter is ready, so read the point */
    fsm.cur_x = fsm.sum_x / TOUCH_SAMPLES;
    fsm.cur_y = fsm.sum_y / TOUCH_SAMPLES;
    return true;
}

static void ft_go_idle(void)
{
    /* Null out the touch state */
    fsm.buttons = 0;
    fsm.samples = 0;
    fsm.sum_x = fsm.sum_y = 0;
    fsm.state = STATE_IDLE;
}

static void ft_start_report(void)
{
    /* Report the button bit */
    fsm.buttons = touch_to_button(fsm.cur_x, fsm.cur_y);
    fsm.orig_x = fsm.cur_x;
    fsm.orig_y = fsm.cur_y;
    fsm.state = STATE_REPORT;
}

static void ft_start_report_or_scroll(void)
{
    ft_start_report();

    /* If the press occurs on the scrollbar, then we need to
     * wait an additional delay before reporting it in case
     * this is the beginning of a scrolling motion */
    if(fsm.buttons & (BUTTON_UP|BUTTON_DOWN|BUTTON_SELECT)) {
        fsm.buttons = 0;
        fsm.scroll_press_t = __ost_read32();
        fsm.state = STATE_SCROLL_PRESS;
    }
}

static void ft_step_state(uint32_t t, int evt, int tx, int ty)
{
    /* Generate a release event automatically in case we missed it */
    if(evt == EVENT_NONE) {
        if(TICKS_SINCE(t, fsm.last_event_t) >= AUTORELEASE_TIME) {
            evt = EVENT_RELEASE;
            tx = fsm.cur_x;
            ty = fsm.cur_y;
        }
    }

    switch(fsm.state) {
    case STATE_IDLE: {
        if(evt == EVENT_PRESS || evt == EVENT_CONTACT) {
            /* Move to REPORT or PRESS state */
            if(ft_accum_touch(t, tx, ty))
                ft_start_report_or_scroll();
            else
                fsm.state = STATE_PRESS;
        }
    } break;

    case STATE_PRESS: {
        if(evt == EVENT_RELEASE) {
            /* Ignore if the number of samples is too low */
            ft_go_idle();
        } else if(evt == EVENT_PRESS || evt == EVENT_CONTACT) {
            /* Accumulate the touch position in the filter */
            if(ft_accum_touch(t, tx, ty))
                ft_start_report_or_scroll();
        }
    } break;

    case STATE_REPORT: {
        if(evt == EVENT_RELEASE)
            ft_go_idle();
        else if(evt == EVENT_PRESS || evt == EVENT_CONTACT)
            ft_accum_touch(t, tx, ty);
    } break;

    case STATE_SCROLL_PRESS: {
        if(evt == EVENT_RELEASE) {
            /* This _should_ synthesize a button press.
             *
             * - ft_start_report() will set the button bit based on the
             *   current touch position and enter the REPORT state, which
             *   will automatically hold the bit high
             *
             * - The next button_read_device() will see the button bit
             *   and report it back to Rockbox, then step the FSM with
             *   EVENT_NONE.
             *
             * - The EVENT_NONE stepping will eventually autogenerate a
             *   RELEASE event and restore the button state back to 0
             *
             * - There's a small logic hole in the REPORT state which
             *   could cause it to miss an immediately repeated PRESS
             *   that occurs before the autorelease timeout kicks in.
             *   FIXME: We might want to special-case that.
             */
            ft_start_report();
            break;
        }

        if(evt == EVENT_PRESS || evt == EVENT_CONTACT)
            ft_accum_touch(t, tx, ty);

        int dx = fsm.cur_x - fsm.orig_x;
        int dy = fsm.cur_y - fsm.orig_y;
        int dp = (dx*dx) + (dy*dy);
        if(dp >= MIN_SCROLL_THRESH*MIN_SCROLL_THRESH) {
            /* Significant motion: enter SCROLLING state */
            fsm.state = STATE_SCROLLING;
        } else if(TICKS_SINCE(t, fsm.scroll_press_t) >= SCROLL_PRESS_TIME) {
            /* No significant motion: report it as a press */
            fsm.cur_x = fsm.orig_x;
            fsm.cur_y = fsm.orig_y;
            ft_start_report();
        }
    } break;

    case STATE_SCROLLING: {
        if(evt == EVENT_RELEASE) {
            ft_go_idle();
            break;
        }

        if(evt == EVENT_PRESS || evt == EVENT_CONTACT)
            ft_accum_touch(t, tx, ty);

        int dx = fsm.cur_x - fsm.orig_x;
        int dy = fsm.cur_y - fsm.orig_y;
        int dp = (dx*dx) + (dy*dy);
        if(dp >= ftd.scroll_thresh_sqr) {
            if(dy < 0) {
                queue_post(&button_queue, BUTTON_SCROLL_BACK, 0);
            } else {
                queue_post(&button_queue, BUTTON_SCROLL_FWD, 0);
            }

            /* Poke the backlight */
            backlight_on();
            buttonlight_on();

            fsm.orig_x = fsm.cur_x;
            fsm.orig_y = fsm.cur_y;
        }
    } break;

    default:
        panicf("ft6x06: unhandled state");
        break;
    }
}

static void ft_i2c_callback(int status, i2c_descriptor* desc)
{
    (void)desc;
    if(status != I2C_STATUS_OK)
        return;

    /* The panel is oriented such that its X axis is vertical,
     * so swap the axes for reporting */
    int evt = ftd.raw_data[1] >> 6;
    int ty = ftd.raw_data[2] | ((ftd.raw_data[1] & 0xf) << 8);
    int tx = ftd.raw_data[4] | ((ftd.raw_data[3] & 0xf) << 8);

    /* TODO: convert the touch positions to linear positions.
     *
     * Points reported by the touch controller are distorted and non-linear,
     * ideally we'd like to correct these values. There's more precision in
     * the middle of the touchpad than on the edges, so scrolling feels slow
     * in the middle and faster near the edge.
     */

    ft_step_state(__ost_read32(), evt, tx, ty);
}

void ft_interrupt(void)
{
    /* We don't care if this fails */
    i2c_async_queue(FT6x06_BUS, TIMEOUT_NOBLOCK, I2C_Q_ONCE,
                    ftd.i2c_cookie, &ftd.i2c_desc);
}

static void ft_init(void)
{
    /* Initialize the driver state */
    ftd.i2c_cookie = i2c_async_reserve_cookies(FT6x06_BUS, 1);
    ftd.i2c_desc.slave_addr = FT6x06_ADDR;
    ftd.i2c_desc.bus_cond   = I2C_START | I2C_STOP;
    ftd.i2c_desc.tran_mode  = I2C_READ;
    ftd.i2c_desc.buffer[0]  = &ftd.raw_data[5];
    ftd.i2c_desc.count[0]   = 1;
    ftd.i2c_desc.buffer[1]  = &ftd.raw_data[0];
    ftd.i2c_desc.count[1]   = 5;
    ftd.i2c_desc.callback   = ft_i2c_callback;
    ftd.i2c_desc.arg        = 0;
    ftd.i2c_desc.next       = NULL;
    ftd.raw_data[5] = 0x02;
    ftd.active = true;
    touchpad_set_sensitivity(DEFAULT_TOUCHPAD_SENSITIVITY_SETTING);

    /* Initialize the state machine */
    fsm.buttons = 0;
    fsm.state = STATE_IDLE;
    fsm.last_event_t = 0;
    fsm.scroll_press_t = 0;
    fsm.samples = 0;
    fsm.sum_x = fsm.sum_y = 0;
    fsm.orig_x = fsm.orig_y = 0;
    fsm.cur_x = fsm.cur_y = 0;

    /* Bring up I2C bus */
    i2c_x1000_set_freq(FT6x06_BUS, I2C_FREQ_400K);

    /* Reset chip */
    gpio_config(GPIO_B, FT_RST_PIN|FT_INT_PIN, GPIO_OUTPUT(0));
    mdelay(5);
    gpio_out_level(GPIO_B, FT_RST_PIN, 1);
    gpio_config(GPIO_B, FT_INT_PIN, GPIO_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_B, FT_INT_PIN);
}

void touchpad_set_sensitivity(int level)
{
    int pixels = 40;
    pixels -= level;
    ftd.scroll_thresh_sqr = pixels * pixels;
}

void touchpad_enable_device(bool en)
{
    i2c_reg_write1(FT6x06_BUS, FT6x06_ADDR, 0xa5, en ? 0 : 3);
    ftd.active = en;
}

/* Value of headphone detect register */
static uint8_t hp_detect_reg = 0x00;

/* Interval to poll the register */
#define HPD_POLL_TIME (HZ/2)

static int hp_detect_tmo_cb(struct timeout* tmo)
{
    i2c_descriptor* d = (i2c_descriptor*)tmo->data;
    i2c_async_queue(AXP173_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, d);
    return HPD_POLL_TIME;
}

static void hp_detect_init(void)
{
    static struct timeout tmo;
    static const uint8_t gpio_reg = 0x94;
    static i2c_descriptor desc = {
        .slave_addr = AXP173_ADDR,
        .bus_cond = I2C_START | I2C_STOP,
        .tran_mode = I2C_READ,
        .buffer[0] = (void*)&gpio_reg,
        .count[0] = 1,
        .buffer[1] = &hp_detect_reg,
        .count[1] = 1,
        .callback = NULL,
        .arg = 0,
        .next = NULL,
    };

    /* Headphone detect is wired to an undocumented GPIO on the AXP173.
     * This sets it to input mode so we can see the pin state. */
    i2c_reg_write1(AXP173_BUS, AXP173_ADDR, 0x93, 0x01);

    /* Get an initial reading before startup */
    int r = i2c_reg_read1(AXP173_BUS, AXP173_ADDR, gpio_reg);
    if(r >= 0)
        hp_detect_reg = r;

    /* Poll the register every second */
    timeout_register(&tmo, &hp_detect_tmo_cb, HPD_POLL_TIME, (intptr_t)&desc);
}

/* Rockbox interface */
void button_init_device(void)
{
    /* Configure physical button GPIOs */
    gpio_config(GPIO_A, (1 << 17) | (1 << 19), GPIO_INPUT);
    gpio_config(GPIO_B, (1 << 28) | (1 << 31), GPIO_INPUT);

    /* Initialize touchpad */
    ft_init();

    /* Set up headphone detect polling */
    hp_detect_init();
}

int button_read_device(void)
{
    int r = fsm.buttons;
    ft_step_state(__ost_read32(), EVENT_NONE, 0, 0);

    /* Read GPIOs for physical buttons */
    uint32_t a = REG_GPIO_PIN(GPIO_A);
    uint32_t b = REG_GPIO_PIN(GPIO_B);

    /* All buttons are active low */
    if((a & (1 << 17)) == 0) r |= BUTTON_PLAY;
    if((a & (1 << 19)) == 0) r |= BUTTON_VOL_UP;
    if((b & (1 << 28)) == 0) r |= BUTTON_VOL_DOWN;
    if((b & (1 << 31)) == 0) r |= BUTTON_POWER;

    return r;
}

bool headphones_inserted()
{
    return hp_detect_reg & 0x40 ? true : false;
}

#ifndef BOOTLOADER
static int getbtn(void)
{
    int btn;
    do {
        btn = button_get_w_tmo(1);
    } while(btn & (BUTTON_REL|BUTTON_REPEAT));
    return btn;
}

bool dbg_fiiom3k_touchpad(void)
{
    static const char* fsm_statenames[] = {
        "IDLE", "PRESS", "REPORT", "SCROLL_PRESS", "SCROLLING"
    };

    do {
        int line = 0;
        lcd_clear_display();
        lcd_putsf(0, line++, "state: %s", fsm_statenames[fsm.state]);
        lcd_putsf(0, line++, "button: %08x", fsm.buttons);
        lcd_putsf(0, line++, "pos x: %4d  orig x: %4d", fsm.cur_x, fsm.orig_x);
        lcd_putsf(0, line++, "pos y: %4d  orig y: %4d", fsm.cur_y, fsm.orig_y);
        lcd_update();
    } while(getbtn() != BUTTON_POWER);
    return false;
}
#endif

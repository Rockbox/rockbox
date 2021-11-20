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
#include "powermgmt.h"
#include "panic.h"
#include "axp-pmu.h"
#include "ft6x06.h"
#include "gpio-x1000.h"
#include "irq-x1000.h"
#include "i2c-x1000.h"
#include <string.h>
#include <stdbool.h>

#ifndef BOOTLOADER
# include "lcd.h"
# include "font.h"
#endif

/* FSM states */
#define STATE_IDLE          0
#define STATE_PRESS         1
#define STATE_REPORT        2
#define STATE_SCROLL_PRESS  3
#define STATE_SCROLLING     4

/* Assume there's no active touch if no event is reported in this time */
#define AUTORELEASE_TIME    (40 * 1000 * OST_TICKS_PER_US)

/* If there's no significant motion on the scrollbar for this time,
 * then report it as a button press instead */
#define SCROLL_PRESS_TIME   (400 * 1000 * OST_TICKS_PER_US)

/* If a press on the scrollbar moves more than this during SCROLL_PRESS_TIME,
 * then we enter scrolling mode. */
#define MIN_SCROLL_THRESH   15

/* If OST tick a is after OST tick b, then returns the number of ticks
 * in the interval between a and b; otherwise undefined. */
#define TICKS_SINCE(a, b)   ((a) - (b))

/* Number of touch samples to smooth before reading */
#define TOUCH_SAMPLES   3

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

    /* Motion threshold squared, in 'pixels', required to trigger scrolling */
    int scroll_thresh_sqr;

    /* Touchpad enabled state */
    bool active;
} fsm;

/* coordinates below this are the left hand buttons,
 * coordinates above this are part of the scrollbar */
#define SCROLLSTRIP_LEFT_X  80

/* top and bottom cutoffs for the center of the strip,
 * divides it into top/middle/bottom zones */
#define SCROLLSTRIP_TOP_Y 105
#define SCROLLSTRIP_BOT_Y 185

/* cutoffs for the menu/left button zones */
#define MENUBUTTON_Y    80
#define LEFTBUTTON_Y    190

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
    } else if(x < SCROLLSTRIP_LEFT_X) {
        /* Left strip */
        if(y < MENUBUTTON_Y)
            return BUTTON_MENU;
        else if(y > LEFTBUTTON_Y)
            return BUTTON_LEFT;
        else
            return 0;
    } else {
        /* Middle strip */
        if(y < SCROLLSTRIP_TOP_Y)
            return BUTTON_UP;
        else if(y > SCROLLSTRIP_BOT_Y)
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
    if(evt == FT6x06_EVT_NONE) {
        if(TICKS_SINCE(t, fsm.last_event_t) >= AUTORELEASE_TIME) {
            evt = FT6x06_EVT_RELEASE;
            tx = fsm.cur_x;
            ty = fsm.cur_y;
        }
    }

    switch(fsm.state) {
    case STATE_IDLE: {
        if(evt == FT6x06_EVT_PRESS || evt == FT6x06_EVT_CONTACT) {
            /* Move to REPORT or PRESS state */
            if(ft_accum_touch(t, tx, ty))
                ft_start_report_or_scroll();
            else
                fsm.state = STATE_PRESS;
        }
    } break;

    case STATE_PRESS: {
        if(evt == FT6x06_EVT_RELEASE) {
            /* Ignore if the number of samples is too low */
            ft_go_idle();
        } else if(evt == FT6x06_EVT_PRESS || evt == FT6x06_EVT_CONTACT) {
            /* Accumulate the touch position in the filter */
            if(ft_accum_touch(t, tx, ty))
                ft_start_report_or_scroll();
        }
    } break;

    case STATE_REPORT: {
        if(evt == FT6x06_EVT_RELEASE)
            ft_go_idle();
        else if(evt == FT6x06_EVT_PRESS || evt == FT6x06_EVT_CONTACT)
            ft_accum_touch(t, tx, ty);
    } break;

    case STATE_SCROLL_PRESS: {
        if(evt == FT6x06_EVT_RELEASE) {
            /* This _should_ synthesize a button press.
             *
             * - ft_start_report() will set the button bit based on the
             *   current touch position and enter the REPORT state, which
             *   will automatically hold the bit high
             *
             * - The next button_read_device() will see the button bit
             *   and report it back to Rockbox, then step the FSM with
             *   FT6x06_EVT_NONE.
             *
             * - The FT6x06_EVT_NONE stepping will eventually autogenerate
             *   a RELEASE event and restore the button state back to 0
             *
             * - There's a small logic hole in the REPORT state which
             *   could cause it to miss an immediately repeated PRESS
             *   that occurs before the autorelease timeout kicks in.
             *   FIXME: We might want to special-case that.
             */
            ft_start_report();
            break;
        }

        if(evt == FT6x06_EVT_PRESS || evt == FT6x06_EVT_CONTACT)
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
        if(evt == FT6x06_EVT_RELEASE) {
            ft_go_idle();
            break;
        }

        if(evt == FT6x06_EVT_PRESS || evt == FT6x06_EVT_CONTACT)
            ft_accum_touch(t, tx, ty);

        int dx = fsm.cur_x - fsm.orig_x;
        int dy = fsm.cur_y - fsm.orig_y;
        int dp = (dx*dx) + (dy*dy);
        if(dp >= fsm.scroll_thresh_sqr) {
            /* avoid generating events if we're supposed to be inactive...
             * should not be necessary but better to be safe. */
            if(fsm.active) {
                if(dy < 0) {
                    queue_post(&button_queue, BUTTON_SCROLL_BACK, 0);
                } else {
                    queue_post(&button_queue, BUTTON_SCROLL_FWD, 0);
                }

                /* Poke the backlight */
                backlight_on();
                buttonlight_on();
                reset_poweroff_timer();
            }

            fsm.orig_x = fsm.cur_x;
            fsm.orig_y = fsm.cur_y;
        }
    } break;

    default:
        panicf("ft6x06: unhandled state");
        break;
    }
}

static void ft_event_cb(struct ft6x06_state* state)
{
    /* TODO: convert the touch positions to linear positions.
     *
     * Points reported by the touch controller are distorted and non-linear,
     * ideally we'd like to correct these values. There's more precision in
     * the middle of the touchpad than on the edges, so scrolling feels slow
     * in the middle and faster near the edge.
     */
    struct ft6x06_point* pt = &state->points[0];
    ft_step_state(__ost_read32(), pt->event, pt->pos_x, pt->pos_y);
}

static void ft_init(void)
{
    /* Initialize the state machine */
    fsm.buttons = 0;
    fsm.state = STATE_IDLE;
    fsm.last_event_t = 0;
    fsm.scroll_press_t = 0;
    fsm.samples = 0;
    fsm.sum_x = fsm.sum_y = 0;
    fsm.orig_x = fsm.orig_y = 0;
    fsm.cur_x = fsm.cur_y = 0;
    fsm.active = true;
    touchpad_set_sensitivity(DEFAULT_TOUCHPAD_SENSITIVITY_SETTING);

    /* Bring up I2C bus */
    i2c_x1000_set_freq(FT6x06_BUS, I2C_FREQ_400K);

    /* Driver init */
    ft6x06_init();
    ft6x06_set_event_cb(ft_event_cb);

    /* Reset chip */
    gpio_set_level(GPIO_FT6x06_RESET, 0);
    mdelay(5);
    gpio_set_level(GPIO_FT6x06_RESET, 1);

    /* Configure the interrupt pin */
    system_set_irq_handler(GPIO_TO_IRQ(GPIO_FT6x06_INTERRUPT),
                           ft6x06_irq_handler);
    gpio_set_function(GPIO_FT6x06_INTERRUPT, GPIOF_IRQ_EDGE(0));
    gpio_enable_irq(GPIO_FT6x06_INTERRUPT);
}

void touchpad_set_sensitivity(int level)
{
    int pixels = 40;
    pixels -= level;
    fsm.scroll_thresh_sqr = pixels * pixels;
}

void touchpad_enable_device(bool en)
{
    ft6x06_enable(en);
    fsm.active = en;
}

/* Value of headphone detect register */
static uint8_t hp_detect_reg = 0x00;

/* Interval to poll the register */
#define HPD_POLL_TIME (HZ/2)

static int hp_detect_tmo_cb(struct timeout* tmo)
{
    i2c_descriptor* d = (i2c_descriptor*)tmo->data;
    i2c_async_queue(AXP_PMU_BUS, TIMEOUT_NOBLOCK, I2C_Q_ADD, 0, d);
    return HPD_POLL_TIME;
}

static void hp_detect_init(void)
{
    static struct timeout tmo;
    static const uint8_t gpio_reg = AXP192_REG_GPIOSTATE1;
    static i2c_descriptor desc = {
        .slave_addr = AXP_PMU_ADDR,
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

    /* Headphone detect is wired to AXP192 GPIO: set it to input state */
    i2c_reg_write1(AXP_PMU_BUS, AXP_PMU_ADDR, AXP192_REG_GPIO2FUNCTION, 0x01);

    /* Get an initial reading before startup */
    int r = i2c_reg_read1(AXP_PMU_BUS, AXP_PMU_ADDR, gpio_reg);
    if(r >= 0)
        hp_detect_reg = r;

    /* Poll the register every second */
    timeout_register(&tmo, &hp_detect_tmo_cb, HPD_POLL_TIME, (intptr_t)&desc);
}

/* Rockbox interface */
void button_init_device(void)
{
    /* Initialize touchpad */
    ft_init();

    /* Set up headphone detect polling */
    hp_detect_init();
}

int button_read_device(void)
{
    int r = fsm.buttons;
    ft_step_state(__ost_read32(), FT6x06_EVT_NONE, 0, 0);

    /* Read GPIOs for physical buttons */
    uint32_t a = REG_GPIO_PIN(GPIO_A);
    uint32_t b = REG_GPIO_PIN(GPIO_B);

    /* All buttons are active low */
    if((a & (1 << 17)) == 0) r |= BUTTON_PLAY;
    if((a & (1 << 19)) == 0) r |= BUTTON_VOL_UP;
    if((b & (1 << 28)) == 0) r |= BUTTON_VOL_DOWN;
    if((b & (1 << 31)) == 0) r |= BUTTON_POWER;

    return touchpad_filter(r);
}

bool headphones_inserted(void)
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

    /* definition of box used to represent the touchpad */
    const int pad_w = LCD_WIDTH;
    const int pad_h = LCD_HEIGHT;
    const int box_h = pad_h - SYSFONT_HEIGHT*5;
    const int box_w = pad_w * box_h / pad_h;
    const int box_x = (LCD_WIDTH - box_w) / 2;
    const int box_y = SYSFONT_HEIGHT * 9 / 2;

    /* cutoffs converted to box coordinates */
    const int ss_left_x = box_x + SCROLLSTRIP_LEFT_X * box_w / pad_w;
    const int ss_top_y  = box_y + SCROLLSTRIP_TOP_Y * box_h / pad_h;
    const int ss_bot_y  = box_y + SCROLLSTRIP_BOT_Y * box_h / pad_h;
    const int menubtn_y = box_y + MENUBUTTON_Y * box_h / pad_h;
    const int leftbtn_y = box_y + LEFTBUTTON_Y * box_h / pad_h;

    bool draw_areas = true;
    bool draw_border = true;

    do {
        int line = 0;
        lcd_clear_display();
        lcd_putsf(0, line++, "state: %s", fsm_statenames[fsm.state]);
        lcd_putsf(0, line++, "button: %08x", fsm.buttons);
        lcd_putsf(0, line++, "pos x: %4d  orig x: %4d", fsm.cur_x, fsm.orig_x);
        lcd_putsf(0, line++, "pos y: %4d  orig y: %4d", fsm.cur_y, fsm.orig_y);

        /* draw touchpad box borders */
        if(draw_border)
            lcd_drawrect(box_x, box_y, box_w, box_h);

        /* draw crosshair */
        int tx = box_x + fsm.cur_x * box_w / pad_w;
        int ty = box_y + fsm.cur_y * box_h / pad_h;
        lcd_hline(tx-2, tx+2, ty);
        lcd_vline(tx, ty-2, ty+2);

        /* draw the button areas */
        if(draw_areas) {
            lcd_vline(ss_left_x, box_y, box_y+box_h);
            lcd_hline(ss_left_x, box_x+box_w, ss_top_y);
            lcd_hline(ss_left_x, box_x+box_w, ss_bot_y);
            lcd_hline(box_x, ss_left_x, menubtn_y);
            lcd_hline(box_x, ss_left_x, leftbtn_y);
        }

        lcd_update();
    } while(getbtn() != BUTTON_POWER);
    return false;
}
#endif

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Sebastian Leonhardt
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


/**
 * @file
 * This file provides the high-level driver for the scrollstrip.
 * Besides the usual sliding action it supports single up/down button events
 * when tapping on the upper respectively lower half of the strip, and an
 * afterscroll feature.
 * The driver handles different speed settings for sliding and afterscroll,
 * and also supports a "no slide"-mode, where the upper and lower area of
 * the strip is treated like ordinary buttons. (This can sometimes be
 * useful, e.g. in games.)
 *
 * To use the driver, the target must have the configuration HAVE_SCROLLSTRIP
 * and HAVE_BUTTON_DATA defined.
 * The scrollstrip position must be delivered by the target's
 * button_read_device() function (located in button-targetname.c) and is
 * passed to the strip driver via button_tick() (button.c)
 *
 * The position value ranges from 0 (top) to SCROLLSTRIP_MAX_VAL (bottom).
 * A negative value indicate that the scrollstrip currently isn't touched.
 * Although the absolute numbers doesn't matter, they need to be high
 * enough to avoid noticable rounding errors, so scaling of the hardware
 * values may be necessary. A total strip length around 16 bit size seems
 * to be reasonable.
 */


#include <stdlib.h>
#include <stdint.h>

#include "config.h"
#include "system.h"
#include "powermgmt.h"
#include "backlight.h"
#include "settings.h"
#include "button.h"
#include "kernel.h"
#ifdef HAVE_SDL
#include "button-sdl.h"
#else
#include "button-target.h"
#endif

#include "scrollstrip.h"

#ifndef HAVE_SDL /* suppress compiler warnings about unused vars */
/* last finger position on scrollstrip; negative numbers mean "not touched" */
static int32_t strip_last_pos = -1;
/* the strip position where the last button event was recognised */
static int32_t last_button_pos;
/* relative strip position where the next button event will be generated. This
   value is adjusted dynamically to achieve acceleration/deceleration effects.*/
static int32_t next_button_delta;
/* number of button ticks between button events. */
static int tick_counter = 0;

static unsigned int  scrollstrip_flags = 0;
/* finger has moved and caused at least one button event                */
#define HAVE_SLIDED             (1<<0)
/* direction of the finger slide                                        */
#define SLIDING_DIRECTION_UP    (1<<1)
/* to prevent heavy afterscrolling after accidental finger slips,
   this feature is only enabled when scrolling at least 2 button spaces */
#define SLIDED_MULTIPLE         (1<<2)
/* finger has left strip, afterscroll is up and running                 */
#define IS_AFTERSCROLLING       (1<<3)
#endif /* HAVE_SDL */

/* When the movement of the finger exceeds a specific distance, a button event
   is generated. The maximum delta is used when sliding slowly, and for
   reverse movement.                                                        */
static int32_t button_delta_max = BUTTON_DELTA_MAX;
/* when sliding fast, the distance is reduced up to this minimum to
   produce an acceleration effect.                                          */
static int32_t button_delta_min = BUTTON_DELTA_MIN;
/* After every button event the delta is reduced by this factor
   (16.16 fixed point value).                                               */
const int32_t  strip_accel = (1<<16) - STRIP_ACCELERATION;
/* every button tick the delta is increased by this factor, so the distance
   enlarges when sliding slower. (16.16 fixed point value)                  */
const int32_t  strip_decel = (1<<16) + STRIP_DECELERATION;

#ifndef HAVE_SDL /* suppress compiler warnings about unused vars */
/* while sliding, this var holds the time (ticks) between the button events.
   During afterscroll this var holds ticks<<8 to minimize rounding errors.  */
static int32_t afterscroll_ticks;
/* This value defines the slowest movement of the user's finger (time between
   button events) after which afterscroll will be enabled, and also the slowest
   afterscroll button generation (stops when decelerating beyond this value)*/
static int  afterscroll_max_ticks = AFTERSCROLL_MAX_TICKS;
#endif /* HAVE_SDL */
/* The fastest afterscroll-generated successive button events.              */
static int  afterscroll_min_ticks = AFTERSCROLL_MIN_TICKS;
/* Decay factor of the afterscroll (as 16.16 fixed point value)             */
static int32_t afterscroll_decay = (1<<16) + AFTERSCROLL_DECAY;

static bool sliding_disable = 0;
static bool afterscroll_disable = 0;


/**
 * Set speed of scrollstrip. Callback function, called from menu.
 * @see scrollstrip_set_afterscroll
 * @param speed     Setting 0 is assumed to be standard, negative values
 *                  cause slower, positive numbers higher speed. A speed
 *                  setting <= -100 switches to "no slide" mode.
 */
void scrollstrip_set_speed(int speed)
{
    if (speed <= -100) {
        sliding_disable = true;
        return;
    }

    sliding_disable = false;

    if (speed < 0)      /* the effect of the modifiers is not strong enough */
        speed *= 2;     /* with the slow settings, so double it.            */

    button_delta_max = BUTTON_DELTA_MAX - (speed*BUTTON_DELTA_MAX)
                        * SETTING_SPEED_MAX_BUT_DELTA / 256;
    button_delta_min = BUTTON_DELTA_MIN - (speed*BUTTON_DELTA_MIN)
                        * SETTING_SPEED_MIN_BUT_DELTA / 256;

    /* some afterscroll parameters depend on the strip speed */
    scrollstrip_set_afterscroll(global_settings.scrollstrip_afterscroll);
}

/**
 * Set speed and duration of afterscroll. Callback function, called from menu.
 * @see scrollstrip_set_speed
 * @param after     Setting 0 is assumed to be standard, negative values
 *                  cause less, positive numbers more afterscroll.
 *                  A setting <= -100 disables afterscroll.
 */
void scrollstrip_set_afterscroll(int after)
{
    if (after <= -100) {
        afterscroll_disable = true;
        return;
    }

    afterscroll_disable = false;

    afterscroll_decay = (1<<16) + AFTERSCROLL_DECAY
                        - after * (AFTERSCROLL_DECAY*SETTING_AFTER_DECAY/256);
    /* Note: this depends also on strip speed setting. */
    afterscroll_min_ticks = AFTERSCROLL_MIN_TICKS
                - (after*AFTERSCROLL_MIN_TICKS*SETTING_AFTER_MIN_TICKS) / 256
                - (global_settings.scrollstrip_speed*AFTERSCROLL_MIN_TICKS
                                        * SETTING_AFTER_MIN_TICKS) / 256;
}

#ifndef HAVE_SDL /* suppress compiler warnings about unused functions */
/**
 *  Determine sliding direction and directly post up- or down-button-event, as
 *  long as no other buttons are pressed at the same time. If so, all buttons
 *  are returned to the calling function and will NOT be posted directly.
 *  @param btn_flags Button flags to be added to the scrolling up/down event
 *                   (BUTTON_REPEAT or BUTTON_REL).
 *  @param other_btn Buttons pressed additional to sliding. If present, up/down
 *                   is NOT posted directly.
 *  @return          Button events that must be handled by "button_tick()"
 *                   (in button.c)
 */
static int post_button_event(int btn_flags, int other_btn)
{
    int updown_btn = btn_flags;

    if (scrollstrip_flags & SLIDING_DIRECTION_UP)
        updown_btn |= BUTTON_UP;
    else
        updown_btn |= BUTTON_DOWN;

#ifdef HAVE_LCD_FLIP
    if ( button_get_flip() )
        updown_btn = button_flip(updown_btn);
#endif

    if (other_btn == BUTTON_NONE) {
        /* Directly post strip sliding events. Make sure to not flood the queue.
           With lower settings than 2 however scrolling is lagging.           */
        if (button_queue_count() <= 2) {
            queue_post(&button_queue, updown_btn, 0);
            backlight_on();
#ifdef HAVE_BUTTONLIGHT
            buttonlight_on();
#endif
            reset_poweroff_timer();
        }
    }
    else {
        /* if other buttons are pressed in addition to strip scrolling, then
           don't post directly. Note that flags (repeat, release) and backlight
           is handled by "button_tick()" and therefore must be stripped here. */
        if (updown_btn & BUTTON_REL) {
            updown_btn = BUTTON_NONE;
        }
        else if (updown_btn & BUTTON_REPEAT) {
            updown_btn &= ~BUTTON_REPEAT;
        }
        other_btn |= updown_btn;
    }

    return other_btn;
}


/**
 *  Determine if touched position matches up- or down-button area.
 *  Used for single short taps and during "no slide"-mode.
 *  @param  strip_pos    Finger position on strip
 *  @return              Button event BUTTON_UP, BUTTON_DOWN or BUTTON_NONE
 */
static int detect_button_area(int32_t strip_pos)
{
    int button = BUTTON_NONE;

    if (strip_pos <= STRIP_POS_IS_BTN_UP)
        button = BUTTON_UP;
    else if (strip_pos >= STRIP_POS_IS_BTN_DOWN)
        button = BUTTON_DOWN;
    else {
        /* even if no button was pressed (i.e. hitting the "dead spot" between
           the button areas), at least this is expected by the user. 
           Note: this must not be done if any buttons are detected, or
           "First keypress enables backlight only" won't work!               */
        backlight_on();
#ifdef HAVE_BUTTONLIGHT
        buttonlight_on();
#endif
        reset_poweroff_timer();
    }
#ifdef HAVE_LCD_FLIP
    if ( button_get_flip() )
        button = button_flip(button);
#endif
    return button;
}
#endif /* HAVE_SDL */


#ifndef HAVE_SDL
/**
 * This is the driver's main function. Note that sliding usually generates
 * directly posted button events (better performance), whereas tapping returns
 * the button codes to the calling function (button_tick()).
 * @param other_btn     additional (non-strip) button presses; if present,
 *                      then afterscrolling is stopped immediately.
 *                      Furthermore sliding events are not posted directly
 *                      to make button combos possible.
 * @param strip_pos     represents the absolute position of the finger on
 *                      the scrollstrip, with 0 being at the top. A negative
 *                      value indicates that the strip isn't touched.
 * @return              button events that must be handled by the calling
 *                      function.
 */
int handle_scrollstrip(int other_btn, int32_t strip_pos)
{
    int button = BUTTON_NONE;

    if (sliding_disable) {
        /*
         * Button-Only-Mode: touching the upper (lower) area of the strip
         * is treated exactly like conventional button up (down) presses.
         * Button queue posting and flags (repeat...) is handled solely
         * by button_tick() in button.c
         */
        if (strip_pos > 0) { /* strip is touched */
            button = detect_button_area(strip_pos) | other_btn;
        }
    }
    else { /* regular scrollstrip sliding-mode */
        if (strip_pos < 0) {
            /*
             * Scrollstrip is not touched
             */
            if (strip_last_pos >= 0) {
                /*
                 * finger has just left scrollstrip
                 */
                if (!(scrollstrip_flags & HAVE_SLIDED)) {
                    if (tick_counter < TAP_EVENT_TIME) {
                        /* a short tap in the upper/lower area without previous
                           sliding produces a single button up/down event     */
                        button = detect_button_area(strip_last_pos) | other_btn;
                    }
                }
                else {
                    /* user already slided on strip: potentially enable afterscroll */
                    if (!afterscroll_disable) {
                        if (afterscroll_ticks < tick_counter)
                            afterscroll_ticks = tick_counter;
                        if (afterscroll_ticks < afterscroll_min_ticks)
                            afterscroll_ticks = afterscroll_min_ticks;
                        if ((afterscroll_ticks <= afterscroll_max_ticks)
                                && (scrollstrip_flags & SLIDED_MULTIPLE))
                            scrollstrip_flags |= IS_AFTERSCROLLING;
                        afterscroll_ticks <<= 8;   /* NOTE: during afterscroll this var
                                            holds ticks<<8 to minimize rounding errors */
                        tick_counter = 0;
                    }
                    if (!(scrollstrip_flags & IS_AFTERSCROLLING)) {
                        /* stopped scrolling: create button release event */
                        button = post_button_event(BUTTON_REL, other_btn);
                    }
                }
            } /* strip_last_pos >= 0 */
            else {
                /*
                 * strip not touched
                 */
                if (scrollstrip_flags & IS_AFTERSCROLLING) {
                    /* we have to do afterscrolling... */
                    if (other_btn != BUTTON_NONE) {
                        /* a new button is pressed: immediately stop afterscrolling
                           and clear queue, so that the new button event can be executed */
                        scrollstrip_flags &= ~IS_AFTERSCROLLING;
                        button_clear_queue();
                    }
                    else {
                        ++tick_counter;
                        if (tick_counter >= afterscroll_ticks>>8) {
                            button = post_button_event(BUTTON_REPEAT, other_btn);
                            afterscroll_ticks = (afterscroll_ticks*afterscroll_decay)>>16;
                            tick_counter = 0;
                            /* disable afterscroll when reaching minimum scroll speed */
                            if (afterscroll_ticks>>8 > afterscroll_max_ticks) {
                                scrollstrip_flags &= ~IS_AFTERSCROLLING;
                                button = post_button_event(BUTTON_REL, other_btn);
                            }
                        }
                    }
                }
            }
        } /* strip_pos < 0 */
        else {
            /*
             * finger touches scrollstrip (strip_pos >=0)
             */
            if (strip_last_pos < 0) {
                /*
                 * the initial finger touch: finger has just touched strip
                 */
                last_button_pos = strip_pos; /* starting position for button delta */
                if (scrollstrip_flags & IS_AFTERSCROLLING) {
                    /* Start with short distance for continuous fast scrolling,
                       and keep direction and HAVE_SLIDED flag                  */
                    next_button_delta = (button_delta_max+button_delta_min)/2;
                    scrollstrip_flags &= (SLIDING_DIRECTION_UP | HAVE_SLIDED);
                }
                else {
                    next_button_delta = button_delta_max; /* 1st movement=full distance */
                    scrollstrip_flags = 0;
                }
                tick_counter = 0;
            }
            else {
                /*
                 * finger touches scrollstrip for >1 button ticks
                 */
                ++tick_counter;
                /* Wake up display when strip is touched. Unfortunately this
                   can't be done earlier, otherwise "First Buttonpress Enables
                   Backlight Only" won't work! (single-tap-button-event will
                   come through, because the backlight is already on).      */
                if (tick_counter == TAP_EVENT_TIME) {
                    backlight_on();
#ifdef HAVE_BUTTONLIGHT
                    buttonlight_on();
#endif
                    reset_poweroff_timer();
                }

                /*
                 * evaluate movement of finger
                 */
                int32_t curr_distance = strip_pos-last_button_pos;

                /* always use positive values for the main movement direction */
                if (scrollstrip_flags & SLIDING_DIRECTION_UP)
                    curr_distance = -curr_distance;

                if (curr_distance >= next_button_delta) {
                    /* finger has reached next "button"-position */
                    if (scrollstrip_flags & HAVE_SLIDED) {
                        scrollstrip_flags |= SLIDED_MULTIPLE;
                        /* sliding direction is handled inside post function */
                        button = post_button_event(BUTTON_REPEAT, other_btn);
                    }
                    else
                        button = post_button_event(BUTTON_NONE, other_btn);
prepare_next_movement:
                    /* deploy strip acceleration and calculate next button delta */
                    scrollstrip_flags |= HAVE_SLIDED;
                    next_button_delta = (next_button_delta*strip_accel)>>16;
                    if (next_button_delta < button_delta_min)
                        next_button_delta = button_delta_min;
                    last_button_pos = strip_pos;
                    afterscroll_ticks = tick_counter;
                    tick_counter = 0;
                }
                else if (-curr_distance >= button_delta_max) {
                    /* finger moved in the opposite direction (always maximum delta) */
                    next_button_delta = button_delta_max;
                    scrollstrip_flags ^= SLIDING_DIRECTION_UP; /* reverse direction flag */
                    scrollstrip_flags &= ~SLIDED_MULTIPLE;  /* reset */
                    button = post_button_event(BUTTON_NONE, other_btn); /* never a button_repeat */
                    goto prepare_next_movement;
                }
                else {
                    /*
                     * finger stays on the same place (or at least doesn't
                     * move far enough to trigger a button event):
                     * gradually decrease acceleration
                     */
                    next_button_delta = (next_button_delta*strip_decel)>>16;
                    if (next_button_delta > button_delta_max)
                        next_button_delta = button_delta_max;
                }
            }
        }
    }

    strip_last_pos = strip_pos;

    return button;
}
#endif /* HAVE_SDL */

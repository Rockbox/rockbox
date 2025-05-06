/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2020 Solomon Peachy

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
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "kernel.h"
#include "sysfs.h"
#include "button.h"
#include "panic.h"

#ifdef HAVE_SCROLLWHEEL
#include "powermgmt.h"
#if defined(HAVE_BACKLIGHT) || defined(HAVE_BUTTON_LIGHT)
#include "backlight.h"
#endif
#endif /* HAVE_SCROLLWHEEL */

/* TODO:  HAVE_SCROLLWHEEL is a hack.  Instead of posting the exact number
   of clicks, instead do it similar to the ipod clickwheel and post
   the wheel angular velocity (degrees per second)

   * Track the relative position  (ie based on click events)
   * Use WHEELCLICKS_PER_ROTATION to convert clicks to angular distance
   * Compute to angular velocity (degrees per second)

 */
#define NR_POLL_DESC    4

static int num_devices = 0;
static struct pollfd poll_fds[NR_POLL_DESC];

void button_init_device(void)
{
    const char * const input_devs[NR_POLL_DESC] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        "/dev/input/event3",
    };

    for(int i = 0; i < NR_POLL_DESC; i++)
    {
        int fd = open(input_devs[i], O_RDONLY | O_CLOEXEC);

        if(fd >= 0)
        {
            poll_fds[num_devices].fd = fd;
            poll_fds[num_devices].events = POLLIN;
            poll_fds[num_devices].revents = 0;
            num_devices++;
        }
    }
}

void button_close_device(void)
{
    /* close descriptors */
    for(int i = 0; i < num_devices; i++)
    {
        close(poll_fds[i].fd);
    }
    num_devices = 0;
}

#ifdef BUTTON_DELAY_RELEASE
static int button_delay_release = 0;
static int delay_tick = 0;
#endif

#ifdef HAVE_TOUCHSCREEN
/* Last known touchscreen coordinates. */
static int _last_x = 0;
static int _last_y = 0;

/* Last known touchscreen state. */
static enum
{
    TOUCHSCREEN_STATE_UNKNOWN = 0,
    TOUCHSCREEN_STATE_UP,
    TOUCHSCREEN_STATE_DOWN
} _last_touch_state = TOUCHSCREEN_STATE_UNKNOWN;

// XXX ... figure out what is standard.
#define EVENT_VALUE_TOUCHSCREEN_PRESS    1
#define EVENT_VALUE_TOUCHSCREEN_RELEASE -1

static int ts_enabled = 0;

void touchscreen_enable_device(bool en)
{
    ts_enabled = en;
}

static bool handle_touchscreen_event(__u16 code, __s32 value)
{
    bool read_more = false;

    switch(code)
    {
        case ABS_X:
        case ABS_MT_POSITION_X:
        {
            _last_x = value;

            /* x -> next will be y. */
            read_more = true;

            break;
        }

        case ABS_Y:
        case ABS_MT_POSITION_Y:
        {
            _last_y = value;
            break;
        }

        case ABS_MT_TRACKING_ID:
        {
            if(value == EVENT_VALUE_TOUCHSCREEN_PRESS)
            {
                _last_touch_state = TOUCHSCREEN_STATE_DOWN;

                /* Press -> next will be x. */
                read_more = true;
            }
            else
            {
                _last_touch_state = TOUCHSCREEN_STATE_UP;
            }
            break;
        }
    }

    return read_more;
}
#endif

#ifdef HAVE_BUTTON_DATA
#define BDATA int *data
#else
#define BDATA void
#endif
int button_read_device(BDATA)
{
    static int button_bitmap = 0;
    struct input_event event;

#if defined(HAVE_BUTTON_DATA) && !defined(HAVE_TOUCHSCREEN)
    (void)data;
#endif

#ifdef HAVE_SCROLLWHEEL
    int wheel_ticks = 0;
#endif

#ifdef BUTTON_DELAY_RELEASE
    /* First de-assert delayed-release buttons */
    if (button_delay_release && current_tick >= delay_tick)
    {
        button_bitmap &= ~button_delay_release;
        button_delay_release = 0;
    }
#endif

    /* check if there are any events pending and process them */
    while(poll(poll_fds, num_devices, 0)) {
        for(int i = 0; i < num_devices; i++) {
            /* read only if non-blocking */
            if(poll_fds[i].revents & POLLIN) {
                int size = read(poll_fds[i].fd, &event, sizeof(event));
                if(size == (int)sizeof(event)) {
                    switch(event.type) {
                    case EV_KEY: {
                        /* map linux event code to rockbox button bitmap */
                        int bmap = button_map(event.code);

                        /* event.value == 0x10000 means press
                         * event.value == 0 means release
                         */
                        if(event.value) {
#ifdef HAVE_SCROLLWHEEL
                            /* Filter out wheel ticks */
                            if (bmap & BUTTON_SCROLL_BACK)
                                wheel_ticks--;
                            else if (bmap & BUTTON_SCROLL_FWD)
                                wheel_ticks++;
                            bmap &= ~(BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD);
#endif
#ifdef BUTTON_DELAY_RELEASE
                            bmap &= ~BUTTON_DELAY_RELEASE;
#endif
#if defined(HAVE_TOUCHSCREEN) && defined(BUTTON_TOUCH)
                            /* Some touchscreens give us actual touch/untouch as a "key" */
                            if (bmap & BUTTON_TOUCH) {
                                handle_touchscreen_event(ABS_MT_TRACKING_ID, EVENT_VALUE_TOUCHSCREEN_PRESS);
                                bmap &= ~BUTTON_TOUCH;
                            }
#endif
                            button_bitmap |= bmap;
                        } else {
#if defined(HAVE_TOUCHSCREEN) && defined(BUTTON_TOUCH)
                            /* Some touchscreens give us actual touch/untouch as a "key" */
                            if (bmap & BUTTON_TOUCH) {
                                handle_touchscreen_event(ABS_MT_TRACKING_ID, 0);
                                bmap &= ~BUTTON_TOUCH;
                            }
#endif
#ifdef BUTTON_DELAY_RELEASE
                            /* Delay the release of any requested buttons */
                            if (bmap & BUTTON_DELAY_RELEASE) {
                                button_delay_release |= bmap & ~BUTTON_DELAY_RELEASE;
                                delay_tick = current_tick + HZ/20;
                                bmap = 0;
                            }
#endif
#ifdef HAVE_SCROLLWHEEL
                            /* Wheel gives us press+release back to back; ignore the release */
                            bmap &= ~(BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD);
#endif
                            button_bitmap &= ~bmap;
                        }
                        break;
                    }
#ifdef HAVE_TOUCHSCREEN
                    case EV_ABS: {
                        if (ts_enabled) {
                            handle_touchscreen_event(event.code, event.value);
                        } else {
                             /* If disabled... ignore */
                            _last_touch_state = TOUCHSCREEN_STATE_UNKNOWN;
                        }
                        break;
                    }
#endif
                    default:
                        /* Ignore other event types */
                        break;
                    }
                }
            }
        }
    }

#ifdef HAVE_SCROLLWHEEL
    /* Reset backlight and poweroff timers */
    if (wheel_ticks) {
#ifdef HAVE_BACKLIGHT
        backlight_on();
#endif
#ifdef HAVE_BUTTON_LIGHT
        buttonlight_on();
#endif
        reset_poweroff_timer();
    }

    if (wheel_ticks > 0)
    {
        while (wheel_ticks-- > 0)
        {
            button_queue_post(BUTTON_SCROLL_FWD, 0);
        }
    }
    else if (wheel_ticks < 0)
    {
        while (wheel_ticks++ < 0)
        {
            button_queue_post(BUTTON_SCROLL_BACK, 0);
        }
    }
#endif /* HAVE_SCROLLWHEEL */

#ifdef HAVE_TOUCHSCREEN
    int touch = touchscreen_to_pixels(_last_x, _last_y, data);

    if(_last_touch_state == TOUCHSCREEN_STATE_DOWN)
    {
        return button_bitmap | touch;
    }
#endif

    return button_bitmap;
}

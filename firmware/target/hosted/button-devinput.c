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

int button_read_device(void)
{
    static int button_bitmap = 0;
    struct input_event event;

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
    while(poll(poll_fds, num_devices, 0))
    {
        for(int i = 0; i < num_devices; i++)
        {
            /* read only if non-blocking */
            if(poll_fds[i].revents & POLLIN)
            {
                int size = read(poll_fds[i].fd, &event, sizeof(event));
                if(size == (int)sizeof(event))
                {
                    /* map linux event code to rockbox button bitmap */
                    int bmap = button_map(event.code);

                    /* event.value == 0x10000 means press
                     * event.value == 0 means release
                     */
                    if(event.value)
                    {
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
                        button_bitmap |= bmap;
                    }
                    else
                    {
#ifdef BUTTON_DELAY_RELEASE
                        /* Delay the release of any requested buttons */
                        if (bmap & BUTTON_DELAY_RELEASE)
                        {
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
            queue_post(&button_queue, BUTTON_SCROLL_FWD, 0);
        }
    }
    else if (wheel_ticks < 0)
    {
        while (wheel_ticks++ < 0)
        {
            queue_post(&button_queue, BUTTON_SCROLL_BACK, 0);
        }
    }
#endif /* HAVE_SCROLLWHEEL */

    return button_bitmap;
}

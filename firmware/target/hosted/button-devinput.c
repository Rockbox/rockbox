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

int button_read_device(void)
{
    static int button_bitmap = 0;
    struct input_event event;

#if defined(BUTTON_SCROLL_BACK)
    // FIXME TODO:  Make this work via HAVE_SCROLL_WHEEL instead

    /* Wheel gives us press+release back to back, clear them after time elapses */
    static long last_tick = 0;
    if (button_bitmap & (BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD) &&
        current_tick - last_tick >= 2)
    {
        button_bitmap &= ~(BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD);
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
                    int keycode = event.code;
                    /* event.value == 0x10000 means press
                     * event.value == 0 means release
                     */
                    bool press = event.value ? true : false;

                    /* map linux event code to rockbox button bitmap */
                    if(press)
                    {
                        int bmap = button_map(keycode);
#if defined(BUTTON_SCROLL_BACK)
                        /* Keep track of when the last wheel tick happened */
                        if (bmap & (BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD))
                            last_tick = current_tick;
#endif
                        button_bitmap |= bmap;
                    }
                    else
                    {
                        int bmap = button_map(keycode);
#if defined(BUTTON_SCROLL_BACK)
                        /* Wheel gives us press+release back to back; ignore the release */
                        bmap &= ~(BUTTON_SCROLL_BACK|BUTTON_SCROLL_FWD);
#endif
                        button_bitmap &= ~bmap;

                    }
                }
            }
        }
    }

    return button_bitmap;
}

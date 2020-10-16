/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
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
//#include <dir.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "sysfs.h"
#include "button.h"
#include "button-target.h"
#include "panic.h"

#define NR_POLL_DESC    2
static struct pollfd poll_fds[NR_POLL_DESC];

static int button_map(int keycode)
{
    switch(keycode)
    {
        case KEY_LEFT:
            return BUTTON_LEFT;

        case KEY_RIGHT:
            return BUTTON_RIGHT;

        case KEY_UP:
            return BUTTON_UP;

        case KEY_DOWN:
            return BUTTON_DOWN;

        case KEY_PLAYPAUSE:
            return BUTTON_SELECT;

        case KEY_VOLUMEUP:
            return BUTTON_VOLUP;

        case KEY_VOLUMEDOWN:
            return BUTTON_VOLDOWN;

        case KEY_POWER:
            return BUTTON_POWER;

        default:
            return 0;
    }
}

void button_init_device(void)
{
    const char * const input_devs[NR_POLL_DESC] = {
        "/dev/input/event0",
        "/dev/input/event1"
    };

    for(int i = 0; i < NR_POLL_DESC; i++)
    {
        int fd = open(input_devs[i], O_RDONLY | O_CLOEXEC);

        if(fd < 0)
        {
            panicf("Cannot open input device: %s\n", input_devs[i]);
        }

        poll_fds[i].fd = fd;
        poll_fds[i].events = POLLIN;
        poll_fds[i].revents = 0;
    }
}

int button_read_device(void)
{
    static int button_bitmap = 0;
    struct input_event event;

    /* check if there are any events pending and process them */
    while(poll(poll_fds, NR_POLL_DESC, 0))
    {
        for(int i = 0; i < NR_POLL_DESC; i++)
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
                        button_bitmap |= button_map(keycode);
                    }
                    else
                    {
                        button_bitmap &= ~button_map(keycode);
                    }
                }
            }
        }
    }

    return button_bitmap;
}

bool headphones_inserted(void)
{
    int status = 0;
    const char * const sysfs_hp_switch = "/sys/class/switch/headset/state";
    sysfs_get_int(sysfs_hp_switch, &status);

    return status ? true : false;
}

void button_close_device(void)
{
    /* close descriptors */
    for(int i = 0; i < NR_POLL_DESC; i++)
    {
        close(poll_fds[i].fd);
    }
}

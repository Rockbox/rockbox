/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2017 Marcin Bukat
 * Copyright (C) 2019 Roman Stolyarov
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

#include "sysfs.h"
#include "button.h"
#include "button-target.h"
#include "panic.h"

#include "kernel.h"

static int key_enter_delay = 0;
static int key_right_delay = 0;
static int key_left_delay = 0;
static int key_power_delay = 0;
static int key_home_delay = 0;
static int key_backspace_delay = 0;
static int key_leftbrace_delay = 0;
static int key_rightbrace_delay = 0;
static int key_up_delay = 0;
static int key_down_delay = 0;
static int key_f12_delay = 0;

#define NR_POLL_DESC	2
static struct pollfd poll_fds[NR_POLL_DESC];

#define DEF_DELAY	5

static int button_map_on(int keycode)
{
    switch(keycode)
    {
        case KEY_ENTER:
            key_enter_delay = DEF_DELAY;
            return BUTTON_PLAY;
        case KEY_F10:
            key_enter_delay = 0;
            return BUTTON_PLAY;

        case KEY_RIGHT:
            key_right_delay = DEF_DELAY;
            return BUTTON_VOL_DOWN;
        case KEY_F7:
            key_right_delay = 0;
            return BUTTON_VOL_DOWN;

        case KEY_LEFT:
            key_left_delay = DEF_DELAY;
            return BUTTON_VOL_UP;
        case KEY_F6:
            key_left_delay = 0;
            return BUTTON_VOL_UP;

        case KEY_POWER:
            key_power_delay = DEF_DELAY;
            return BUTTON_POWER;
        case KEY_F8:
            key_power_delay = 0;
            return BUTTON_POWER;

        case KEY_HOME:
            key_home_delay = DEF_DELAY;
            return BUTTON_OPTION;
        case KEY_F9:
            key_home_delay = 0;
            return BUTTON_OPTION;

        case KEY_BACKSPACE:
            key_backspace_delay = DEF_DELAY;
            return BUTTON_HOME;
        case KEY_NUMLOCK:
            key_backspace_delay = 0;
            return BUTTON_HOME;

        case KEY_LEFTBRACE:
            key_leftbrace_delay = DEF_DELAY;
            return BUTTON_PREV;
        case KEY_F5:
            key_leftbrace_delay = 0;
            return BUTTON_PREV;

        case KEY_RIGHTBRACE:
            key_rightbrace_delay = DEF_DELAY;
            return BUTTON_NEXT;
        case KEY_F4:
            key_rightbrace_delay = 0;
            return BUTTON_NEXT;

        case KEY_UP:
            if (!key_up_delay) key_up_delay = DEF_DELAY;
            return BUTTON_UP;

        case KEY_DOWN:
            if (!key_down_delay) key_down_delay = DEF_DELAY;
            return BUTTON_DOWN;

        case KEY_F12:
            key_f12_delay = DEF_DELAY;
            //return BUTTON_UNLOCK;
            return 0;

        default:
            return 0;
    }
}

static int button_map_off(int keycode)
{
    switch(keycode)
    {
        case KEY_F10:
            return BUTTON_PLAY;

        case KEY_F7:
            return BUTTON_VOL_DOWN;

        case KEY_F6:
            return BUTTON_VOL_UP;

        case KEY_F8:
            return BUTTON_POWER;

        case KEY_F9:
            return BUTTON_OPTION;

        case KEY_NUMLOCK:
            return BUTTON_HOME;

        case KEY_F5:
            return BUTTON_PREV;

        case KEY_F4:
            return BUTTON_NEXT;

        default:
            return 0;
    }
}

static int button_map_timer(void)
{
  int map = 0;

  if (key_enter_delay)
  {
    if (--key_enter_delay == 0) map |= BUTTON_PLAY;
  }
  if (key_right_delay)
  {
    if (--key_right_delay == 0) map |= BUTTON_VOL_DOWN;
  }
  if (key_left_delay)
  {
    if (--key_left_delay == 0) map |= BUTTON_VOL_UP;
  }
  if (key_power_delay)
  {
    if (--key_power_delay == 0) map |= BUTTON_POWER;
  }
  if (key_home_delay)
  {
    if (--key_home_delay == 0) map |= BUTTON_OPTION;
  }
  if (key_backspace_delay)
  {
    if (--key_backspace_delay == 0) map |= BUTTON_HOME;
  }
  if (key_leftbrace_delay)
  {
    if (--key_leftbrace_delay == 0) map |= BUTTON_PREV;
  }
  if (key_rightbrace_delay)
  {
    if (--key_rightbrace_delay == 0) map |= BUTTON_NEXT;
  }
  if (key_up_delay)
  {
    if (--key_up_delay == 0) map |= BUTTON_UP;
  }
  if (key_down_delay)
  {
    if (--key_down_delay == 0) map |= BUTTON_DOWN;
  }
  if (key_f12_delay)
  {
    if (--key_f12_delay == 0) map |= 0; //BUTTON_UNLOCK
  }

  return map;
}

void button_init_device(void)
{
    const char * const input_devs[NR_POLL_DESC] = {
        "/dev/input/event0",
        "/dev/input/event1",
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
    static int map;
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
                    if(event.type == EV_KEY)
                    {
                        int keycode = event.code;

                        /* event.value == 1 means press
                         * event.value == 0 means release
                         */
                        bool press = event.value ? true : false;

                        if(press)
                        {
                            map = button_map_on(keycode);
                            if(map) button_bitmap |= map;
                        }
                        else
                        {
                            map = button_map_off(keycode);
                            if(map) button_bitmap &= ~map;
                        }
                    }
                }
            }
        }
    }

    map = button_map_timer();
    if(map) button_bitmap &= ~map;

    return button_bitmap;
}

bool headphones_inserted(void)
{
    int status = 0;
    const char * const sysfs_hs_switch = "/sys/class/misc/axp173/headset_state";

    sysfs_get_int(sysfs_hs_switch, &status);
    if (status) return true;

    return false;
}

void button_close_device(void)
{
    /* close descriptors */
    for(int i = 0; i < NR_POLL_DESC; i++)
    {
        close(poll_fds[i].fd);
    }
}

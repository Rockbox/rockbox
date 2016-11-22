/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 Amaury Pouly
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
#define LOGF_ENABLE
#include "logf.h"
#include "panic.h"
#include "backlight.h"

#include "nwz_keys.h"
#include "nwz_ts.h"

#include <poll.h>
#include <dir.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/input.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

/* device types */
#define DEV_KEY     0   /* icx_keys driver */
#define DEV_TOUCH   1   /* icx_touch_screen driver */

/* HOLD status */
static bool hold_status;
/* button bitmap */
static int button_bitmap;
/* poll() descriptors (up to 2 for now: keys and touchscreen) */
#define NR_POLL_DESC    2
static struct pollfd poll_fds[NR_POLL_DESC];
static nfds_t poll_nfds;
int dev_type[NR_POLL_DESC]; /* DEV_* */

static void open_input_device(const char *path)
{
    int fd = open(path, O_RDWR);
    if(fd < 0)
        return;
    /* query name */
    char name[256];
    if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
    {
        close(fd);
        return;
    }
    logf("%s: %s", path, name);

    if(strcmp(name, NWZ_KEY_NAME) == 0)
        dev_type[poll_nfds] = DEV_KEY;
    else if(strcmp(name, NWZ_TS_NAME) == 0)
        dev_type[poll_nfds] = DEV_TOUCH;
    else
    {
        /* only keep devices we know about */
        close(fd);
        return;
    }
    /* if we found a key driver, we can read the hold status from it (and keep
     * it updated with events) */
    if(dev_type[poll_nfds] == DEV_KEY)
    {
        /* HOLD is reported as the first LED */
        unsigned long led_hold = 0;
        if(ioctl(fd, EVIOCGLED(sizeof(led_hold)), &led_hold) < 0)
            logf("cannot read HOLD status: %s", strerror(errno));
        hold_status = !!led_hold;
    }
    /* fill poll descriptor */
    poll_fds[poll_nfds].fd = fd;
    poll_fds[poll_nfds].events = POLLIN;
    poll_fds[poll_nfds].revents = 0;
    poll_nfds++;
}

/* keycode -> rockbox button mapping */
static int button_map[NWZ_KEY_MASK + 1] =
{
    [0 ... NWZ_KEY_MASK] = 0,
    [NWZ_KEY_PLAY] = BUTTON_PLAY,
    [NWZ_KEY_RIGHT] = BUTTON_RIGHT,
    [NWZ_KEY_LEFT] = BUTTON_LEFT,
    [NWZ_KEY_UP] = BUTTON_UP,
    [NWZ_KEY_DOWN] = BUTTON_DOWN,
    [NWZ_KEY_ZAPPIN] = 0,
    [NWZ_KEY_AD0_6] = 0,
    [NWZ_KEY_AD0_7] = 0,
    [NWZ_KEY_NONE] = 0,
    [NWZ_KEY_VOL_DOWN] = BUTTON_VOL_DOWN,
    [NWZ_KEY_VOL_UP] = BUTTON_VOL_UP,
    [NWZ_KEY_BACK] = BUTTON_BACK,
    [NWZ_KEY_OPTION] = BUTTON_POWER,
    [NWZ_KEY_BT] = 0,
    [NWZ_KEY_AD1_5] = 0,
    [NWZ_KEY_AD1_6] = 0,
    [NWZ_KEY_AD1_7] = 0,
};

static void handle_key(struct input_event evt)
{
    /* See headers/nwz_keys.h for explanation of Sony's nonstandard interface */
    int keycode = evt.code & NWZ_KEY_MASK;
    bool press = (evt.value == 0);
    if(press)
        button_bitmap |= button_map[keycode];
    else
        button_bitmap &= ~button_map[keycode];
#ifndef BOOTLOADER
    bool new_hold_status = !!(evt.code & NWZ_KEY_HOLD_MASK);
    if(new_hold_status != hold_status)
    {
        hold_status = new_hold_status;
        backlight_hold_changed(hold_status);
    }
#endif
}

static void handle_touch(struct input_event evt)
{
    /* TODO */
    (void) evt;
}

bool button_hold(void)
{
    return hold_status;
}

void button_init_device(void)
{
    const char *input_path = "/dev/input";
    char device_name[PATH_MAX];
    /* find what input devices are available */
    DIR* input_dir = opendir(input_path);
    if(input_dir == NULL)
        panicf("Cannot read /dev/input directory: %s", strerror(errno));
    strcpy(device_name, input_path);
    strcat(device_name, "/");
    char *device_name_p = device_name + strlen(device_name);
    struct dirent *dir_entry;
    while((dir_entry = readdir(input_dir)))
    {
        /* skip '.' and '..' entries */
        if(strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
            continue;
        /* create device full path and open it */
        strcpy(device_name_p, dir_entry->d_name);
        open_input_device(device_name);
    }
    closedir(input_dir);
    /* check if we have at least one device */
    if(poll_nfds == 0)
        panicf("No input device found");
}

int button_read_device(void)
{
    struct input_event event;
    /* check if there are any events pending and process them */
    while(true)
    {
        /* stop when there are no more events */
        if(poll(poll_fds, poll_nfds, 0) == 0)
            break;
        for(unsigned int i = 0; i < poll_nfds; i++)
        {
            /* only read if we won't block */
            if(!(poll_fds[i].revents & POLLIN))
                continue;
            if(read(poll_fds[i].fd, &event, sizeof(event)) != (int)sizeof(event))
                continue;
            if(dev_type[i] == DEV_KEY)
                handle_key(event);
            else if(dev_type[i] == DEV_TOUCH)
                handle_touch(event);
        }
    }
    return hold_status ? 0 : button_bitmap;
}


void button_close_device(void)
{
    /* close descriptors */
    for(unsigned int i = 0; i < poll_nfds; i++)
        close(poll_fds[i].fd);
}

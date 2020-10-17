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

#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <system.h>

#include "button.h"
//#define LOGF_ENABLE
#include "logf.h"
#include "panic.h"
#include "backlight.h"

#include "nwz_keys.h"
#include "nwz_ts.h"


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

#ifdef HAVE_TOUCHSCREEN
/* structure to track touch state */
static struct
{
    int x, y; /* current position (valid is touch is true) */
    int max_x, max_y; /* maximum possible values */
    int pressure, tool_width; /* current pressure and tool width */
    int max_pressure, max_tool_width; /* maximum possible values */
    bool touch; /* is the user touching the screen? */
    /* the hardware supports "flick" gesture */
    bool flick; /* was the action a flick? */
    int flick_x, flick_y; /* if so, this is the flick direction */
}ts_state;
/* rockbox state, updated from ts state on SYN event */
static int touch_x, touch_y;
static bool touch_detect;

/* get touchscreen information and init state */
int ts_init_state(int fd)
{
    memset(&ts_state, 0, sizeof(ts_state));
    struct input_absinfo info;
    if(ioctl(fd, EVIOCGABS(ABS_X), &info) < 0)
        return -1;
    ts_state.max_x = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_Y), &info) < 0)
        return -1;
    ts_state.max_y = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_PRESSURE), &info) < 0)
        return -1;
    ts_state.max_pressure = info.maximum;
    if(ioctl(fd, EVIOCGABS(ABS_TOOL_WIDTH), &info) < 0)
        return -1;
    ts_state.max_tool_width = info.maximum;
    touch_detect = false;
    return 0;
}

void handle_touch(struct input_event evt)
{
    switch(evt.type)
    {
        case EV_SYN:
            /* on SYN, we copy the state to the rockbox state */
            touch_x = ts_state.x;
            touch_y = ts_state.y;
            /* map coordinate to screen */
            touch_x = touch_x * LCD_WIDTH / ts_state.max_x;
            touch_y = touch_y * LCD_HEIGHT / ts_state.max_y;
            /* don't trust driver reported ranges */
            touch_x = MAX(0, MIN(touch_x, LCD_WIDTH - 1));
            touch_y = MAX(0, MIN(touch_y, LCD_HEIGHT - 1));
            touch_detect = ts_state.touch;
            /* reset flick */
            ts_state.flick = false;
            break;
        case EV_REL:
            if(evt.code == REL_RX)
                ts_state.flick_x = evt.value;
            else if(evt.code == REL_RY)
                ts_state.flick_y = evt.value;
            else
                break;
            ts_state.flick = true;
            break;
        case EV_ABS:
            if(evt.code == ABS_X)
                ts_state.x = evt.value;
            else if(evt.code == ABS_Y)
                ts_state.y = evt.value;
            else if(evt.code == ABS_PRESSURE)
                ts_state.pressure = evt.value;
            else if(evt.code == ABS_TOOL_WIDTH)
                ts_state.tool_width = evt.value;
            break;
        case EV_KEY:
            if(evt.code == BTN_TOUCH)
                ts_state.touch = evt.value;
            break;
        default:
            break;
    }
}
#endif

static void load_hold_status(int fd)
{
    /* HOLD is reported as the first LED */
    unsigned long led_hold = 0;
    if(ioctl(fd, EVIOCGLED(sizeof(led_hold)), &led_hold) < 0)
        logf("cannot read HOLD status: %s", strerror(errno));
    hold_status = !!led_hold;
}

static void key_init_state(int fd)
{
    /* the driver knows the HOLD statu at all times */
    load_hold_status(fd);
    /* the driver can be queried for button status but the output is garbage
     * so just assume no keys are pressed */
    button_bitmap = 0;
}

static void open_input_device(const char *path)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if(fd < 0)
        return;
    /* query name */
    char name[256];
    if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
    {
        close(fd);
        return;
    }

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
        key_init_state(fd);
#ifdef HAVE_TOUCHSCREEN
    else if(dev_type[poll_nfds] == DEV_TOUCH)
        ts_init_state(fd);
#endif
    /* fill poll descriptor */
    poll_fds[poll_nfds].fd = fd;
    poll_fds[poll_nfds].events = POLLIN;
    poll_fds[poll_nfds].revents = 0;
    poll_nfds++;
}

#if defined(SONY_NWZA860)
/* keycode -> rockbox button mapping */
static int button_map[NWZ_KEY_MASK + 1] =
{
    [0 ... NWZ_KEY_MASK] = 0,
    [NWZ_KEY_PLAY] = BUTTON_PLAY,
    [NWZ_KEY_RIGHT] = BUTTON_FF,
    [NWZ_KEY_LEFT] = BUTTON_REW,
    [NWZ_KEY_VOL_DOWN] = BUTTON_VOL_DOWN,
    [NWZ_KEY_VOL_UP] = BUTTON_VOL_UP,
    [NWZ_KEY_BACK] = BUTTON_BACK,
};
#else /* SONY_NWZA860 */
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
#endif /* SONY_NWZA860 */

static void handle_key(struct input_event evt)
{
    /* See headers/nwz_keys.h for explanation of Sony's nonstandard interface */
    int keycode = evt.code & NWZ_KEY_MASK;
    bool press = (evt.value == 0);
    if(press)
        button_bitmap |= button_map[keycode];
    else
        button_bitmap &= ~button_map[keycode];
    bool new_hold_status = !!(evt.code & NWZ_KEY_HOLD_MASK);
    if(new_hold_status != hold_status)
    {
        hold_status = new_hold_status;
#ifndef BOOTLOADER
        backlight_hold_changed(hold_status);
#endif
    }
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

int button_read_device(
#ifdef HAVE_BUTTON_DATA
    int *data
#else
    void
#endif
    )
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
#ifdef HAVE_TOUCHSCREEN
            else if(dev_type[i] == DEV_TOUCH)
                handle_touch(event);
#endif
        }
    }
    int btns = button_bitmap;
#ifdef HAVE_TOUCHSCREEN
    /* WARNING we must call touchscreen_to_pixels even if there is no touch,
     * otherwsise *data is not filled with the last position and it breaks
     * everything */
    int touch_bitmap = touchscreen_to_pixels(touch_x, touch_y, data);
    if(touch_detect)
        btns |= touch_bitmap;
#endif
    return hold_status ? 0 : btns;
}

void nwz_button_reload_after_suspend(void)
{
    /* reinit everything, particularly important for keys and HOLD */
    for(unsigned int i = 0; i < poll_nfds; i++)
    {
        if(dev_type[i] == DEV_KEY)
            key_init_state(poll_fds[i].fd);
#ifdef HAVE_TOUCHSCREEN
        else if(dev_type[i] == DEV_TOUCH)
            ts_init_state(poll_fds[i].fd);
#endif
    }
}

void button_close_device(void)
{
    /* close descriptors */
    for(unsigned int i = 0; i < poll_nfds; i++)
        close(poll_fds[i].fd);
}

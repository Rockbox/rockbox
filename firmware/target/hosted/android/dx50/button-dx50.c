/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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


#include <stdbool.h>
#include "button.h"
#include "buttonmap.h"
#include "config.h"
#include "kernel.h"
#include "system.h"
#include "touchscreen.h"
#include "powermgmt.h"
#include "backlight.h"

#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <errno.h>


static struct pollfd *ufds;
static char **device_names;
static int nfds;

enum {
    PRINT_DEVICE_ERRORS     = 1U << 0,
    PRINT_DEVICE            = 1U << 1,
    PRINT_DEVICE_NAME       = 1U << 2,
    PRINT_DEVICE_INFO       = 1U << 3,
    PRINT_VERSION           = 1U << 4,
    PRINT_POSSIBLE_EVENTS   = 1U << 5,
    PRINT_INPUT_PROPS       = 1U << 6,
    PRINT_HID_DESCRIPTOR    = 1U << 7,

    PRINT_ALL_INFO          = (1U << 8) - 1,

    PRINT_LABELS            = 1U << 16,
};

static int last_y, last_x;
static int last_btns;

static enum {
    STATE_UNKNOWN,
    STATE_UP,
    STATE_DOWN
} last_touch_state = STATE_UNKNOWN;


static int open_device(const char *device, int print_flags)
{
    int fd;
    struct pollfd *new_ufds;
    char **new_device_names;

    fd = open(device, O_RDWR);
    if(fd < 0) {
        if(print_flags & PRINT_DEVICE_ERRORS)
            fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }

    new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL) {
        fprintf(stderr, "out of memory\n");
        close(fd);
        return -1;
    }
    ufds = new_ufds;
    new_device_names = realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL) {
        fprintf(stderr, "out of memory\n");
        close(fd);
        return -1;
    }
    device_names = new_device_names;

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(device);
    nfds++;

    return 0;
}



static int scan_dir(const char *dirname, int print_flags)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        open_device(devname, print_flags);
    }
    closedir(dir);
    return 0;
}

bool _hold;

bool button_hold()
{
    FILE *f = fopen("/sys/class/axppower/holdkey", "r");
    char x;
    fscanf(f, "%c", &x);
    fclose(f);
    _hold = !(x&STATE_UNLOCKED);
    return _hold;
}


void button_init_device(void)
{
    int res;
    int print_flags = 0;
    const char *device = NULL;
    const char *device_path = "/dev/input";

    nfds = 1;
    ufds = calloc(1, sizeof(ufds[0]));
    ufds[0].fd = inotify_init();
    ufds[0].events = POLLIN;
    if(device) {
        res = open_device(device, print_flags);
        if(res < 0) {
            // return 1;
        }
    } else {
        res = inotify_add_watch(ufds[0].fd, device_path, IN_DELETE | IN_CREATE);
        if(res < 0) {
            fprintf(stderr, "could not add watch for %s, %s\n", device_path, strerror(errno));
            // return 1;
        }
        res = scan_dir(device_path, print_flags);
        if(res < 0) {
            fprintf(stderr, "scan dir failed for %s\n", device_path);
            // return 1;
        }
    }

    button_hold();    //store state

    set_rockbox_ready();
}

void touchscreen_enable_device(bool en)
{
    (void)en; /* FIXME: do something smart */
}


int keycode_to_button(int keyboard_key)
{
    switch(keyboard_key){
        case KEYCODE_PWR:
            return BUTTON_POWER;

        case KEYCODE_PWR_LONG:
            return BUTTON_POWER_LONG;

        case KEYCODE_VOLPLUS:
            return BUTTON_VOL_UP;

        case KEYCODE_VOLMINUS:
            return BUTTON_VOL_DOWN;

        case KEYCODE_PREV:
            return BUTTON_LEFT;

        case KEYCODE_NEXT:
            return BUTTON_RIGHT;

        case KEYCODE_PLAY:
            return BUTTON_PLAY;

        case KEYCODE_HOLD:
            button_hold();    /* store state */
            backlight_hold_changed(_hold);
            return BUTTON_NONE;

        default:
            return BUTTON_NONE;
    }
}


int button_read_device(int *data)
{
    int i;
    int res;
    struct input_event event;
    int read_more;
    unsigned button = 0;

    if(last_btns & BUTTON_POWER_LONG)
    {
        return last_btns;   /* simulate repeat */
    }

    do {
        read_more = 0;
        poll(ufds, nfds, 10);
        for(i = 1; i < nfds; i++) {
            if(ufds[i].revents & POLLIN) {
                res = read(ufds[i].fd, &event, sizeof(event));
                if(res < (int)sizeof(event)) {
                    fprintf(stderr, "could not get event\n");
                }

                switch(event.type)
                {
                    case 1: /* HW-Button */
                        button = keycode_to_button(event.code);
                        if (_hold) /* we have to wait for keycode_to_button() first to maybe clear hold state */
                            break;
                        if (button == BUTTON_NONE)
                        {
                            last_btns = button;
                            break;
                        }
/* workaround for a wrong feedback, only present with DX90 */
#if defined(DX90)
                       if (button == BUTTON_RIGHT && (last_btns & BUTTON_LEFT == BUTTON_LEFT) && !event.value)
                        {
                            button = BUTTON_LEFT;
                        }
#endif
                        if (event.value)
                            last_btns |= button;
                        else
                            last_btns &= (~button);

                        break;

                    case 3:  /* Touchscreen */
                        if(_hold)
                            break;

                        switch(event.code)
                        {
                            case 53: /* x -> next will be y */
                                last_x = event.value;
                                read_more = 1;
                            break;
                            case 54: /* y */
                                last_y = event.value;
                            break;
                            case 57: /* press -> next will be x */
                                if(event.value==1)
                                {
                                    last_touch_state = STATE_DOWN;
                                    read_more = 1;
                                }
                                else
                                    last_touch_state = STATE_UP;
                            break;
                        }
                    break;
                }
            }
        }
    } while(read_more);


    /* Get grid button/coordinates based on the current touchscreen mode
     *
     * Caveat: the caller seemingly depends on *data always being filled with
     *         the last known touchscreen position, so always call
     *         touchscreen_to_pixels() */
    int touch = touchscreen_to_pixels(last_x, last_y, data);

    if (last_touch_state == STATE_DOWN)
        return last_btns | touch;

    return last_btns;
}


/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>

#include "config.h"
#include "backlight.h"
#include "button.h"
#include "debug.h"
#include "panic.h"
#include "touchscreen.h"

#include "button-ibasso.h"
#include "button-target.h"
#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


#define EVENT_TYPE_BUTTON 1

/* /dev/input/event0 */
#define EVENT_CODE_BUTTON_LINEOUT  113
#define EVENT_CODE_BUTTON_SPDIF    114
#define EVENT_CODE_BUTTON_HOLD     115

/* /dev/input/event1 */
#define EVENT_CODE_BUTTON_SDCARD   143


#define EVENT_TYPE_TOUCHSCREEN 3

/* /dev/input/event2 */
#define EVENT_CODE_TOUCHSCREEN_X 53
#define EVENT_CODE_TOUCHSCREEN_Y 54
#define EVENT_CODE_TOUCHSCREEN   57

#define EVENT_VALUE_TOUCHSCREEN_PRESS    1
#define EVENT_VALUE_TOUCHSCREEN_RELEASE -1


/*
    Changing bit, when hold switch is toggled.
    Bit is off when hold switch is engaged.
*/
#define HOLD_SWITCH_BIT 16

/*
    Changing bit, when coaxial out is plugged.
    Bit is off when coaxial out is plugged in.
*/
#define COAX_BIT 32

/*
    Changing bit, when line out is plugged.
    Bit is off when line out is plugged in.
*/
#define SPDIF_BIT 64


/* State of the hold switch; true: hold switch engaged. */
static bool _hold = false;


/* See button.h. */
bool button_hold(void)
{
    char hold_state;
    if(! sysfs_get_char(SYSFS_HOLDKEY, &hold_state))
    {
        DEBUGF("ERROR %s: Can not get hold switch state.", __func__);
        hold_state = HOLD_SWITCH_BIT;
    }

    /*DEBUGF("%s: hold_state: %d, %c.", __func__, hold_state, hold_state);*/

    /*bool coax_connected = ! (hold_state & COAX_BIT);
    bool spdif_connected = ! (hold_state & SPDIF_BIT);*/

    _hold = ! (hold_state & HOLD_SWITCH_BIT);

    /*DEBUGF("%s: _hold: %d, coax_connected: %d, spdif_connected: %d.", __func__, _hold, coax_connected, spdif_connected);*/

    return _hold;
}


/* Input devices monitored with poll API. */
static struct pollfd* _fds = NULL;


/* Number of input devices monitored with poll API. */
static nfds_t _nfds = 0;


/* The names of the devices in _fds. */
static char** _device_names = NULL;


/* Open device device_name and add it to the list of polled devices. */
static bool open_device(const char* device_name)
{
    int fd = open(device_name, O_RDONLY);
    if(fd == -1)
    {
        DEBUGF("ERROR %s: open failed on %s.", __func__, device_name);
        return false;
    }

    struct pollfd* new_fds = realloc(_fds, sizeof(struct pollfd) * (_nfds + 1));
    if(new_fds == NULL)
    {
        DEBUGF("ERROR %s: realloc for _fds failed.", __func__);
        panicf("ERROR %s: realloc for _fds failed.", __func__);
        return false;
    }

    _fds = new_fds;
    _fds[_nfds].fd = fd;
    _fds[_nfds].events = POLLIN;

    char** new_device_names = realloc(_device_names, sizeof(char*) * (_nfds + 1));
    if(new_device_names == NULL)
    {
        DEBUGF("ERROR %s: realloc for _device_names failed.", __func__);
        panicf("ERROR %s: realloc for _device_names failed.", __func__);
        return false;
    }

    _device_names = new_device_names;
    _device_names[_nfds] = strdup(device_name);
    if(_device_names[_nfds] == NULL)
    {
        DEBUGF("ERROR %s: strdup failed.", __func__);
        panicf("ERROR %s: strdup failed.", __func__);
        return false;
    }

    ++_nfds;

    DEBUGF("DEBUG %s: Opened device %s.", __func__, device_name);

    return true;
}


/* See button.h. */
void button_init_device(void)
{
    TRACE;

    if((_fds != NULL) || (_nfds != 0) || (_device_names != NULL))
    {
        DEBUGF("ERROR %s: Allready initialized.", __func__);
        panicf("ERROR %s: Allready initialized.", __func__);
        return;
    }

    /* The input device directory. */
    static const char device_path[] = "/dev/input";

    /* Path delimeter. */
    static const char delimeter[] = "/";

    /* Open all devices in device_path. */
    DIR* dir = opendir(device_path);
    if(dir == NULL)
    {
        DEBUGF("ERROR %s: opendir failed: errno: %d.", __func__, errno);
        panicf("ERROR %s: opendir failed: errno: %d.", __func__, errno);
        return;
    }

    char device_name[PATH_MAX];
    strcpy(device_name, device_path);
    strcat(device_name, delimeter);
    char* device_name_idx = device_name + strlen(device_name);

    struct dirent* dir_entry;
    while((dir_entry = readdir(dir)))
    {
        if(   ((dir_entry->d_name[0] == '.') && (dir_entry->d_name[1] == '\0'))
           || ((dir_entry->d_name[0] == '.') && (dir_entry->d_name[1] == '.') && (dir_entry->d_name[2] == '\0')))
        {
            continue;
        }

        strcpy(device_name_idx, dir_entry->d_name);

        /* Open and add device to _fds. */
        open_device(device_name);
    }

    closedir(dir);

    /* Sanity check. */
    if(_nfds < 2)
    {
        DEBUGF("ERROR %s: No input devices.", __func__);
        panicf("ERROR %s: No input devices.", __func__);
        return;
    }

    /*
        Hold switch has a separate interface for its state.
        Input events just report that it has been toggled, but not the state.
    */
    button_hold();
}


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


static bool handle_touchscreen_event(__u16 code, __s32 value)
{
    bool read_more = false;

    switch(code)
    {
        case EVENT_CODE_TOUCHSCREEN_X:
        {
            _last_x = value;

            /* x -> next will be y. */
            read_more = true;

            break;
        }

        case EVENT_CODE_TOUCHSCREEN_Y:
        {
            _last_y = value;
            break;
        }

        case EVENT_CODE_TOUCHSCREEN:
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


/* Last known hardware buttons pressed. */
static int _last_btns = BUTTON_NONE;


/* See button.h. */
int button_read_device(int *data)
{
    bool read_more = true;
    while(read_more)
    {
        read_more = false;

        /* Poll all input devices. */
        poll(_fds, _nfds, 0);

        for(nfds_t fds_idx = 0; fds_idx < _nfds; ++fds_idx)
        {
            if(! (_fds[fds_idx].revents & POLLIN))
            {
                continue;
            }

            struct input_event event;
            if(read(_fds[fds_idx].fd, &event, sizeof(event)) < (int) sizeof(event))
            {
                DEBUGF("ERROR %s: Read of input devices failed.", __func__);
                continue;
            }

            /*DEBUGF("DEBUG %s: device: %s, event.type: %d, event.code: %d, event.value: %d", __func__, _device_names[fds_idx], event.type, event.code, event.value);*/

            switch(event.type)
            {
                case EVENT_TYPE_BUTTON:
                {
                    if(event.code == EVENT_CODE_BUTTON_HOLD)
                    {
                        /* Hold switch toggled, update hold switch state. */
                        button_hold();
                        backlight_hold_changed(_hold);

                        _last_btns = BUTTON_NONE;
                        break;
                    }

                    _last_btns = handle_button_event(event.code, event.value, _last_btns);

                    if(_hold)
                    {
                        /* Hold switch engaged. Ignore all button events. */
                        _last_btns = BUTTON_NONE;
                    }

                    /*DEBUGF("DEBUG %s: _last_btns: %#8.8x", __func__, _last_btns);*/
                    break;
                }

                case EVENT_TYPE_TOUCHSCREEN:
                {
                    if(_hold)
                    {
                        /* Hold switch engaged, ignore all touchscreen events. */
                        _last_touch_state = TOUCHSCREEN_STATE_UNKNOWN;
                        _last_btns        = BUTTON_NONE;
                    }
                    else
                    {
                        read_more = handle_touchscreen_event(event.code, event.value);
                        /*DEBUGF("DEBUG %s: _last_touch_state: %d, _last_x: %d, _last_y: %d, read_more: %s", __func__, _last_touch_state, _last_x, _last_y, read_more ? "true" : "false");*/
                    }
                    break;
                }
            }
        }
    }

    /*
        Get grid button/coordinates based on the current touchscreen mode
        Caveat: The caller seemingly depends on *data always being filled with
                the last known touchscreen position, so always call
                touchscreen_to_pixels().
    */
    int touch = touchscreen_to_pixels(_last_x, _last_y, data);

    if(_last_touch_state == TOUCHSCREEN_STATE_DOWN)
    {
        return _last_btns | touch;
    }

    /*DEBUGF("DEBUG %s: _last_btns: %#8.8x.", __func__, _last_btns);*/

    return _last_btns;
}


void button_close_device(void)
{
    TRACE;

    if(_fds)
    {
        for(nfds_t fds_idx = 0; fds_idx < _nfds; ++fds_idx)
        {
            close(_fds[fds_idx].fd);
        }
        free(_fds);
        _fds = NULL;
    }

    if(_device_names)
    {
        for(nfds_t fds_idx = 0; fds_idx < _nfds; ++fds_idx)
        {
            free(_device_names[fds_idx]);
        }
        free(_device_names);
        _device_names = NULL;
    }

    _nfds = 0;
}

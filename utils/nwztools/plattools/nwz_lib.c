/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "nwz_lib.h"

void nwz_run(const char *file, const char *args[], bool wait)
{
    pid_t child_pid = fork();
    if(child_pid != 0)
    {
        if(wait)
        {
            int status;
            waitpid(child_pid, &status, 0);
        }
    }
    else
    {
        execvp(file, (char * const *)args);
        _exit(1);
    }
}

void nwz_lcdmsg(bool clear, int x, int y, const char *msg)
{
    const char *path_lcdmsg = "/usr/local/bin/lcdmsg";
    const char *args[16];
    int index = 0;
    char locate[32];
    args[index++] = path_lcdmsg;
    if(clear)
        args[index++] = "-c";
    args[index++] = "-f";
    args[index++] = "/usr/local/bin/font_08x12.bmp";
    args[index++] = "-l";
    sprintf(locate, "%d,%d", x, y);
    args[index++] = locate;
    args[index++] = msg;
    args[index++] = NULL;
    /* wait for lcdmsg to finish to avoid any race conditions in framebuffer
     * accesses */
    nwz_run(path_lcdmsg, args, true);
}

void nwz_lcdmsgf(bool clear, int x, int y, const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    nwz_lcdmsg(clear, x, y, buffer);
}

int nwz_key_open(void)
{
    return open("/dev/input/event0", O_RDONLY);
}

void nwz_key_close(int fd)
{
    close(fd);
}

int nwz_key_get_hold_status(int fd)
{
    unsigned long led_hold = 0;
    if(ioctl(fd, EVIOCGLED(sizeof(led_hold)), &led_hold) < 0)
        return -1;
    return led_hold;
}

int nwz_key_wait_event(int fd, long tmo_us)
{
    fd_set rfds;
    struct timeval tv;
    struct timeval *tv_ptr = NULL;
    /* watch the input device */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    /* setup timeout */
    if(tmo_us >= 0)
    {
        tv.tv_sec = 0;
        tv.tv_usec = tmo_us;
        tv_ptr = &tv;
    }
    return select(fd + 1, &rfds, NULL, NULL, tv_ptr);
}

int nwz_key_read_event(int fd, struct input_event *evt)
{
    int ret = read(fd, evt, sizeof(struct input_event));
    if(ret != sizeof(struct input_event))
        return -1;
    return 1;
}

int nwz_key_event_get_keycode(struct input_event *evt)
{
    return evt->code & NWZ_KEY_MASK;
}

bool nwz_key_event_is_press(struct input_event *evt)
{
    return evt->value == 0;
}

bool nwz_key_event_get_hold_status(struct input_event *evt)
{
    return !!(evt->code & NWZ_KEY_HOLD_MASK);
}

static const char *nwz_keyname[NWZ_KEY_MASK + 1] =
{
    [0 ... NWZ_KEY_MASK] = "unknown",
    [NWZ_KEY_PLAY] = "PLAY",
    [NWZ_KEY_RIGHT] = "RIGHT",
    [NWZ_KEY_LEFT] = "LEFT",
    [NWZ_KEY_UP] = "UP",
    [NWZ_KEY_DOWN] = "DOWN",
    [NWZ_KEY_ZAPPIN] = "ZAPPIN",
    [NWZ_KEY_AD0_6] = "AD0_6",
    [NWZ_KEY_AD0_7] = "AD0_7",
    [NWZ_KEY_NONE] = "NONE",
    [NWZ_KEY_VOL_DOWN] = "VOL DOWN",
    [NWZ_KEY_VOL_UP] = "VOL UP",
    [NWZ_KEY_BACK] = "BACK",
    [NWZ_KEY_OPTION] = "OPTION",
    [NWZ_KEY_BT] = "BT",
    [NWZ_KEY_AD1_5] = "AD1_5",
    [NWZ_KEY_AD1_6] = "AD1_6",
    [NWZ_KEY_AD1_7] = "AD1_7",
};

const char *nwz_key_get_name(int keycode)
{
    if(keycode <0 || keycode > NWZ_KEY_MASK)
        return "invalid";
    else
        return nwz_keyname[keycode];
}

int nwz_fb_open(bool lcd)
{
    return open(lcd ? "/dev/fb/0" : "/dev/fb/1", O_RDWR);
}

void nwz_fb_close(int fd)
{
    close(fd);
}

int nwz_fb_get_brightness(int fd, struct nwz_fb_brightness *bl)
{
    if(ioctl(fd, NWZ_FB_GET_BRIGHTNESS, bl) < 0)
        return -1;
    else
        return 1;
}

int nwz_fb_set_brightness(int fd, struct nwz_fb_brightness *bl)
{
    if(ioctl(fd, NWZ_FB_SET_BRIGHTNESS, bl) < 0)
        return -1;
    else
        return 1;
}

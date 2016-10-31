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
#include "nwz_plattools.h"

int NWZ_TOOL_MAIN(test_bl)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_bl");
    nwz_lcdmsg(false, 0, 2, "LEFT/RIGHT: level");
    nwz_lcdmsg(false, 0, 3, "UP/DOWN: step");
    nwz_lcdmsg(false, 0, 4, "VOL UP/DOWN: period");
    nwz_lcdmsg(false, 0, 5, "BACK: quit");
    /* open input and framebuffer device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 7, "Cannot open input device");
        sleep(2);
        return 1;
    }
    int fb_fd = nwz_fb_open(true);
    if(fb_fd < 0)
    {
        nwz_key_close(input_fd);
        nwz_lcdmsg(false, 3, 7, "Cannot open framebuffer device");
        sleep(2);
        return 1;
    }
    /* display input state in a loop */
    while(1)
    {
        struct nwz_fb_brightness bl;
        if(nwz_fb_get_brightness(fb_fd, &bl) == 1)
        {
            nwz_lcdmsgf(false, 1, 7, "level: %d   ", bl.level);
            nwz_lcdmsgf(false, 1, 8, "step: %d   ", bl.step);
            nwz_lcdmsgf(false, 1, 9, "period: %d   ", bl.period);
        }
        /* wait for event */
        int ret = nwz_key_wait_event(input_fd, -1);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        int code = nwz_key_event_get_keycode(&evt);
        bool press = nwz_key_event_is_press(&evt);
        /* only act on release */
        if(press)
            continue;
        if(code == NWZ_KEY_BACK)
            break; /* quit */
        bool change_bl = false;
        if(code == NWZ_KEY_RIGHT && bl.level < NWZ_FB_BL_MAX_LEVEL)
        {
            change_bl = true;
            bl.level++;
        }
        else if(code == NWZ_KEY_LEFT && bl.level > NWZ_FB_BL_MIN_LEVEL)
        {
            change_bl = true;
            bl.level--;
        }
        else if(code == NWZ_KEY_UP && bl.step < NWZ_FB_BL_MAX_STEP)
        {
            change_bl = true;
            bl.step++;
        }
        else if(code == NWZ_KEY_DOWN && bl.step > NWZ_FB_BL_MIN_STEP)
        {
            change_bl = true;
            bl.step--;
        }
        else if(code == NWZ_KEY_VOL_UP && bl.period < 100) /* artificial bound on period */
        {
            change_bl = true;
            bl.period++;
        }
        else if(code == NWZ_KEY_VOL_DOWN && bl.period > NWZ_FB_BL_MIN_PERIOD)
        {
            change_bl = true;
            bl.period--;
        }
        /* change bl */
        if(change_bl)
            nwz_fb_set_brightness(fb_fd, &bl);
    }
    /* close input device */
    nwz_key_close(input_fd);
    nwz_fb_close(fb_fd);
    /* finish nicely */
    return 0;
}

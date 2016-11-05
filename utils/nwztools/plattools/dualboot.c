/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
 *
 * Based on Rockbox iriver bootloader by Linus Nielsen Feltzing
 * and the ipodlinux bootloader by Daniel Palffy and Bernard Leach
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
#include <time.h>

/* Important Note: this bootloader is carefully written so that in case of
 * error, the OF is run. This seems like the safest option since the OF is
 * always there and might do magic things. */

enum boot_mode
{
    BOOT_ROCKBOX,
    BOOT_TOOLS,
    BOOT_OF
};

enum boot_mode get_boot_mode(void)
{
    /* get time */
    struct timeval deadline;
    if(gettimeofday(&deadline, NULL) != 0)
    {
        nwz_lcdmsg(false, 0, 2, "Cannot get time");
        sleep(2);
        return BOOT_OF;
    }
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 0, 2, "Cannot open input device");
        sleep(2);
        return BOOT_OF;
    }
    deadline.tv_sec += 5;
    /* wait for user action */
    enum boot_mode mode = BOOT_OF;
    while(true)
    {
        /* get time */
        struct timeval cur_time;
        if(gettimeofday(&cur_time, NULL) != 0)
        {
            nwz_lcdmsg(false, 0, 4, "Cannot get time");
            sleep(2);
            break;
        }
        /* check timeout */
        if(cur_time.tv_sec > deadline.tv_sec)
            break;
        if(cur_time.tv_sec == deadline.tv_sec && cur_time.tv_usec >= deadline.tv_usec)
            break;
        /* print message */
        int sec_left = deadline.tv_sec - cur_time.tv_sec;
        sec_left += (deadline.tv_usec - cur_time.tv_usec + 999999) / 1000000; /* round up */
        nwz_lcdmsgf(false, 0, 2, "Booting OF in %d seconds  ", sec_left);
        nwz_lcdmsg(false, 0, 3, "Press BACK to run tools");
        nwz_lcdmsg(false, 0, 3, "Press PLAY to boot RB");
        /* wait for a key (1s) */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        if(nwz_key_event_is_press(&evt))
            continue;
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_PLAY)
        {
            mode = BOOT_ROCKBOX;
            break;
        }
        else if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK)
        {
            mode = BOOT_TOOLS;
            break;
        }
    }
    nwz_key_close(input_fd);
    return mode;
}

static char *boot_rb_argv[] =
{
    "lcdmsg",
    "-c",
    "-l",
    "0,3",
    "Hello from RB",
    NULL
};

int NWZ_TOOL_MAIN(all_tools)(int argc, char **argv);

int main(int argc, char **argv)
{
    /* make sure backlight is on and we are running the standard lcd mode */
    int fb_fd = nwz_fb_open(true);
    if(fb_fd >= 0)
    {
        struct nwz_fb_brightness bl;
        nwz_fb_get_brightness(fb_fd, &bl);
        bl.level = NWZ_FB_BL_MAX_LEVEL;
        nwz_fb_set_brightness(fb_fd, &bl);
        nwz_fb_set_standard_mode(fb_fd);
        nwz_fb_close(fb_fd);
    }
    nwz_lcdmsg(true, 0, 0, "dualboot");
    /* run all tools menu */
    enum boot_mode mode = get_boot_mode();
    if(mode == BOOT_TOOLS)
    {
        /* run tools and then run OF */
        NWZ_TOOL_MAIN(all_tools)(argc, argv);
    }
    else if(mode == BOOT_ROCKBOX)
    {
        /* boot rockox */
        nwz_lcdmsg(true, 0, 3, "Booting rockbox...");
        /* in the future, we will run rockbox here, for now we just print a
         * message */
        execvp("/usr/local/bin/lcdmsg", boot_rb_argv);
        /* fallback to OF in case of failure */
        nwz_lcdmsg(false, 0, 4, "failed.");
        sleep(5);
    }
    /* boot OF */
    nwz_lcdmsg(true, 0, 3, "Booting OF...");
    execvp("/usr/local/bin/SpiderApp.of", argv);
    nwz_lcdmsg(false, 0, 4, "failed.");
    sleep(5);
    /* if we reach this point, everything failed, so return an error so that
     * sysmgrd knows something is wrong */
    return 1;
}

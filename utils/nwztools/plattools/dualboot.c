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

bool boot_rockbox(void)
{
    /* get time */
    struct timeval deadline;
    if(gettimeofday(&deadline, NULL) != 0)
    {
        nwz_lcdmsg(false, 0, 2, "Cannot get time");
        sleep(2);
        return false;
    }
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 0, 2, "Cannot open input device");
        sleep(2);
        return false;
    }
    deadline.tv_sec += 5;
    /* wait for user action */
    bool boot_rb = false;
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
        nwz_lcdmsg(false, 0, 3, "Press BACK to boot RB");
        /* wait for a key (1s) */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && !nwz_key_event_is_press(&evt))
        {
            boot_rb = true;
            break;
        }
    }
    nwz_key_close(input_fd);
    return boot_rb;
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

static void wait_key(void)
{
    int input_fd = nwz_key_open();
    /* display input state in a loop */
    while(1)
    {
        /* wait for event (10ms) */
        int ret = nwz_key_wait_event(input_fd, 10000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && !nwz_key_event_is_press(&evt))
            break;
    }
    /* finish nicely */
    nwz_key_close(input_fd);
}

int NWZ_TOOL_MAIN(all_tools)(int argc, char **argv);

int main(int argc, char **argv)
{
#if 0
    nwz_lcdmsg(true, 0, 0, "dualboot");
    if(boot_rockbox())
    {
        /* boot rockox */
        nwz_lcdmsg(true, 0, 3, "Booting rockbox...");
        execvp("/usr/local/bin/lcdmsg", boot_rb_argv);
        nwz_lcdmsg(false, 0, 4, "failed.");
        sleep(5);
    }
    /* if for some reason, running rockbox failed, then try to run the OF */
    /* boot OF */
    nwz_lcdmsg(true, 0, 3, "Booting OF...");
    execvp("/usr/local/bin/SpiderApp.of", argv);
    nwz_lcdmsg(false, 0, 4, "failed.");
    sleep(5);
    /* if we reach this point, everything failed, so return an error so that
     * sysmgrd knows something is wrong */
    return 1;
#elif 0
    const char *args_mount[] = {"mount", NULL};
    int status;
    char *output = nwz_run_pipe("mount", args_mount, &status);
    nwz_lcdmsgf(true, 0, 0, "%d\n%s", status, output);
    free(output);
    wait_key();
    const char *args_ls[] = {"ls", "/var", NULL};
    output = nwz_run_pipe("ls", args_ls, &status);
    nwz_lcdmsgf(true, 0, 0, "%d\n%s", status, output);
    free(output);
    wait_key();
    const char *args_glogctl[] = {"glogctl", "flush", NULL};
    output = nwz_run_pipe("/usr/local/bin/glogctl", args_glogctl, &status);
    nwz_lcdmsgf(true, 0, 0, "%d\n%s", status, output);
    free(output);
    wait_key();
    system("cp /var/GEMINILOG* /contents/");
    sync();
    execvp("/usr/local/bin/SpiderApp.of", argv);
    return 0;
#else
    /* make sure backlight is on */
    int fb_fd = nwz_fb_open(true);
    if(fb_fd >= 0)
    {
        struct nwz_fb_brightness bl;
        nwz_fb_get_brightness(fb_fd, &bl);
        bl.level = NWZ_FB_BL_MAX_LEVEL;
        nwz_fb_set_brightness(fb_fd, &bl);
        nwz_fb_close(fb_fd);
    }
    /* run all tools menu */
    NWZ_TOOL_MAIN(all_tools)(argc, argv);
    /* run OF */
    execvp("/usr/local/bin/SpiderApp.of", argv);
    return 0;
#endif
}

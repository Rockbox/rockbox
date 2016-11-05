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

#define TOOL_LIST \
    TOOL(dest_tool) \
    TOOL(test_adc) \
    TOOL(test_bl) \
    TOOL(test_display) \
    TOOL(test_keys) \
    TOOL(test_power) \
    TOOL(test_ts) \
    TOOL(test_fb) \

typedef int (*nwz_tool_main_t)(int argc, char **argv);

struct nwz_tool_t
{
    const char *name;
    nwz_tool_main_t main;
};

/* create list of extern definition */
#define TOOL(name) extern int NWZ_TOOL_MAIN(name)(int argc, char **argv);
TOOL_LIST
#undef TOOL

/* create actual list */
#define TOOL(name) { #name, NWZ_TOOL_MAIN(name) },
static struct nwz_tool_t g_tools[] =
{
    TOOL_LIST
};
#undef TOOL

#define NR_TOOLS    (sizeof(g_tools) / sizeof(g_tools[0]))

static void hello(void)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "all_tools");
    nwz_lcdmsg(false, 0, 1, "BACK: quit");
    nwz_lcdmsg(false, 0, 2, "LEFT/RIGHT: change tool");
    nwz_lcdmsg(false, 0, 3, "PLAY: run tool");
}

/* this tool itself can be embedded in the dualboot */
#ifdef NWZ_DUALBOOT
int NWZ_TOOL_MAIN(all_tools)(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    hello();
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 5, "Cannot open input device");
        sleep(2);
        return 1;
    }
    /* main loop */
    int cur_tool = 0;
    while(true)
    {
        /* print tools */
        int line = 5;
        for(size_t i = 0; i < NR_TOOLS; i++)
        {
            nwz_lcdmsgf(false, 0, line++, "%c %s", (i == cur_tool) ? '>' : ' ',
                g_tools[i].name);
        }
        /* wait for event (1000ms) */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        /* only act on key release */
        if(nwz_key_event_is_press(&evt))
            continue;
        int keycode = nwz_key_event_get_keycode(&evt);
        if(keycode == NWZ_KEY_LEFT)
        {
            cur_tool--;
            if(cur_tool == -1)
                cur_tool += NR_TOOLS;
        }
        else if(keycode == NWZ_KEY_RIGHT)
        {
            cur_tool++;
            if(cur_tool == NR_TOOLS)
                cur_tool = 0;
        }
        else if(keycode == NWZ_KEY_PLAY)
        {
            /* close input */
            nwz_key_close(input_fd);
            g_tools[cur_tool].main(argc, argv);
            /* reopen input and clear the screen */
            input_fd = nwz_key_open();
            hello();
        }
        else if(keycode == NWZ_KEY_BACK)
            break;
    }
    nwz_key_close(input_fd);
    return 0;
}

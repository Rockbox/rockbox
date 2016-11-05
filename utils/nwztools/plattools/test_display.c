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

int NWZ_TOOL_MAIN(test_display)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_display");
    nwz_lcdmsg(false, 0, 1, "BACK: quit");
    /* wait for key */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        sleep(2);
        return 1;
    }
    while(1)
    {
        /* wait for event */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
            continue;
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        /* handle quit */
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && !nwz_key_event_is_press(&evt))
            break;
    }
    nwz_key_close(input_fd);
    return 0;
}

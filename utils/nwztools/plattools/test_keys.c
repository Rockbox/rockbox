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

int NWZ_TOOL_MAIN(test_keys)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_keys");
    nwz_lcdmsg(false, 0, 2, "BACK: hold 3 seconds to quit");
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open input device");
        sleep(2);
        return 1;
    }
    /* display input state in a loop */
    int back_pressed = 0; /* 0 = no pressed, >0 = number of seconds pressed - 1 */
#define FIRST_LINE  7
#define LAST_LINE   17
    int event_line = FIRST_LINE;
    int prev_evt_line = -1;
    while(1)
    {
        /* display HOLD status */
        nwz_lcdmsgf(false, 2, 5, "HOLD: %d", nwz_key_get_hold_status(input_fd));
        /* wait for event */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
        {
            if(back_pressed > 0)
                back_pressed++;
            if(back_pressed >= 4)
                break;
            continue;
        }
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        /* erase last '>' indicator */
        if(prev_evt_line != -1)
            nwz_lcdmsg(false, 0, prev_evt_line, "  ");
        prev_evt_line = event_line;
        char buffer[32];
        int len = sprintf(buffer, "> %s %s (HOLD=%d)",
            nwz_key_get_name(nwz_key_event_get_keycode(&evt)),
            nwz_key_event_is_press(&evt) ? "pressed" : "released",
            nwz_key_event_get_hold_status(&evt));
        /* pad with spaces to erase old stuff */
        while(len + 1 < sizeof(buffer))
            buffer[len++] = ' ';
        buffer[len] = 0;
        /* print line */
        nwz_lcdmsg(false, 0, event_line, buffer);
        /* compute next line */
        event_line++;
        if(event_line == LAST_LINE)
            event_line = FIRST_LINE;
        /* handle quit */
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK && nwz_key_event_is_press(&evt))
            back_pressed = 1;
        else
            back_pressed = 0;
    }
    /* wait until back is released, to avoid messing with all_tools (if embedded) */
    nwz_lcdmsg(true, 0, 0, "test_keys");
    nwz_lcdmsg(false, 0, 2, "BACK: release to quit");
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
    /* close input device */
    nwz_key_close(input_fd);
    /* finish nicely */
    return 0;
}

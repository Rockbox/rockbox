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

int main(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_keys");
    nwz_lcdmsg(false, 0, 2, "hold PWR OFF for 3 seconds to quit");
    /* open input device */
    int input_fd = nwz_key_open();
    if(input_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open input device");
        sleep(2);
        return 1;
    }
    /* display input state in a loop */
    int pwr_off_pressed = 0; /* 0 = no pressed, >0 = number of seconds pressed - 1 */
    while(1)
    {
        /* display HOLD status */
        nwz_lcdmsgf(false, 2, 5, "HOLD: %d", nwz_key_get_hold_status(input_fd));
        /* wait for event */
        int ret = nwz_key_wait_event(input_fd, 1000000);
        if(ret != 1)
        {
            if(pwr_off_pressed > 0)
                pwr_off_pressed++;
            if(pwr_off_pressed >= 4)
                break;
            continue;
        }
        struct input_event evt;
        if(nwz_key_read_event(input_fd, &evt) != 1)
            continue;
        nwz_lcdmsgf(false, 2, 6, "%s %s (HOLD=%d)                 ",
            nwz_key_get_name(nwz_key_event_get_keycode(&evt)),
            nwz_key_event_is_press(&evt) ? "pressed" : "released",
            nwz_key_event_get_hold_status(&evt));
        if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_OPTION && nwz_key_event_is_press(&evt))
            pwr_off_pressed = 1;
        else
            pwr_off_pressed = 0;
    }
    /* close input device */
    close(input_fd);
    /* finish nicely */
    return 0;
}
 

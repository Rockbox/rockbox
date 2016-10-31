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

int NWZ_TOOL_MAIN(test_ts)(int argc, char **argv)
{
    /* clear screen and display welcome message */
    nwz_lcdmsg(true, 0, 0, "test_ts");
    nwz_lcdmsg(false, 0, 2, "BACK: quit");
    /* open input device */
    int key_fd = nwz_key_open();
    if(key_fd < 0)
    {
        nwz_lcdmsg(false, 3, 4, "Cannot open key device");
        sleep(2);
        return 1;
    }
    int ts_fd = nwz_ts_open();
    if(ts_fd < 0)
    {
        nwz_key_close(key_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot open touch screen device");
        sleep(2);
        return 1;
    }
    /* init state and print maximum information */
    struct nwz_ts_state_t ts_state;
    if(nwz_ts_state_init(ts_fd, &ts_state) < 0)
    {
        nwz_key_close(key_fd);
        nwz_ts_close(ts_fd);
        nwz_lcdmsg(false, 3, 4, "Cannot init touch screen device");
        sleep(2);
        return 1;
    }
    /* display static information */
    nwz_lcdmsgf(false, 1, 6, "size: %d, %d       ", ts_state.max_x, ts_state.max_y);
    /* display input state in a loop */
    while(1)
    {
        /* wait for event */
        int fds[2] = {key_fd, ts_fd};
        int ret = nwz_wait_fds(fds, 2, -1);
        if(ret & 1) /* key_fd */
        {
            struct input_event evt;
            if(nwz_key_read_event(key_fd, &evt) == 1)
            {
                if(nwz_key_event_get_keycode(&evt) == NWZ_KEY_BACK &&
                        nwz_key_event_is_press(&evt))
                    break; /* quit */
            }
        }
        if(ret & 2) /* ts_fd */
        {
#define NR_TS_EVTS  16
            struct input_event evts[NR_TS_EVTS];
            int nr = nwz_ts_read_events(ts_fd, evts, NR_TS_EVTS);
            for(int i = 0; i < nr; i++)
                if(nwz_ts_state_update(&ts_state, &evts[i]) == 1)
                {
                    nwz_lcdmsgf(false, 1, 7, "touch: %s  ", ts_state.touch ? "yes" : "no");
                    nwz_lcdmsgf(false, 1, 8, "pos: %d, %d       ", ts_state.x, ts_state.y);
                    nwz_lcdmsgf(false, 1, 9, "pressure: %d     ", ts_state.pressure);
                    nwz_lcdmsgf(false, 1, 10, "width: %d     ", ts_state.tool_width);
                    nwz_lcdmsgf(false, 1, 11, "flick: %s ", ts_state.flick ? "yes" : "no");
                    nwz_lcdmsgf(false, 1, 12, "flick vec: %d, %d    ", ts_state.flick_x, ts_state.flick_y);
                    /* process touch */
                    nwz_ts_state_post_syn(&ts_state);
                }
#undef NR_TS_EVTS
        }
    }
    /* close input device */
    nwz_key_close(key_fd);
    nwz_ts_close(ts_fd);
    /* finish nicely */
    return 0;
}

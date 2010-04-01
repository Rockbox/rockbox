/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2009 Björn Stenberg
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

#include "plugin.h"

PLUGIN_HEADER

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
    bool boost = false;
    int count = 0;
    int last_count = 0;
    int last_tick = *rb->current_tick;
    int per_sec = 0;

    rb->lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        int j,x;
        for (j=1; j<100000; j++)
            x = j*11;
        rb->screens[0]->clear_display();
        rb->screens[0]->putsf(0, 0, "%s: %d",boost?"boost":"normal",count);
        if (TIME_AFTER(*rb->current_tick, last_tick+HZ))
        {
            last_tick = *rb->current_tick;
            per_sec = count-last_count;
            last_count = count;
        }
        rb->screens[0]->putsf(0, 1, "loops/s: %d", per_sec);
        rb->screens[0]->update();
        count++;

        int button = rb->button_get(false);
        switch (button)
        {
            case BUTTON_UP:
                if (!boost)
                {
                    rb->cpu_boost(true);
                    boost = true;
                }
                break;

            case BUTTON_DOWN:
                if (boost)
                {
                    rb->cpu_boost(false);
                    boost = false;
                }
                break;

            case BUTTON_LEFT:
                done = true;
                break;
        }
    }

    return PLUGIN_OK;
}

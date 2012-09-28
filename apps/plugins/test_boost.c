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



enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
    bool boost = false;
    int count = 0;
    int last_count = 0;
    int last_tick = current_tick;
    int per_sec = 0;

    lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        int j,x;
        for (j=1; j<100000; j++)
            x = j*11;
        screens[0]->clear_display();
        screens[0]->putsf(0, 0, "%s: %d",boost?"boost":"normal",count);
        if (TIME_AFTER(current_tick, last_tick+HZ))
        {
            last_tick = current_tick;
            per_sec = count-last_count;
            last_count = count;
        }
        screens[0]->putsf(0, 1, "loops/s: %d", per_sec);
        screens[0]->update();
        count++;

        switch (get_action(CONTEXT_STD, TIMEOUT_NOBLOCK))
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            case ACTION_STD_PREV:
                if (!boost)
                {
                    cpu_boost(true);
                    boost = true;
                }
                break;

            case ACTION_STD_NEXT:
                if (boost)
                {
                    cpu_boost(false);
                    boost = false;
                }
                break;
#endif
            case ACTION_STD_CANCEL:
                done = true;
                break;
        }
    }

    return PLUGIN_OK;
}

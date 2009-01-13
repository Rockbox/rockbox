/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id: test_scanrate.c 19673 2009-01-04 23:33:15Z saratoga $
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

enum plugin_status plugin_start(const struct plugin_api* api,
                                const void* parameter)
{
    (void)parameter;
    const struct plugin_api* rb = api;
    bool done = false;
    bool boost = false;
    int count = 0;

    rb->lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        char buf[32];
        int j,x;
        for (j=1; j<100000; j++)
            x = j*11;
        rb->lcd_clear_display();
        rb->snprintf(buf,sizeof buf, "%s %d",boost?"boost":"normal",count);
        rb->lcd_putsxy(0, 0, buf);
        rb->lcd_update();
        count++;

        int button = rb->button_get(false);
        switch (button)
        {
            case BUTTON_UP:
                boost = true;
                rb->cpu_boost(boost);
                break;

            case BUTTON_DOWN:
                boost = false;
                rb->cpu_boost(boost);
                break;

            case BUTTON_LEFT:
                done = true;
                break;
        }
    }

    return PLUGIN_OK;
}

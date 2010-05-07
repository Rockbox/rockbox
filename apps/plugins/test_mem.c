/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2010 Thomas Martitz
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

#define BUF_SIZE ((PLUGIN_BUFFER_SIZE-(10<<10)) / (sizeof(int)))
#define LOOP_REPEAT 8
static volatile int buf[BUF_SIZE];

/* (Byte per loop * loops * ticks per s / ticks)>>10 = KB per s */
#define KB_PER_SEC(delta) (((BUF_SIZE*sizeof(buf[0])*LOOP_REPEAT*HZ)/delta) >> 10)

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
    bool boost = false;
    int count = 0;
    int last_tick = 0;

    rb->lcd_setfont(FONT_SYSFIXED);

    while (!done)
    {
        unsigned i, j;
        int line = 0;
        int x;
        int delta;
        last_tick = *rb->current_tick;

        for(i = 0; i < LOOP_REPEAT; i++)
        {
            for (j = 0; j < BUF_SIZE; j+=4)
            {
                buf[j  ] = j;
                buf[j+1] = j+1;
                buf[j+2] = j+2;
                buf[j+3] = j+3;
            }
        }
        delta = *rb->current_tick - last_tick;
        rb->screens[0]->clear_display();
        rb->screens[0]->putsf(0, line++, "%s", boost?"boosted":"unboosted");
        rb->screens[0]->putsf(0, line++, "bufsize: %u", BUF_SIZE*sizeof(buf[0]));
        rb->screens[0]->putsf(0, line++, "loop#: %d", ++count);
        rb->screens[0]->putsf(0, line++, "write ticks: %2d (%5d KB/s)", delta,
                              KB_PER_SEC(delta));
        last_tick = *rb->current_tick;
        for(i = 0; i < LOOP_REPEAT; i++)
        {
            for(j = 0; j < BUF_SIZE; j+=4)
            {
                x = buf[j  ];
                x = buf[j+2];
                x = buf[j+3];
                x = buf[j+4];
            }
        }
        delta = *rb->current_tick - last_tick;
        rb->screens[0]->putsf(0, line++, "read ticks : %2d (%5d KB/s)", delta,
                              KB_PER_SEC(delta));
        rb->screens[0]->update();

        switch (rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK))
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            case ACTION_STD_PREV:
                if (!boost)
                {
                    rb->cpu_boost(true);
                    boost = true;
                }
                break;

            case ACTION_STD_NEXT:
                if (boost)
                {
                    rb->cpu_boost(false);
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

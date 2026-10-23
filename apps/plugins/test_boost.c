/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
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
#include "lib/helper.h"

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define CPU_FREQ_KHZ (*rb->cpu_frequency / 1000)
#else
#ifdef CPU_FREQ
#define CPU_FREQ_KHZ (CPU_FREQ / 1000)
#else
#define CPU_FREQ_KHZ (0)
#endif
#endif

static void update_screen(bool boost, unsigned long count,
                          unsigned long secs, int per_sec, int khz, int x)
{
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->screens[0]->clear_display();
    rb->screens[0]->putsf(0, 0,
        "%s: avg %d.%02dMHz",boost?"boost":"normal",khz/1000, (khz%1000)/10);
    rb->screens[0]->putsf(0, 1,
        "total: %lu / %lus",count, secs);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->screens[0]->putsf(0, 2,
        "loops/s: %d avg: %ld%c%d", per_sec, count / secs, '\0', x); /* dont discard x */
#else
    rb->screens[0]->putsf(0, 2,
        "loops/s: %d%c%ld%d", per_sec, '\0', x); /* dont discard x */
#endif
    rb->screens[0]->update();
    rb->yield();
}

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    bool done = false;
    bool boost = false;

    unsigned long count = 0;
    unsigned long seconds_total = 0;
    unsigned long last_count = 0;
    unsigned long avgkhz = CPU_FREQ_KHZ;
    long last_tick = *rb->current_tick;
    long ticks_rem = 0; /* total ticks = ticks_rem * HZ + seconds_total */
    int per_sec = 0;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    backlight_ignore_timeout();

    update_screen(boost, count, seconds_total, per_sec, avgkhz, 0);

    while (!done)
    {
        int j, x, khz;
        for (j=1; j<100000; j++)
        {
            x = j*11;
            khz = avgkhz / ((count-last_count) + 1);
            if (TIME_AFTER(*rb->current_tick, last_tick+HZ))
            {
                long actual_ticks = last_tick;
                last_tick = *rb->current_tick;
                actual_ticks = last_tick - actual_ticks;
                ticks_rem += actual_ticks;
                unsigned long secs = ticks_rem / HZ;
                ticks_rem -= (secs * HZ);
                seconds_total += secs;
                per_sec = ((count-last_count) * HZ) / actual_ticks;
                update_screen(boost, count, seconds_total, per_sec, khz, x);
                last_count = count;
                avgkhz = (khz + CPU_FREQ_KHZ + 1) / 2;
            }
        }
        avgkhz += CPU_FREQ_KHZ;
        count++;

        switch (rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK))
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            case ACTION_STD_PREV:
                if (!boost)
                {
                    rb->cpu_boost(true);
                    boost = true;
                    update_screen(boost, count, seconds_total,
                                  per_sec, avgkhz / (per_sec + 1), 0);
                }
                break;

            case ACTION_STD_NEXT:
                if (boost)
                {
                    rb->cpu_boost(false);
                    boost = false;
                    update_screen(boost, count, seconds_total,
                                  per_sec, avgkhz / (per_sec + 1), 0);
                }
                break;
#endif
            case ACTION_STD_CANCEL:
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                if (boost)
                {
                    rb->cpu_boost(false);
                }
#endif
                done = true;
                break;
        }
    }

    backlight_use_settings();
    return PLUGIN_OK;
}

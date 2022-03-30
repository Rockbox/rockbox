/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Uwe Freese
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
#include "config.h"

#include <stdbool.h>

#include "lcd.h"
#include "action.h"
#include "kernel.h"
#include <string.h>
#include "settings.h"
#include "power.h"
#include "icons.h"
#include "rtc.h"
#include "misc.h"
#include "screens.h"
#include "talk.h"
#include "lang.h"
#include "alarm_menu.h"
#include "splash.h"
#include "viewport.h"

int alarm_screen(void)
{
    bool usb, update;
    struct tm *now = get_time();
    struct tm atm;
    memcpy(&atm, now, sizeof(struct tm));
    rtc_get_alarm(&atm.tm_hour, &atm.tm_min);

    /* After a battery change the RTC values are out of range */
    if (!valid_time(&atm))
        memcpy(&atm, now, sizeof(struct tm));
    atm.tm_sec = 0;

    usb = set_time_screen(str(LANG_ALARM_MOD_TIME), &atm, false);
    update = valid_time(&atm); /* set_time returns invalid time if canceled */

    if (!usb && update)
    {

            now = get_time();
            int nmins = now->tm_min + (now->tm_hour * 60);
            int amins = atm.tm_min + (atm.tm_hour * 60);
            int mins_togo = (amins - nmins + 1440) % 1440;
            /* prevent that an alarm occurs in the shutdown procedure */
            /* accept alarms only if they are in 2 minutes or more */
            if (mins_togo > 1) {
                rtc_init();
                rtc_set_alarm(atm.tm_hour,atm.tm_min);
                rtc_enable_alarm(true);
                if (global_settings.talk_menu)
                {
                    talk_id(LANG_ALARM_MOD_TIME_TO_GO, true);
                    talk_value(mins_togo / 60, UNIT_HOUR, true);
                    talk_value(mins_togo % 60, UNIT_MIN, true);
                    talk_force_enqueue_next();
                }
                splashf(HZ*2, str(LANG_ALARM_MOD_TIME_TO_GO),
                               mins_togo / 60, mins_togo % 60);
            } else {
                splash(HZ, ID2P(LANG_ALARM_MOD_ERROR));
                update = false;
            }
    }

    if (usb || !update)
    {
        if (!usb)
            splash(HZ*2, ID2P(LANG_ALARM_MOD_DISABLE));
        rtc_enable_alarm(false);
        return 1;
    }
    return 0;
}

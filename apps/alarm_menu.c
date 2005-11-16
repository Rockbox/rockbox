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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#ifdef HAVE_ALARM_MOD

#include <stdbool.h>

#include "options.h"

#include "lcd.h"
#include "font.h"
#include "button.h"
#include "kernel.h"
#include "sprintf.h"
#include <string.h>
#include "settings.h"
#include "power.h"
#include "status.h"
#include "icons.h"
#include "rtc.h"
#include "misc.h"
#include "screens.h"

#include "lang.h"
#include "power.h"
#include "alarm_menu.h"
#include "backlight.h"

#include "splash.h"
#define MARGIN_Y (global_settings.statusbar ? STATUSBAR_HEIGHT : 0)

bool alarm_screen(void)
{
    int h, m;
    bool done=false;
    char buf[32];
    struct tm *tm;
    int togo;
    int button;
    bool update = true;

    rtc_get_alarm(&h, &m);

    /* After a battery change the RTC values are out of range */
    if (m > 60 || h > 24) {
        m = 0;
        h = 12;
    } else {
        m = m / 5 * 5; /* 5 min accuracy should be enough */
    }

    while(!done) {
        if(update)
        {
            lcd_clear_display();
            status_draw(true);
            lcd_setfont(FONT_SYSFIXED);
            lcd_setmargins(0, MARGIN_Y);
            lcd_puts(0, 3, str(LANG_ALARM_MOD_KEYS));
            update = false;
        }
        
        snprintf(buf, 32, str(LANG_ALARM_MOD_TIME), h, m);
        lcd_puts(0, 1, buf);
        lcd_update();

        button = button_get_w_tmo(HZ);
        
        switch(button) {
        case BUTTON_PLAY:
            /* prevent that an alarm occurs in the shutdown procedure */
            /* accept alarms only if they are in 2 minutes or more */
            tm = get_time();
            togo = (m + h * 60 - tm->tm_min - tm->tm_hour * 60 + 1440) % 1440;
            if (togo > 1) {
                rtc_init();
                rtc_set_alarm(h,m);
                rtc_enable_alarm(true);
                gui_syncsplash(HZ*2, true, str(LANG_ALARM_MOD_TIME_TO_GO),
                       togo / 60, togo % 60);
		done = true;
            } else {
                gui_syncsplash(HZ, true, str(LANG_ALARM_MOD_ERROR));
                update = true;
            }
            break;

         /* inc(m) */
        case BUTTON_RIGHT:
        case BUTTON_RIGHT | BUTTON_REPEAT:
            m += 5;
            if (m == 60) {
                h += 1;
                m = 0;
            }
            if (h == 24)
                h = 0;
            break;

         /* dec(m) */
         case BUTTON_LEFT:
         case BUTTON_LEFT | BUTTON_REPEAT:
             m -= 5;
             if (m == -5) {
                 h -= 1;
                 m = 55;
             }
             if (h == -1)
                 h = 23;
             break;
            
#if CONFIG_KEYPAD == RECORDER_PAD
         /* inc(h) */            
         case BUTTON_UP:
         case BUTTON_UP | BUTTON_REPEAT:
             h = (h+1) % 24;
             break;

         /* dec(h) */
         case BUTTON_DOWN:
         case BUTTON_DOWN | BUTTON_REPEAT:
             h = (h+23) % 24;
             break;
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
        case BUTTON_OFF:
#else
        case BUTTON_STOP:
        case BUTTON_MENU:
#endif
            rtc_enable_alarm(false);
            gui_syncsplash(HZ*2, true, str(LANG_ALARM_MOD_DISABLE));
            done = true;
            break;

        case BUTTON_NONE:
            status_draw(false);
            break;

        default:
            if(default_event_handler(button) == SYS_USB_CONNECTED)
            {
                rtc_enable_alarm(false);
                return true;
            }
            break;
        }
    }

    return false;
}

#endif /* HAVE_ALARM_MOD */

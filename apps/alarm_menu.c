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
#include "rtc.h"
#include <stdbool.h>

#include "lang.h"
#include "power.h"
#include "alarm_menu.h"
#include "backlight.h"

#ifdef HAVE_ALARM_MOD

bool alarm_screen(void)
{
    /* get alarm time from RTC */

    int h, m, hour, minute;

    rtc_get_alarm(&h, &m);

    if (m > 60 || h > 24) {	/* after battery-change RTC-values are out of range */
        m = 0;
        h = 12;
    } else {
        m = m / 5 * 5; /* 5 min accuracy should be enough */
    }

    bool done=false;
    char buf[32]; 

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    lcd_setmargins(0, 0);
    lcd_puts(0,1, str(LANG_ALARM_MOD_KEYS));
    
    while(!done) {
        snprintf(buf, 32, str(LANG_ALARM_MOD_TIME), h, m);
        lcd_puts(0,0, buf);
        lcd_update();
        
        switch(button_get(true)) {
        case BUTTON_PLAY:
            /* prevent that an alarm occurs in the shutdown procedure */
            /* accept alarms only if they are in 2 minutes or more */
            hour = rtc_read(0x03);
            hour = ((hour & 0x30) >> 4) * 10 + (hour & 0x0f);
            minute = rtc_read(0x02);
            minute = ((minute & 0x70) >> 4) * 10 + (minute & 0x0f);
            int togo = (m + h * 60 - minute - hour * 60 + 1440) % 1440;
            if (togo > 1) {
                lcd_clear_display();
                snprintf(buf, 32, str(LANG_ALARM_MOD_TIME_TO_GO), togo / 60, togo % 60);
                lcd_puts(0,0, buf);
                lcd_update();
                rtc_init();
                rtc_set_alarm(h,m);
                rtc_enable_alarm(true);
                lcd_puts(0,1,str(LANG_ALARM_MOD_SHUTDOWN));
                lcd_update();
                sleep(HZ);
		done = true;
            } else {
                lcd_clear_display();
                lcd_puts(0,0,str(LANG_ALARM_MOD_ERROR));
                lcd_update();
                sleep(HZ);
                lcd_clear_display();
                lcd_puts(0,1,str(LANG_ALARM_MOD_KEYS));
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
            lcd_clear_display();
	    lcd_puts(0,0,str(LANG_ALARM_MOD_DISABLE));
	    lcd_update();
	    sleep(HZ);
            rtc_enable_alarm(false);
            done = true;
            break;
        }
    }

    return false;
}

#endif /* HAVE_ALARM_MOD */

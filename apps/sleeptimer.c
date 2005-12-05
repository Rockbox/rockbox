/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Linus Nielsen Feltzing
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
#include "powermgmt.h"
#include "statusbar.h"
#include "debug.h"
#include "talk.h"
#include "icons.h"

#include "lang.h"

#define SMALL_STEP_SIZE 15*60    /* Seconds */
#define LARGE_STEP_SIZE 30*60    /* Seconds */
#define THRESHOLD       60       /* Minutes */
#define MAX_TIME        5*60*60  /* Hours */

bool sleeptimer_screen(void)
{
    unsigned long seconds;
    int hours, minutes;
    int button;
    bool done = false;
    char buf[32];
    int oldtime, newtime;
    int amount = 0;
    int org_timer=get_sleep_timer();
    bool changed=false;
    bool sayit = true;

#ifdef HAVE_LCD_BITMAP
    if (global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif
    
    lcd_clear_display();
    lcd_puts_scroll(0, 0, str(LANG_SLEEP_TIMER));

    while(!done)
    {
        button = button_get_w_tmo(HZ);
        switch(button)
        {
#ifdef SETTINGS_OK2
            case SETTINGS_OK2:
#endif
            case SETTINGS_OK:
                done = true;
                break;

            case SETTINGS_CANCEL:
                if (changed) {
                    lcd_stop_scroll();
                    lcd_puts(0, 0, str(LANG_MENU_SETTING_CANCEL));
                    lcd_update();
                    set_sleep_timer(org_timer);
                    sleep(HZ/2);
                }
                done = true;
                break;

            case SETTINGS_INC:
                oldtime = (get_sleep_timer()+59) / 60;
                if(oldtime < THRESHOLD)
                    amount = SMALL_STEP_SIZE;
                else
                    amount = LARGE_STEP_SIZE;
            
                newtime = oldtime * 60 + amount;
                if(newtime > MAX_TIME)
                    newtime = MAX_TIME;

                changed = sayit = true;
                set_sleep_timer(newtime);
                break;
                
            case SETTINGS_DEC:
                oldtime = (get_sleep_timer()+59) / 60;
                if(oldtime <= THRESHOLD)
                    amount = SMALL_STEP_SIZE;
                else
                    amount = LARGE_STEP_SIZE;
            
                newtime = oldtime*60 - amount;
                if(newtime < 0)
                    newtime = 0;
            
                changed = sayit = true;
                set_sleep_timer(newtime);
                break;
        }

        seconds = get_sleep_timer();

        if(seconds)
        {
            seconds += 59; /* Round up for a "friendlier" display */
            hours = seconds / 3600;
            minutes = (seconds - (hours * 3600)) / 60;
            snprintf(buf, 32, "%d:%02d",
                     hours, minutes);
            lcd_puts(0, 1, (unsigned char *)buf);

            if (sayit && global_settings.talk_menu)
            {
                bool enqueue = false; /* first one should not ne queued */

                if (hours)
                {   
                    talk_value(hours, UNIT_HOUR, enqueue);
                    enqueue = true; /* queue any following */
                }
                if (minutes)
                {
                    talk_value(minutes, UNIT_MIN, enqueue);
                }

                sayit = false;
            }
        }
        else
        {
            lcd_puts(0, 1, str(LANG_OFF));
            if (sayit && global_settings.talk_menu)
            {
                talk_id(LANG_OFF, false);
                sayit = false;
            }
        }

        gui_syncstatusbar_draw(&statusbars, true);

        lcd_update();
    }
    lcd_stop_scroll();
    return false;
}

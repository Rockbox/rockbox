/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Jerome Kuptz
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "system.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "button.h"
#include "kernel.h"
#include "tree.h"
#include "debug.h"
#include "sprintf.h"
#include "settings.h"
#include "wps.h"
#include "wps-display.h"
#include "mpeg.h"
#include "mp3_playback.h"
#include "usb.h"
#include "status.h"
#include "main_menu.h"
#include "ata.h"
#include "screens.h"
#include "playlist.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "peakmeter.h"
#include "action.h"
#endif
#include "lang.h"
#include "bookmark.h"
#include "misc.h"

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

bool keys_locked = false;
static bool ff_rewind = false;
static bool paused = false;
static struct mp3entry* id3 = NULL;
static struct mp3entry* nid3 = NULL;
static char current_track_path[MAX_PATH+1];

/* button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define WPS_NEXT    (BUTTON_RIGHT | BUTTON_REL)
#define WPS_PREV    (BUTTON_LEFT | BUTTON_REL)
#define WPS_FFWD    (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW     (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL   BUTTON_UP
#define WPS_DECVOL   BUTTON_DOWN
#define WPS_PAUSE    BUTTON_PLAY
#define WPS_MENU    (BUTTON_F1 | BUTTON_REL)
#define WPS_MENU_PRE BUTTON_F1
#define WPS_BROWSE  (BUTTON_ON | BUTTON_REL)
#define WPS_EXIT     BUTTON_OFF
#define WPS_KEYLOCK (BUTTON_F1 | BUTTON_DOWN)
#define WPS_ID3     (BUTTON_F1 | BUTTON_ON)

#define WPS_RC_NEXT   BUTTON_RC_RIGHT
#define WPS_RC_PREV   BUTTON_RC_LEFT
#define WPS_RC_PAUSE  BUTTON_RC_PLAY
#define WPS_RC_INCVOL BUTTON_RC_VOL_UP
#define WPS_RC_DECVOL BUTTON_RC_VOL_DOWN
#define WPS_RC_EXIT   BUTTON_RC_STOP

#elif CONFIG_KEYPAD == PLAYER_PAD
#define WPS_NEXT     BUTTON_RIGHT
#define WPS_PREV     BUTTON_LEFT
#define WPS_FFWD    (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW     (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL  (BUTTON_MENU | BUTTON_RIGHT)
#define WPS_DECVOL  (BUTTON_MENU | BUTTON_LEFT)
#define WPS_PAUSE    BUTTON_PLAY
#define WPS_MENU    (BUTTON_MENU | BUTTON_REL)
#define WPS_MENU_PRE BUTTON_MENU
#define WPS_BROWSE  (BUTTON_ON | BUTTON_REL)
#define WPS_EXIT     BUTTON_STOP
#define WPS_KEYLOCK (BUTTON_MENU | BUTTON_STOP)
#define WPS_ID3     (BUTTON_MENU | BUTTON_ON)

#define WPS_RC_NEXT   BUTTON_RC_RIGHT
#define WPS_RC_PREV   BUTTON_RC_LEFT
#define WPS_RC_PAUSE  BUTTON_RC_PLAY
#define WPS_RC_INCVOL BUTTON_RC_VOL_UP
#define WPS_RC_DECVOL BUTTON_RC_VOL_DOWN
#define WPS_RC_EXIT   BUTTON_RC_STOP

#elif CONFIG_KEYPAD == ONDIO_PAD
#define WPS_NEXT    (BUTTON_RIGHT | BUTTON_REL)
#define WPS_PREV    (BUTTON_LEFT | BUTTON_REL)
#define WPS_FFWD    (BUTTON_RIGHT | BUTTON_REPEAT)
#define WPS_REW     (BUTTON_LEFT | BUTTON_REPEAT)
#define WPS_INCVOL   BUTTON_UP
#define WPS_DECVOL   BUTTON_DOWN
#define WPS_PAUSE    BUTTON_OFF
#define WPS_MENU    (BUTTON_MENU | BUTTON_REPEAT)
#define WPS_BROWSE  (BUTTON_MENU | BUTTON_REL)
#define WPS_KEYLOCK (BUTTON_MENU | BUTTON_DOWN)

#endif

/* set volume
   return true if screen restore is needed
   return false otherwise
*/
static bool setvol(void)
{
    if (global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
        global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
    status_draw(false);
    wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);
    settings_save();
#ifdef HAVE_LCD_CHARCELLS
    splash(0, false, "Vol: %d %%   ",
           mpeg_val2phys(SOUND_VOLUME, global_settings.volume));
    return true;
#endif
    return false;
}

static void display_keylock_text(bool locked)
{
    char* s;
    lcd_stop_scroll();
#ifdef HAVE_LCD_CHARCELLS
    if(locked)
        s = str(LANG_KEYLOCK_ON_PLAYER);
    else
        s = str(LANG_KEYLOCK_OFF_PLAYER);
#else
    if(locked)
        s = str(LANG_KEYLOCK_ON_RECORDER);
    else
        s = str(LANG_KEYLOCK_OFF_RECORDER);
#endif
    splash(HZ, true, s);
}

static bool ffwd_rew(int button)
{
    static const int ff_rew_steps[] = {
      1000, 2000, 3000, 4000,
      5000, 6000, 8000, 10000,
      15000, 20000, 25000, 30000,
      45000, 60000
    };

    unsigned int step = 0;     /* current ff/rewind step */ 
    unsigned int max_step = 0; /* maximum ff/rewind step */ 
    int ff_rewind_count = 0;   /* current ff/rewind count (in ticks) */
    int direction = -1;         /* forward=1 or backward=-1 */
    long accel_tick = 0;       /* next time at which to bump the step size */
    bool exit = false;
    bool usb = false;

    while (!exit) {
        switch ( button ) {
            case WPS_FFWD:
                 direction = 1;
            case WPS_REW:
                if (ff_rewind)
                {
                    if (direction == 1)
                    {
                        /* fast forwarding, calc max step relative to end */
                        max_step =
                            (id3->length - (id3->elapsed + ff_rewind_count)) *
                            FF_REWIND_MAX_PERCENT / 100;
                    }
                    else
                    {
                        /* rewinding, calc max step relative to start */
                        max_step = (id3->elapsed + ff_rewind_count) *
                            FF_REWIND_MAX_PERCENT / 100;
                    }

                    max_step = MAX(max_step, MIN_FF_REWIND_STEP);

                    if (step > max_step)
                        step = max_step;

                    ff_rewind_count += step * direction;

                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= accel_tick)
                    { 
                        step *= 2;
                        accel_tick = current_tick +
                            global_settings.ff_rewind_accel*HZ; 
                    } 
                }
                else
                {
                    if ( (mpeg_status() & MPEG_STATUS_PLAY) &&
                          id3 && id3->length )
                    {
                        if (!paused)
                            mpeg_pause();
#if CONFIG_KEYPAD == PLAYER_PAD
                        lcd_stop_scroll();
#endif
                        if (direction > 0) 
                            status_set_ffmode(STATUS_FASTFORWARD);
                        else
                            status_set_ffmode(STATUS_FASTBACKWARD);

                        ff_rewind = true;

                        step = ff_rew_steps[global_settings.ff_rewind_min_step];

                        accel_tick = current_tick +
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if (direction > 0) {
                    if ((id3->elapsed + ff_rewind_count) > id3->length)
                        ff_rewind_count = id3->length - id3->elapsed;
                }
                else {
                    if ((int)(id3->elapsed + ff_rewind_count) < 0)
                        ff_rewind_count = -id3->elapsed;
                }

                if(wps_time_countup == false)
                    wps_refresh(id3, nid3, -ff_rewind_count,
                                WPS_REFRESH_PLAYER_PROGRESS |
                                WPS_REFRESH_DYNAMIC);
                else
                    wps_refresh(id3, nid3, ff_rewind_count,
                                WPS_REFRESH_PLAYER_PROGRESS |
                                WPS_REFRESH_DYNAMIC);

                break;

            case WPS_PREV:
            case WPS_NEXT: 
                mpeg_ff_rewind(id3->elapsed+ff_rewind_count);
                ff_rewind_count = 0;
                ff_rewind = false;
                status_set_ffmode(0);
                if (!paused)
                    mpeg_resume();
#ifdef HAVE_LCD_CHARCELLS
                wps_display(id3, nid3);
#endif
                exit = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    status_set_ffmode(0);
                    usb = true;
                    exit = true;
                }
                break;
        }
        if (!exit)
            button = button_get(true);
    }

    /* let mpeg thread update id3->elapsed before calling wps_refresh */
    yield(); 
    wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);
    return usb;
}

static bool update(void)
{
    bool track_changed = mpeg_has_changed_track();
    bool retcode = false;

    nid3 = mpeg_next_track();
    if (track_changed)
    {
        lcd_stop_scroll();
        id3 = mpeg_current_track();
        if (wps_display(id3, nid3))
            retcode = true;
        else
            wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);

        if (id3)
            memcpy(current_track_path, id3->path, sizeof(current_track_path));
    }

    if (id3)
        wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);

    status_draw(false);

    /* save resume data */
    if ( id3 &&
         global_settings.resume &&
         global_settings.resume_offset != id3->offset ) {
        DEBUGF("R%X,%X (%X)\n", global_settings.resume_offset,
               id3->offset,id3);
 
        if (!playlist_get_resume_info(&global_settings.resume_index))
        {
            global_settings.resume_offset = id3->offset;
            settings_save();
        }
    }
    else if ( !id3 && track_changed ) {
        global_settings.resume_index = -1;
        global_settings.resume_offset = -1;
        settings_save();
    }

    return retcode;
}

static void fade(bool fade_in)
{
    if (fade_in) {
        /* fade in */
        int current_volume = 20;
        
        /* zero out the sound */
        mpeg_sound_set(SOUND_VOLUME, current_volume);

        sleep(HZ/10); /* let mpeg thread run */
        mpeg_resume();
        
        while (current_volume < global_settings.volume) {    
            current_volume += 2;
            sleep(1);
            mpeg_sound_set(SOUND_VOLUME, current_volume);
        }
        mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
    }
    else {
        /* fade out */
        int current_volume = global_settings.volume;

        while (current_volume > 20) {    
            current_volume -= 2;
            sleep(1);
            mpeg_sound_set(SOUND_VOLUME, current_volume);
        }
        mpeg_pause();
        sleep(HZ/5); /* let mpeg thread run */

        /* reset volume to what it was before the fade */
        mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
    }
}


static void waitfor_nokey(void)
{
    /* wait until all keys are released */
    while (button_get(false) != BUTTON_NONE)
        yield();
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    int button = 0, lastbutton = 0;
    bool ignore_keyup = true;
    bool restore = false;
    long restoretimer = 0; /* timer to delay screen redraw temporarily */
    bool exit = false;
    bool update_track = false;

    id3 = nid3 = NULL;
    current_track_path[0] = '\0';

#ifdef HAVE_LCD_CHARCELLS
    status_set_audio(true);
    status_set_param(false);
#else
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    ff_rewind = false;

    if(mpeg_status() & MPEG_STATUS_PLAY)
    {
        id3 = mpeg_current_track();
        nid3 = mpeg_next_track();
        if (id3) {
            if (wps_display(id3, nid3))
                return 0;
            wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);

            memcpy(current_track_path, id3->path, sizeof(current_track_path));
        }

        restore = true;
    }

    while ( 1 )
    {
        bool mpeg_paused = (mpeg_status() & MPEG_STATUS_PAUSE)?true:false;
        
        /* did someone else (i.e power thread) change mpeg pause mode? */
        if (paused != mpeg_paused) {
            paused = mpeg_paused;

            /* if another thread paused mpeg, we are probably in car mode,
               about to shut down. lets save the settings. */
            if (paused && global_settings.resume) {
                settings_save();
#ifndef HAVE_RTC
                ata_flush();
#endif
            }
        }

#ifdef HAVE_LCD_BITMAP
        /* when the peak meter is enabled we want to have a
            few extra updates to make it look smooth. On the
            other hand we don't want to waste energy if it 
            isn't displayed */
        if (peak_meter_enabled) {
            int i;

            /* In high performance mode we read out the mas as
               often as we can. There is no sleep for cpu */
            if (global_settings.peak_meter_performance) {
                long next_refresh = current_tick;
                long next_big_refresh = current_tick + HZ / 5;
                button = BUTTON_NONE;
                while (!TIME_AFTER(current_tick, next_big_refresh)) {
                    button = button_get(false);
                    if (button != BUTTON_NONE) {
                        break;
                    }
                    peak_meter_peek();
                    sleep(1);

                    if (TIME_AFTER(current_tick, next_refresh)) {
                        wps_refresh(id3, nid3, 0, WPS_REFRESH_PEAK_METER);
                        next_refresh = current_tick + HZ / peak_meter_fps;
                    }
                }
            } 
            
            /* In energy saver mode the cpu may sleep a 
               little bit while waiting for buttons */
            else {
                for (i = 0; i < 4; i++) {
                    button = button_get_w_tmo(HZ / peak_meter_fps);
                    if (button != 0) {
                        break;
                    }
                    wps_refresh(id3, nid3, 0, WPS_REFRESH_PEAK_METER);
                }
            }
        } 
        
        /* The peak meter is disabled 
           -> no additional screen updates needed */
        else {
            button = button_get_w_tmo(HZ/5);
        }
#else
        button = button_get_w_tmo(HZ/5);
#endif

        /* discard first event if it's a button release */
        if (button && ignore_keyup)
        {
            ignore_keyup = false;
            /* Negative events are system events */
            if (button >= 0 && button & BUTTON_REL )
                continue;
        }

#ifdef WPS_KEYLOCK
        /* ignore non-remote buttons when keys are locked */
        if (keys_locked &&
            ! ((button < 0) ||
               (button == BUTTON_NONE) ||
               ((button & WPS_KEYLOCK) == WPS_KEYLOCK) ||
               (button & BUTTON_REMOTE)
                ))
        {
            if (!(button & BUTTON_REL))
                display_keylock_text(true);
            restore = true;
            button = BUTTON_NONE;
        }
#endif

        /* Exit if mpeg has stopped playing. This can happen if using the
           sleep timer with the charger plugged or if starting a recording
           from F1 */
        if (!mpeg_status())
            exit = true;

        switch(button)
        {
            case WPS_BROWSE:
#ifdef HAVE_LCD_CHARCELLS
                status_set_record(false);
                status_set_audio(false);
#endif
                lcd_stop_scroll();

                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    current_track_path[0] != '\0')
                    set_current_file(current_track_path);
                
                return 0;
                break;

                /* play/pause */
            case WPS_PAUSE:
#ifdef WPS_RC_PAUSE
            case WPS_RC_PAUSE:
#endif
                if ( paused )
                {
                    paused = false;
                    if ( global_settings.fade_on_stop )
                        fade(1);
                    else
                        mpeg_resume();
                }
                else
                {
                    paused = true;
                    if ( global_settings.fade_on_stop )
                        fade(0);
                    else
                        mpeg_pause();
                    if (global_settings.resume) {
                        settings_save();
#ifndef HAVE_RTC
                        ata_flush();
#endif
                    }
                }
                break;

                /* volume up */
            case WPS_INCVOL:
            case WPS_INCVOL | BUTTON_REPEAT:
#ifdef WPS_RC_INCVOL
            case WPS_RC_INCVOL:
#endif
                global_settings.volume++;
                if (setvol()) {
                    restore = true;
                    restoretimer = current_tick + HZ;
                }
                break;

                /* volume down */
            case WPS_DECVOL:
            case WPS_DECVOL | BUTTON_REPEAT:
#ifdef WPS_RC_DECVOL
            case WPS_RC_DECVOL:
#endif
                global_settings.volume--;
                if (setvol()) {
                    restore = true;
                    restoretimer = current_tick + HZ;
                }
                break;

                /* fast forward / rewind */
            case WPS_FFWD:
            case WPS_REW:
#ifdef WPS_RC_FFWD
            case WPS_RC_FFWD:
            case WPS_RC_RWD:
#endif
                ffwd_rew(button);
                break;

                /* prev / restart */
#ifdef WPS_RC_PREV
            case WPS_RC_PREV:
#endif
            case WPS_PREV:
                /* ignore release event after rewind */
                if (lastbutton & BUTTON_REPEAT)
                    break; 

                if (!id3 || (id3->elapsed < 3*1000)) {
                    mpeg_prev();
                }
                else {
                    if (!paused)
                        mpeg_pause();

                    mpeg_ff_rewind(0);

                    if (!paused)
                        mpeg_resume();
                }
                break;

                /* next */
#ifdef WPS_RC_NEXT
            case WPS_RC_NEXT:
#endif
            case WPS_NEXT:
#if CONFIG_KEYPAD == RECORDER_PAD
                if (lastbutton & BUTTON_REPEAT)
                    break; 
#endif
                mpeg_next();
                break;

                /* menu key functions */
            case WPS_MENU:
#ifdef WPS_MENU_PRE
                if (lastbutton != WPS_MENU_PRE)
                    break;
#endif
                lcd_stop_scroll();

                if (main_menu())
                    return true;
#ifdef HAVE_LCD_BITMAP
                if (global_settings.statusbar)
                    lcd_setmargins(0, STATUSBAR_HEIGHT);
                else
                    lcd_setmargins(0, 0);
#endif
                restore = true;
                break;

            /* key lock */
            case WPS_KEYLOCK:
            case WPS_KEYLOCK | BUTTON_REPEAT:
                keys_locked = !keys_locked;
                display_keylock_text(keys_locked);
                restore = true;
                waitfor_nokey();
                break;

#if CONFIG_KEYPAD == RECORDER_PAD
                /* play settings */
            case BUTTON_F2:
                if (quick_screen(CONTEXT_WPS, BUTTON_F2))
                    return SYS_USB_CONNECTED;
                restore = true;
                break;

                /* screen settings */
            case BUTTON_F3:
                if (quick_screen(CONTEXT_WPS, BUTTON_F3))
                    return SYS_USB_CONNECTED;
                restore = true;
                break;

                /* pitch screen */
            case BUTTON_ON | BUTTON_REPEAT:
                if (2 == pitch_screen())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif

                /* stop and exit wps */
#ifdef WPS_EXIT
            case WPS_EXIT:
#ifdef WPS_RC_EXIT
            case WPS_RC_EXIT:
#endif
                exit = true;
                break;
#endif

#ifdef WPS_ID3
            case WPS_ID3:
                browse_id3();
                restore = true;
                break;
#endif
                
            case BUTTON_NONE: /* Timeout */
                update_track = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return SYS_USB_CONNECTED;
                break;
        }

        if (update_track)
        {
            if (update())
            {
                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    current_track_path[0] != '\0')
                    set_current_file(current_track_path);
                
                return 0;
            }
            update_track = false;
        }

        if (exit) {
#ifdef HAVE_LCD_CHARCELLS
            status_set_record(false);
            status_set_audio(false);
#endif
            if (global_settings.fade_on_stop)
                fade(0);
                
            lcd_stop_scroll();
            bookmark_autobookmark();
            mpeg_stop();

            /* Keys can be locked when exiting, so either unlock here
               or implement key locking in tree.c too */
            keys_locked=false;

            /* set dir browser to current playing song */
            if (global_settings.browse_current &&
                current_track_path[0] != '\0')
                set_current_file(current_track_path);
            
            return 0;
        }
                    
        if ( button )
            ata_spin();

        if (restore &&
            ((restoretimer == 0) ||
             (restoretimer < current_tick)))
        {
            restore = false;
            restoretimer = 0;
            if (wps_display(id3, nid3))
            {
                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    current_track_path[0] != '\0')
                    set_current_file(current_track_path);

                return 0;
            }

            if (id3)
                wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
    return 0; /* unreachable - just to reduce compiler warnings */
}

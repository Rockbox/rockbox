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
#include "audio.h"
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
#endif
#include "action.h"
#include "lang.h"
#include "bookmark.h"
#include "misc.h"
#include "sound.h"
#include "onplay.h"

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

bool keys_locked = false;
static bool ff_rewind = false;
static bool paused = false;
static struct mp3entry* id3 = NULL;
static struct mp3entry* nid3 = NULL;
static char current_track_path[MAX_PATH+1];

/* set volume
   return true if screen restore is needed
   return false otherwise
*/
static bool setvol(void)
{
    if (global_settings.volume < sound_min(SOUND_VOLUME))
        global_settings.volume = sound_min(SOUND_VOLUME);
    if (global_settings.volume > sound_max(SOUND_VOLUME))
        global_settings.volume = sound_max(SOUND_VOLUME);
    sound_set(SOUND_VOLUME, global_settings.volume);
    status_draw(false);
    wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);
    settings_save();
#ifdef HAVE_LCD_CHARCELLS
    splash(0, false, "Vol: %d %%   ",
           sound_val2phys(SOUND_VOLUME, global_settings.volume));
    return true;
#endif
    return false;
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
#ifdef WPS_RC_FFWD 
            case WPS_RC_FFWD:
#endif
                 direction = 1;
            case WPS_REW:
#ifdef WPS_RC_REW
            case WPS_RC_REW:
#endif
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
                    if ( (audio_status() & AUDIO_STATUS_PLAY) &&
                          id3 && id3->length )
                    {
                        if (!paused)
                            audio_pause();
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
#ifdef WPS_RC_PREV
            case WPS_RC_PREV:
            case WPS_RC_NEXT:
#endif
                audio_ff_rewind(id3->elapsed+ff_rewind_count);
                ff_rewind_count = 0;
                ff_rewind = false;
                status_set_ffmode(0);
                if (!paused)
                    audio_resume();
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

    /* let audio thread update id3->elapsed before calling wps_refresh */
    yield(); 
    wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);
    return usb;
}

static bool update(void)
{
    bool track_changed = audio_has_changed_track();
    bool retcode = false;

    nid3 = audio_next_track();
    if (track_changed)
    {
        lcd_stop_scroll();
        id3 = audio_current_track();
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
         (global_settings.resume_offset != id3->offset || track_changed)) {
 
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
    unsigned fp_global_vol = global_settings.volume << 8;
    unsigned fp_step = fp_global_vol / 30;

    if (fade_in) {
        /* fade in */
        unsigned fp_volume = 0;

        /* zero out the sound */
        sound_set(SOUND_VOLUME, 0);

        sleep(HZ/10); /* let audio thread run */
        audio_resume();
        
        while (fp_volume < fp_global_vol) {
            fp_volume += fp_step;
            sleep(1);
            sound_set(SOUND_VOLUME, fp_volume >> 8);
        }
        sound_set(SOUND_VOLUME, global_settings.volume);
    }
    else {
        /* fade out */
        unsigned fp_volume = fp_global_vol;

        while (fp_volume > fp_step) {
            fp_volume -= fp_step;
            sleep(1);
            sound_set(SOUND_VOLUME, fp_volume >> 8);
        }
        audio_pause();
#ifndef SIMULATOR
        /* let audio thread run and wait for the mas to run out of data */
        while (!mp3_pause_done())
#endif
            sleep(HZ/10);

        /* reset volume to what it was before the fade */
        sound_set(SOUND_VOLUME, global_settings.volume);
    }
}


#ifdef WPS_KEYLOCK
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

static void waitfor_nokey(void)
{
    /* wait until all keys are released */
    while (button_get(false) != BUTTON_NONE)
        yield();
}
#endif

/* demonstrates showing different formats from playtune */
long wps_show(void)
{
    long button = 0, lastbutton = 0;
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

    if(audio_status() & AUDIO_STATUS_PLAY)
    {
        id3 = audio_current_track();
        nid3 = audio_next_track();
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
        bool audio_paused = (audio_status() & AUDIO_STATUS_PAUSE)?true:false;
        
        /* did someone else (i.e power thread) change audio pause mode? */
        if (paused != audio_paused) {
            paused = audio_paused;

            /* if another thread paused audio, we are probably in car mode,
               about to shut down. lets save the settings. */
            if (paused) {
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

        /* Exit if audio has stopped playing. This can happen if using the
           sleep timer with the charger plugged or if starting a recording
           from F1 */
        if (!audio_status())
            exit = true;

        switch(button)
        {
#ifdef WPS_CONTEXT
            case WPS_CONTEXT:
                onplay(id3->path, TREE_ATTR_MPA, CONTEXT_WPS);
                restore = true;
                break;
#endif

#ifdef WPS_RC_BROWSE
            case WPS_RC_BROWSE:
#endif
            case WPS_BROWSE:
#ifdef WPS_BROWSE_PRE
                if ((lastbutton != WPS_BROWSE_PRE)
#ifdef WPS_RC_BROWSE_PRE
                    && (lastbutton != WPS_RC_BROWSE_PRE)
#endif
                    )
                    break;
#endif
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
#ifdef WPS_PAUSE_PRE
                if (lastbutton != WPS_PAUSE_PRE)
                    break;
#endif
#ifdef WPS_RC_PAUSE
            case WPS_RC_PAUSE:
#ifdef WPS_RC_PAUSE_PRE
                if ((button == WPS_RC_PAUSE) && (lastbutton != WPS_RC_PAUSE_PRE))
                    break;
#endif
#endif
                if ( paused )
                {
                    paused = false;
                    if ( global_settings.fade_on_stop )
                        fade(1);
                    else
                        audio_resume();
                }
                else
                {
                    paused = true;
                    if ( global_settings.fade_on_stop )
                        fade(0);
                    else
                        audio_pause();
                    settings_save();
#if !defined(HAVE_RTC) && !defined(HAVE_SW_POWEROFF)
                    ata_flush();   /* make sure resume info is saved */
#endif
                }
                break;

                /* volume up */
            case WPS_INCVOL:
            case WPS_INCVOL | BUTTON_REPEAT:
#ifdef WPS_RC_INCVOL
            case WPS_RC_INCVOL:
            case WPS_RC_INCVOL | BUTTON_REPEAT:
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
            case WPS_RC_DECVOL | BUTTON_REPEAT:
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
            case WPS_RC_REW:
#endif
                ffwd_rew(button);
                break;

                /* prev / restart */
            case WPS_PREV:
#ifdef WPS_PREV_PRE
                if (lastbutton != WPS_PREV_PRE)
                    break;
#endif
#ifdef WPS_RC_PREV
            case WPS_RC_PREV:
#ifdef WPS_RC_PREV_PRE
                if ((button == WPS_RC_PREV) && (lastbutton != WPS_RC_PREV_PRE))
                    break;
#endif
#endif
                if (!id3 || (id3->elapsed < 3*1000)) {
                    audio_prev();
                }
                else {
                    if (!paused)
                        audio_pause();

                    audio_ff_rewind(0);

                    if (!paused)
                        audio_resume();
                }
                break;

                /* next */
            case WPS_NEXT:
#ifdef WPS_NEXT_PRE
                if (lastbutton != WPS_NEXT_PRE)
                    break; 
#endif
#ifdef WPS_RC_NEXT
            case WPS_RC_NEXT:
#ifdef WPS_RC_NEXT_PRE
                if ((button == WPS_RC_NEXT) && (lastbutton != WPS_RC_NEXT_PRE))
                    break;
#endif
#endif
                audio_next();
                break;

#ifdef WPS_MENU
            /* menu key functions */
            case WPS_MENU:
#ifdef WPS_MENU_PRE
                if (lastbutton != WPS_MENU_PRE)
                    break;
#endif
#ifdef WPS_RC_MENU
            case WPS_RC_MENU:
#ifdef WPS_RC_MENU_PRE
                if ((button == WPS_RC_MENU) && (lastbutton != WPS_RC_MENU_PRE))
                    break;
#endif
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
#endif /* WPS_MENU */

#ifdef WPS_KEYLOCK
            /* key lock */
            case WPS_KEYLOCK:
            case WPS_KEYLOCK | BUTTON_REPEAT:
                keys_locked = !keys_locked;
                display_keylock_text(keys_locked);
                restore = true;
                waitfor_nokey();
                break;
#endif

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD)
                /* play settings */
            case WPS_QUICK:
                if (quick_screen(CONTEXT_WPS, WPS_QUICK))
                    return SYS_USB_CONNECTED;
                restore = true;
		lastbutton = 0;
                break;

                /* screen settings */
#ifdef BUTTON_F3
            case BUTTON_F3:
                if (quick_screen(CONTEXT_WPS, BUTTON_F3))
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif

                /* pitch screen */
#if CONFIG_KEYPAD == RECORDER_PAD
            case BUTTON_ON | BUTTON_REPEAT:
                if (2 == pitch_screen())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif
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
            audio_stop();

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

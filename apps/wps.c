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

#if defined(HAVE_PLAYER_KEYPAD) || defined(HAVE_NEO_KEYPAD)
void player_change_volume(int button)
{
    bool exit = false;
    char buffer[32];

    lcd_stop_scroll();
    while (!exit)
    {
        switch (button)
        {
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                wps_refresh(id3, nid3, 0, WPS_REFRESH_NON_STATIC);
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_REL:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REL:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REL:
                exit = true;
                break;
        }

        snprintf(buffer,sizeof(buffer),"Vol: %d %%       ", 
                 mpeg_val2phys(SOUND_VOLUME, global_settings.volume));

#ifdef HAVE_LCD_CHARCELLS
        lcd_puts(0, 0, buffer);
#else
        lcd_puts(2, 3, buffer);
        lcd_update();
#endif
        status_draw(false);

        if (!exit)
            button = button_get(true);
    }
    wps_refresh(id3, nid3, 0, WPS_REFRESH_ALL);
}
#endif

void display_keylock_text(bool locked)
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

void display_mute_text(bool muted)
{
    char *s;
    lcd_stop_scroll();
#ifdef HAVE_LCD_CHARCELLS
    if (muted)
        s = str(LANG_MUTE_ON_PLAYER);
    else
        s = str(LANG_MUTE_OFF_PLAYER);
#else
    if (muted)
        s = str(LANG_MUTE_ON_RECORDER);
    else
        s = str(LANG_MUTE_OFF_RECORDER);
#endif
    splash(HZ, true, s);
}

bool browse_id3(void)
{
    int button;
    int menu_pos = 0;
    int menu_max = 8;
    bool exit = false;
    char scroll_text[MAX_PATH];

    if (!(mpeg_status() & MPEG_STATUS_PLAY))
        return false;

    while (!exit)
    {
        lcd_clear_display();

        switch (menu_pos)
        {
            case 0:
                lcd_puts(0, 0, str(LANG_ID3_TITLE));
                lcd_puts_scroll(0, 1, id3->title ? id3->title : 
                                (char*)str(LANG_ID3_NO_TITLE));
                break;

            case 1:
                lcd_puts(0, 0, str(LANG_ID3_ARTIST));
                lcd_puts_scroll(0, 1, 
                                id3->artist ? id3->artist : 
                                (char*)str(LANG_ID3_NO_ARTIST));
                break;

            case 2:
                lcd_puts(0, 0, str(LANG_ID3_ALBUM));
                lcd_puts_scroll(0, 1, id3->album ? id3->album : 
                                (char*)str(LANG_ID3_NO_ALBUM));
                break;

            case 3:
                lcd_puts(0, 0, str(LANG_ID3_TRACKNUM));
                
                if (id3->tracknum) {
                    snprintf(scroll_text,sizeof(scroll_text), "%d",
                             id3->tracknum);
                    lcd_puts_scroll(0, 1, scroll_text);
                }
                else
                    lcd_puts_scroll(0, 1, str(LANG_ID3_NO_TRACKNUM));
                break;

            case 4:
                lcd_puts(0, 0, str(LANG_ID3_GENRE));
                lcd_puts_scroll(0, 1,
                                id3_get_genre(id3) ?
                                id3_get_genre(id3) :
                                (char*)str(LANG_ID3_NO_INFO));
                break;

            case 5:
                lcd_puts(0, 0, str(LANG_ID3_YEAR));
                if (id3->year) {
                    snprintf(scroll_text,sizeof(scroll_text), "%d",
                             id3->year);
                    lcd_puts_scroll(0, 1, scroll_text);
                }
                else
                    lcd_puts_scroll(0, 1, str(LANG_ID3_NO_INFO));
                break;

            case 6:
                lcd_puts(0, 0, str(LANG_ID3_LENGHT));
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                         id3->length / 60000,
                         id3->length % 60000 / 1000 );
                lcd_puts(0, 1, scroll_text);
                break;

            case 7:
                lcd_puts(0, 0, str(LANG_ID3_PLAYLIST));
                snprintf(scroll_text,sizeof(scroll_text), "%d/%d",
                         playlist_get_display_index(), playlist_amount());
                lcd_puts_scroll(0, 1, scroll_text);
                break;


            case 8:
                lcd_puts(0, 0, str(LANG_ID3_BITRATE));
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                         id3->bitrate);
                lcd_puts(0, 1, scroll_text);
                break;

            case 9:
                lcd_puts(0, 0, str(LANG_ID3_FRECUENCY));
                snprintf(scroll_text,sizeof(scroll_text), "%d Hz",
                         id3->frequency);
                lcd_puts(0, 1, scroll_text);
                break;

            case 10:
                lcd_puts(0, 0, str(LANG_ID3_PATH));
                lcd_puts_scroll(0, 1, id3->path);
                break;
        }
        lcd_update();

        button = button_get(true);

        switch(button)
        {
            case BUTTON_LEFT:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
#endif
                if (menu_pos > 0)
                    menu_pos--;
                else
                    menu_pos = menu_max;
                break;

            case BUTTON_RIGHT:
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
#endif
                if (menu_pos < menu_max)
                    menu_pos++;
                else
                    menu_pos = 0;
                break;
            
            case BUTTON_REPEAT:
                break;

#ifdef BUTTON_STOP
            case BUTTON_STOP:
#else
            case BUTTON_OFF:
#endif
            case BUTTON_PLAY:
                lcd_stop_scroll();
                /* eat release event */
                button_get(true);
                exit = true;
                break;

            default:
                if(default_event_handler(button) ==  SYS_USB_CONNECTED)
                    return true;
                break;
        }
    }
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
    int direction = 1;         /* forward=1 or backward=-1 */
    long accel_tick = 0;       /* next time at which to bump the step size */
    bool exit = false;
    bool usb = false;

    while (!exit) {
        switch ( button ) {
            case BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
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
#ifdef HAVE_PLAYER_KEYPAD
                        lcd_stop_scroll();
#endif
                        direction = (button & BUTTON_RIGHT) ? 1 : -1;

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

            case BUTTON_LEFT | BUTTON_REL:
            case BUTTON_RIGHT | BUTTON_REL: 
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

static bool menu(void)
{
    static bool muted = false;
    bool exit = false;
    int last_button = 0;

#ifdef HAVE_LCD_CHARCELLS
    status_set_param(true);
    status_draw(false);
#endif

    while (!exit) {
        int button = button_get(true);

        /* these are never locked */
        switch (button)
        {
            /* key lock */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
                keys_locked = !keys_locked;
                display_keylock_text(keys_locked);
                exit = true;
                while (button_get(false)); /* clear button queue */
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    keys_locked = false;
                    return true;
                }
                break;
        }

        if (keys_locked) {
            display_keylock_text(true);
            break;
        }
        
        switch ( button ) {
            /* go into menu */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_REL:
#else
            case BUTTON_MENU | BUTTON_REL:
#endif
                exit = true;
                if ( !last_button && !keys_locked ) {
                    lcd_stop_scroll();

                    if (main_menu())
                        return true;
#ifdef HAVE_LCD_BITMAP
                    if(global_settings.statusbar)
                        lcd_setmargins(0, STATUSBAR_HEIGHT);
                    else
                        lcd_setmargins(0, 0);
#endif
                }
                break;

                /* mute */
#ifdef BUTTON_MENU
            case BUTTON_MENU | BUTTON_PLAY:
#else
            case BUTTON_F1 | BUTTON_PLAY:
#endif
                if ( muted )
                    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                else
                    mpeg_sound_set(SOUND_VOLUME, 0);
                muted = !muted;
#ifdef HAVE_LCD_CHARCELLS
                status_set_param(false);
#endif
                display_mute_text(muted);
                break;

#ifdef BUTTON_MENU
                /* change volume */
            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                player_change_volume(button);
                exit = true;
                break;

                /* show id3 tags */
#ifdef BUTTON_ON
            case BUTTON_MENU | BUTTON_ON:
                status_set_param(true);
                status_set_audio(true);
#endif
#else
            case BUTTON_F1 | BUTTON_ON:
#endif
                lcd_clear_display();
                lcd_puts(0, 0, str(LANG_ID3_INFO));
                lcd_puts(0, 1, str(LANG_ID3_SCREEN));
                lcd_update();
                sleep(HZ);
 
                if(browse_id3())
                    return true;
#ifdef HAVE_PLAYER_KEYPAD
                status_set_param(false);
                status_set_audio(true);
#endif
                exit = true;
                break;
        }
        last_button = button;
    }

#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
#endif

    return false;
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


/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    int button = 0, lastbutton = 0;
    bool ignore_keyup = true;
    bool restore = false;
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
        
        /* ignore non-remote buttons when keys are locked */
        if (keys_locked &&
            ! ((button < 0) ||
#ifdef HAVE_RECORDER_KEYPAD
               (button & BUTTON_F1) ||
#else
               (button & BUTTON_MENU) ||
#endif
               (button == BUTTON_NONE)
#ifdef BUTTON_REMOTE
               || (button & BUTTON_REMOTE)
#endif
               ))
        {
            while (button_get(false)); /* clear button queue */
            display_keylock_text(true);
            restore = true;
            continue;
        }

        /* Exit if mpeg has stopped playing. This can happen if using the
           sleep timer with the charger plugged or if starting a recording
           from F1 */
        if (!mpeg_status())
            exit = true;

        switch(button)
        {
#ifdef BUTTON_ON
            case BUTTON_ON:
#ifdef HAVE_RECORDER_KEYPAD
                switch (on_screen()) {
                    case 2:
                        /* usb connected? */
                        return SYS_USB_CONNECTED;
                
                    case 1:
                        /* was on_screen used? */
                        restore = true;

                        /* pause may have been turned off by pitch screen */
                        if (paused && !(mpeg_status() & MPEG_STATUS_PAUSE)) {
                            paused = false;
                        }
                        break;

                    case 0:
                        /* otherwise, exit to browser */
#else
                        status_set_record(false);
                        status_set_audio(false);
#endif
                        lcd_stop_scroll();

                        /* set dir browser to current playing song */
                        if (global_settings.browse_current &&
                            current_track_path[0] != '\0')
                            set_current_file(current_track_path);
                        
                        return 0;
#ifdef HAVE_RECORDER_KEYPAD
                }
                break;
#endif
#endif /* BUTTON_ON */
                /* play/pause */
            case BUTTON_PLAY:
#ifdef BUTTON_RC_PLAY
            case BUTTON_RC_PLAY:
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
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
#endif
#ifdef BUTTON_RC_VOL_UP
            case BUTTON_RC_VOL_UP:
#endif
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw(false);
                settings_save();
                break;

                /* volume down */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#endif
#ifdef BUTTON_RC_VOL_DOWN
            case BUTTON_RC_VOL_DOWN:
#endif
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw(false);
                settings_save();
                break;

                /* fast forward / rewind */
            case BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                ffwd_rew(button);
                break;

                /* prev / restart */
#ifdef BUTTON_RC_LEFT
            case BUTTON_RC_LEFT:
#endif
            case BUTTON_LEFT | BUTTON_REL:
#ifdef HAVE_RECORDER_KEYPAD
                if ((button == (BUTTON_LEFT | BUTTON_REL)) &&
                    (lastbutton != BUTTON_LEFT ))
                    break; 
#endif
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
#ifdef BUTTON_RC_RIGHT
            case BUTTON_RC_RIGHT:
#endif
            case BUTTON_RIGHT | BUTTON_REL:
#ifdef HAVE_RECORDER_KEYPAD
                if ((button == (BUTTON_RIGHT | BUTTON_REL)) &&
                    (lastbutton != BUTTON_RIGHT))
                     break; 
#endif
                mpeg_next();
                break;

                /* menu key functions */
#ifdef BUTTON_MENU
            case BUTTON_MENU:
#else
            case BUTTON_F1:
#endif
                if (menu())
                    return SYS_USB_CONNECTED;

                update_track = true;
                restore = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
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
#endif

                /* stop and exit wps */
#ifdef BUTTON_OFF
            case BUTTON_OFF | BUTTON_REL:
#else
            case BUTTON_STOP | BUTTON_REL:
                if ( lastbutton != BUTTON_STOP )
                    break;
#endif
#ifdef BUTTON_RC_STOP
            case BUTTON_RC_STOP:
#endif
                exit = true;
                break;

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

        if (restore) {
            restore = false;
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
        if(button != BUTTON_NONE)
            lastbutton = button;
    }
    return 0; /* unreachable - just to reduce compiler warnings */
}

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
#include "usb.h"
#include "status.h"
#include "main_menu.h"
#include "ata.h"
#include "screens.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "peakmeter.h"
#endif
#include "lang.h"
#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */

bool keys_locked = false;
static bool ff_rewind = false;
static bool paused = false;
static struct mp3entry* id3 = NULL;

#ifdef HAVE_PLAYER_KEYPAD
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
                wps_refresh(id3,0,false);
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                wps_refresh(id3,0,false);
                settings_save();
                break;

            case BUTTON_MENU | BUTTON_REL:
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
        if (!exit)
            button = button_get(true);
    }
    status_draw();
    wps_refresh(id3,0,true);
}
#endif

void display_keylock_text(bool locked)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if(locked)
        lcd_puts(0, 0, str(LANG_KEYLOCK_ON_PLAYER));
    else
        lcd_puts(0, 0, str(LANG_KEYLOCK_OFF_PLAYER));
#else
    if(locked)
        lcd_puts(2, 3, str(LANG_KEYLOCK_ON_RECORDER));
    else
        lcd_puts(2, 3, str(LANG_KEYLOCK_OFF_RECORDER));
    lcd_update();
#endif
    
    sleep(HZ);
}

void display_mute_text(bool muted)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if (muted)
        lcd_puts(0, 0, str(LANG_MUTE_ON_PLAYER));
    else
        lcd_puts(0, 0, str(LANG_MUTE_OFF_PLAYER));
#else
    if (muted)
        lcd_puts(2, 3, str(LANG_MUTE_ON_RECORDER));
    else
        lcd_puts(2, 3, str(LANG_MUTE_OFF_RECORDER));
    lcd_update();
#endif
    
    sleep(HZ);
}

static int browse_id3(void)
{
    int button;
    int menu_pos = 0;
    int menu_max = 8;
    bool exit = false;
    char scroll_text[MAX_PATH];

    lcd_stop_scroll();
    lcd_clear_display();
    lcd_puts(0, 0, str(LANG_ID3_INFO));
    lcd_puts(0, 1, str(LANG_ID3_SCREEN));
    lcd_update();
    sleep(HZ);
 
    while (!exit)
    {
        lcd_stop_scroll();
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
                
                if (id3->tracknum)
                {
                    snprintf(scroll_text,sizeof(scroll_text), "%d",
                             id3->tracknum);
                    lcd_puts_scroll(0, 1, scroll_text);
                }
                else
                {
                    lcd_puts_scroll(0, 1, str(LANG_ID3_NO_TRACKNUM));
                }
                break;

            case 4:
                lcd_puts(0, 0, str(LANG_ID3_LENGHT));
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                         id3->length / 60000,
                         id3->length % 60000 / 1000 );
                lcd_puts(0, 1, scroll_text);
                break;

            case 5:
                lcd_puts(0, 0, str(LANG_ID3_PLAYLIST));
                snprintf(scroll_text,sizeof(scroll_text), "%d/%d",
                         id3->index + 1, playlist_amount());
                lcd_puts_scroll(0, 1, scroll_text);
                break;


            case 6:
                lcd_puts(0, 0, str(LANG_ID3_BITRATE));
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                         id3->bitrate);
                lcd_puts(0, 1, scroll_text);
                break;

            case 7:
                lcd_puts(0, 0, str(LANG_ID3_FRECUENCY));
                snprintf(scroll_text,sizeof(scroll_text), "%d Hz",
                         id3->frequency);
                lcd_puts(0, 1, scroll_text);
                break;

            case 8:
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

#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_STOP:
#else
            case BUTTON_OFF:
#endif
            case BUTTON_PLAY:
                lcd_stop_scroll();
                exit = true;
                break;

            case SYS_USB_CONNECTED: 
                status_set_playmode(STATUS_STOP);
                usb_screen();
                return SYS_USB_CONNECTED;
                break;
        }
    }
    return 0;
}

static bool ffwd_rew(int button)
{
    static int ff_rew_steps[] = {
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
                    ff_rewind_count += step * direction;

                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= accel_tick)
                    { 
                        step *= 2;
                        if (step > max_step)
                            step = max_step;

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
                            status_set_playmode(STATUS_FASTFORWARD);
                        else
                        status_set_playmode(STATUS_FASTBACKWARD);

                        ff_rewind = true;

                        step = ff_rew_steps[global_settings.ff_rewind_min_step];

                        max_step = id3->length * FF_REWIND_MAX_PERCENT / 100;

                        if (step > max_step)
                            step = max_step;

                        ff_rewind_count = step * direction;
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
                    wps_refresh(id3, -ff_rewind_count, false);
                else
                    wps_refresh(id3, ff_rewind_count, false);

                break;

            case BUTTON_LEFT | BUTTON_REL:
            case BUTTON_RIGHT | BUTTON_REL: 
                mpeg_ff_rewind(ff_rewind_count);
                ff_rewind_count = 0;
                ff_rewind = false;
                if (paused)
                    status_set_playmode(STATUS_PAUSE);
                else {
                    mpeg_resume();
                    status_set_playmode(STATUS_PLAY);
                }
#ifdef HAVE_LCD_CHARCELLS
                if (wps_display(id3))
                    return true;
#endif
                exit = true;
                break;

            case SYS_USB_CONNECTED:
                status_set_playmode(STATUS_STOP);
                usb_screen();
                usb = true;
                exit = true;
                break;
        }
        if (!exit)
            button = button_get(true);
    }
    wps_refresh(id3,0,true);
    return usb;
}

static bool update(void)
{
    bool track_changed = mpeg_has_changed_track();

    if (track_changed)
    {
        lcd_stop_scroll();
        id3 = mpeg_current_track();
        if (wps_display(id3))
            return true;
        wps_refresh(id3,0,true);
    }

    if (id3)
        wps_refresh(id3,0,false);

    status_draw();

    /* save resume data */
    if ( id3 && 
         global_settings.resume &&
         global_settings.resume_offset != id3->offset ) {
        DEBUGF("R%X,%X (%X)\n", global_settings.resume_offset,
               id3->offset,id3);
        global_settings.resume_index = id3->index;
        global_settings.resume_offset = id3->offset;
        settings_save();
    }
    else if ( !id3 && track_changed ) {
        global_settings.resume_index = -1;
        global_settings.resume_offset = -1;
        settings_save();
    }

    return false;
}


static bool keylock(void)
{
    bool exit = false;

#ifdef HAVE_LCD_CHARCELLS
    status_set_record(true);
    status_set_param(false);
#endif
    display_keylock_text(true);
    keys_locked = true;
    wps_refresh(id3,0,true);
    if (wps_display(id3))
        return true;
    status_draw();
    while (button_get(false)); /* clear button queue */

    while (!exit) {
        switch ( button_get_w_tmo(HZ/5) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
            case BUTTON_F1 | BUTTON_REPEAT | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
            case BUTTON_MENU | BUTTON_REPEAT | BUTTON_STOP:
#endif
#ifdef HAVE_LCD_CHARCELLS
                status_set_record(false);
#endif
                display_keylock_text(false);
                keys_locked = false;
                exit = true;
                while (button_get(false)); /* clear button queue */
                break;

            case SYS_USB_CONNECTED:
                status_set_playmode(STATUS_STOP);
                usb_screen();
                return true;

            case BUTTON_NONE:
                update();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
            case BUTTON_F1 | BUTTON_REPEAT:
#else
            case BUTTON_MENU:
            case BUTTON_MENU | BUTTON_REPEAT:
#endif
                /* ignore menu key, to avoid displaying "Keylock ON"
                   every time we unlock the keys */
                break;

            default:
                display_keylock_text(true);
                while (button_get(false)); /* clear button queue */
                wps_refresh(id3,0,true);
                if(wps_display(id3))
                    return true;
                break;
        }
    }
    return false;
}

static bool menu(void)
{
    static bool muted = false;
    bool exit = false;
    int last_button = 0;

#ifdef HAVE_LCD_CHARCELLS
    status_set_param(true);
    status_draw();
#endif

    while (!exit) {
        int button = button_get(true);
        
        switch ( button ) {
            /* go into menu */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_REL:
#else
            case BUTTON_MENU | BUTTON_REL:
#endif
                exit = true;
                if ( !last_button ) {
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
#ifdef HAVE_PLAYER_KEYPAD
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

                /* key lock */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
                if (keylock())
                    return true;
                exit = true;
                break;

#ifdef HAVE_PLAYER_KEYPAD
                /* change volume */
            case BUTTON_MENU | BUTTON_LEFT:
            case BUTTON_MENU | BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                player_change_volume(button);
                exit = true;
                break;

                /* show id3 tags */
            case BUTTON_MENU | BUTTON_ON:
                status_set_param(true);
                status_set_audio(true);
#else
            case BUTTON_F1 | BUTTON_ON:
#endif
                if(browse_id3() == SYS_USB_CONNECTED)
                    return true;
#ifdef HAVE_PLAYER_KEYPAD
                status_set_param(false);
                status_set_audio(true);
#endif
                exit = true;
                break;

            case SYS_USB_CONNECTED:
                status_set_playmode(STATUS_STOP);
                usb_screen();
                return true;
        }
        last_button = button;
    }

#ifdef HAVE_LCD_CHARCELLS
    status_set_param(false);
#endif

    if (wps_display(id3))
        return true;
    wps_refresh(id3,0,true);
    return false;
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    int button = 0, lastbutton = 0;
    bool ignore_keyup = true;
    bool restore = false;

    id3 = NULL;

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
        if (id3) {
            if (wps_display(id3))
                return 0;
            wps_refresh(id3,0,true);
        }
        restore = true;
    }

    while ( 1 )
    {

#ifdef HAVE_LCD_BITMAP
        /* when the peak meter is enabled we want to have a
            few extra updates to make it look smooth. On the
            other hand we don't want to waste energy if it 
            isn't displayed */
        if (peak_meter_enabled) {
            int i;
            for (i = 0; i < 4; i++) {
                button = button_get_w_tmo(HZ / 20);
                if (button != 0) {
                    break;
                }
                wps_refresh(id3, 0, false);
            }
        } else {
            button = button_get_w_tmo(HZ/5);
        }
#else
        button = button_get_w_tmo(HZ/5);
#endif

        /* discard first event if it's a button release */
        if (button && ignore_keyup)
        {
            ignore_keyup = false;
            if (button & BUTTON_REL && button != SYS_USB_CONNECTED)
                continue;
        }
        
        switch(button)
        {
            case BUTTON_ON:
#ifdef HAVE_RECORDER_KEYPAD
                switch (on_screen()) {
                    case 2:
                        /* usb connected? */
                        return SYS_USB_CONNECTED;
                
                    case 1:
                        /* was on_screen used? */
                        restore = true;
                        break;

                    case 0:
                        /* otherwise, exit to browser */
#else
                        status_set_record(false);
                        status_set_audio(false);
#endif
                        lcd_stop_scroll();

                        /* set dir browser to current playing song */
                        if (global_settings.browse_current && id3)
                            set_current_file(id3->path);
                        
                        return 0;
#ifdef HAVE_RECORDER_KEYPAD
                }
                break;
#endif

                /* play/pause */
            case BUTTON_PLAY:
                if ( paused )
                {
                    mpeg_resume();
                    paused = false;
                    status_set_playmode(STATUS_PLAY);
                }
                else
                {
                    mpeg_pause();
                    paused = true;
                    status_set_playmode(STATUS_PAUSE);
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
            case BUTTON_VOL_UP:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

                /* volume down */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
#endif
            case BUTTON_VOL_DOWN:
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                status_draw();
                settings_save();
                break;

                /* fast forward / rewind */
            case BUTTON_LEFT | BUTTON_REPEAT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                ffwd_rew(button);
                break;

                /* prev / restart */
            case BUTTON_LEFT | BUTTON_REL:
#ifdef HAVE_RECORDER_KEYPAD
                if ( lastbutton != BUTTON_LEFT )   
                     break; 
#endif
                if (!id3 || (id3->elapsed < 3*1000))
                    mpeg_prev();
                else {
                    if (!paused)
                        mpeg_pause();

                    mpeg_ff_rewind(-(id3->elapsed));

                    if (!paused)
                        mpeg_resume();
                }
                break;

                /* next */
            case BUTTON_RIGHT | BUTTON_REL:
#ifdef HAVE_RECORDER_KEYPAD
                if ( lastbutton != BUTTON_RIGHT )   
                     break; 
#endif
                mpeg_next();
                break;

                /* menu key functions */
#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_MENU:
#else
            case BUTTON_F1:
#endif
                if (menu())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;

#ifdef HAVE_RECORDER_KEYPAD
                /* play settings */
            case BUTTON_F2:
                if (f2_screen())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;

                /* screen settings */
            case BUTTON_F3:
                if (f3_screen())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif

                /* stop and exit wps */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
#else
            case BUTTON_STOP | BUTTON_REL:
                if ( lastbutton != BUTTON_STOP )
                    break;
#endif
#ifdef HAVE_LCD_CHARCELLS
                status_set_record(false);
                status_set_audio(false);
#endif
                lcd_stop_scroll();

                /* set dir browser to current playing song */
                if (global_settings.browse_current && id3)
                    set_current_file(id3->path);

                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                return 0;
                    
            case SYS_USB_CONNECTED:
                status_set_playmode(STATUS_STOP);
                usb_screen();
                return SYS_USB_CONNECTED;

            case BUTTON_NONE: /* Timeout */
                if (update())
                    return 0;
                break;
        }

        if ( button )
            ata_spin();

        if (restore) {
            restore = false;
            if (wps_display(id3))
                return 0;
            if (id3)
                wps_refresh(id3,0,false);
        }
        if(button != BUTTON_NONE)
            lastbutton = button;
    }
    return 0; /* unreachable - just to reduce compiler warnings */
}

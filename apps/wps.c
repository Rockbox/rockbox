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
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#endif

#ifdef LOADABLE_FONTS
#include "ajf.h"
#endif

#define FF_REWIND_MIN_STEP 1000 /* minimum ff/rewind step is 1 second */
#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */

#ifdef HAVE_RECORDER_KEYPAD
#define RELEASE_MASK (BUTTON_F1 | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP)
#else
#define RELEASE_MASK (BUTTON_MENU | BUTTON_STOP | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_PLAY)
#endif

bool keys_locked = false;
static bool ff_rewind = false;
static bool paused = false;
static struct mp3entry* id3 = NULL;
static int old_release_mask;


void display_volume_level(int vol_level)
{
    char buffer[32];

    lcd_stop_scroll();
    snprintf(buffer,sizeof(buffer),"Vol: %d %%       ", vol_level * 2);

#ifdef HAVE_LCD_CHARCELLS
    lcd_puts(0, 0, buffer);
#else
    lcd_puts(2, 3, buffer);
    lcd_update();
#endif

    sleep(HZ/6);
}

void display_keylock_text(bool locked)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if(locked)
        lcd_puts(0, 0, "Keylock ON");
    else
        lcd_puts(0, 0, "Keylock OFF");
#else
    if(locked)
    {
        lcd_puts(2, 3, "Key lock is ON");
    }
    else
    {
        lcd_puts(2, 3, "Key lock is OFF");
    }
    lcd_update();
#endif
    
    sleep(HZ);
}

void display_mute_text(bool muted)
{
    lcd_stop_scroll();
    lcd_clear_display();

#ifdef HAVE_LCD_CHARCELLS
    if(muted)
        lcd_puts(0, 0, "Mute ON");
    else
        lcd_puts(0, 0, "Mute OFF");
#else
    if(muted)
    {
        lcd_puts(2, 3, "Mute is ON");
    }
    else
    {
        lcd_puts(2, 3, "Mute is OFF");
    }
    lcd_update();
#endif
    
    sleep(HZ);
}

static void handle_usb(void)
{
#ifndef SIMULATOR
#ifdef HAVE_LCD_BITMAP
    bool laststate=statusbar(false);
#endif
    /* Tell the USB thread that we are safe */
    DEBUGF("wps got SYS_USB_CONNECTED\n");
    usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
    /* Wait until the USB cable is extracted again */
    usb_wait_for_disconnect(&button_queue);

#ifdef HAVE_LCD_BITMAP
    statusbar(laststate);
#endif
#endif
}

#ifdef HAVE_PLAYER_KEYPAD
int player_id3_show(void)
{
    int button;
    int menu_pos = 0;
    int menu_max = 6;
    bool menu_changed = true;
    char scroll_text[MAX_PATH];

    lcd_stop_scroll();
    lcd_clear_display();
    lcd_puts(0, 0, "-ID3 Info- ");
    lcd_puts(0, 1, "--Screen-- ");
    sleep(HZ*1.5);
 
    while(1)
    {
        button = button_get(false);

        switch(button)
        {
            case BUTTON_LEFT:
                menu_changed = true;
                if (menu_pos > 0)
                    menu_pos--;
                else
                    menu_pos = menu_max;
                break;

            case BUTTON_RIGHT:
                menu_changed = true;
                if (menu_pos < menu_max)
                    menu_pos++;
                else
                    menu_pos = 0;
                break;
            
            case BUTTON_REPEAT:
                break;

            case BUTTON_STOP:
            case BUTTON_PLAY:
                lcd_stop_scroll();
                wps_display(id3);
                return(0);
                break;

#ifndef SIMULATOR
            case SYS_USB_CONNECTED: 
                /* Tell the USB thread that we are safe */
                DEBUGF("wps got SYS_USB_CONNECTED\n");
                usb_acknowledge(SYS_USB_CONNECTED_ACK);
                    
                /* Wait until the USB cable is extracted again */
                usb_wait_for_disconnect(&button_queue);

                /* Signal to our caller that we have been in USB mode */
                return SYS_USB_CONNECTED;
                break;
#endif

        }

        switch(menu_pos)
        {
            case 0:
                lcd_puts(0, 0, "[Title]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->title?id3->title:"<no title>");
                break;
            case 1:
                lcd_puts(0, 0, "[Artist]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->artist?id3->artist:"<no artist>");
                break;
            case 2:
                lcd_puts(0, 0, "[Album]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->album?id3->album:"<no album>");
                break;
            case 3:
                lcd_puts(0, 0, "[Length]");
                snprintf(scroll_text,sizeof(scroll_text), "%d:%02d",
                   id3->length / 60000,
                   id3->length % 60000 / 1000 );
                break;
            case 4:
                lcd_puts(0, 0, "[Bitrate]");
                snprintf(scroll_text,sizeof(scroll_text), "%d kbps", 
                        id3->bitrate);
                break;
            case 5:
                lcd_puts(0, 0, "[Frequency]");
                snprintf(scroll_text,sizeof(scroll_text), "%d kHz",
                        id3->frequency);
                break;
            case 6:
                lcd_puts(0, 0, "[Path]");
                snprintf(scroll_text,sizeof(scroll_text), "%s",
                        id3->path);
                break;
        }

        if (menu_changed == true)
        {
            menu_changed = false;
            lcd_stop_scroll();
            lcd_clear_display();
            lcd_puts_scroll(0, 1, scroll_text);
        }

        lcd_update();
        yield();
    }
    return 0;
}
#endif

static bool ffwd_rew(int button)
{
    unsigned int ff_rewind_step = 0; /* current rewind step size */ 
    unsigned int ff_rewind_max_step = 0; /* max rewind step size */ 
    int ff_rewind_count = 0;
    long ff_rewind_accel_tick = 0; /* next time to bump ff/rewind step size */ 
    bool exit = false;
    bool usb = false;

    while (!exit) {
        switch ( button ) {
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (ff_rewind)
                {
                    ff_rewind_count -= ff_rewind_step; 
                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= ff_rewind_accel_tick) 
                    { 
                        ff_rewind_step *= 2; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    } 
                }
                else
                {
                    if ( mpeg_is_playing() && id3 && id3->length )
                    {
                        if (!paused)
                            mpeg_pause();
#ifdef HAVE_PLAYER_KEYPAD
                        lcd_stop_scroll();
#endif
                        status_set_playmode(STATUS_FASTBACKWARD);
                        ff_rewind = true;
                        ff_rewind_max_step = 
                            id3->length * FF_REWIND_MAX_PERCENT / 100; 
                        ff_rewind_step = FF_REWIND_MIN_STEP;
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_count = -ff_rewind_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if ((int)(id3->elapsed + ff_rewind_count) < 0)
                    ff_rewind_count = -id3->elapsed;

                wps_refresh(id3, ff_rewind_count, false);
                break;

            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (ff_rewind)
                {
                    ff_rewind_count += ff_rewind_step; 
                    if (global_settings.ff_rewind_accel != 0 && 
                        current_tick >= ff_rewind_accel_tick) 
                    { 
                        ff_rewind_step *= 2; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    } 
                }
                else
                {
                    if ( mpeg_is_playing() && id3 && id3->length )
                    {
                        if (!paused)
                            mpeg_pause();
#ifdef HAVE_PLAYER_KEYPAD
                        lcd_stop_scroll();
#endif
                        status_set_playmode(STATUS_FASTFORWARD);
                        ff_rewind = true;
                        ff_rewind_max_step = 
                            id3->length * FF_REWIND_MAX_PERCENT / 100; 
                        ff_rewind_step = FF_REWIND_MIN_STEP; 
                        if (ff_rewind_step > ff_rewind_max_step) 
                            ff_rewind_step = ff_rewind_max_step; 
                        ff_rewind_count = ff_rewind_step; 
                        ff_rewind_accel_tick = current_tick + 
                            global_settings.ff_rewind_accel*HZ; 
                    }
                    else
                        break;
                }

                if ((id3->elapsed + ff_rewind_count) > id3->length)
                    ff_rewind_count = id3->length - id3->elapsed;

                wps_refresh(id3, ff_rewind_count, false);
                break;

            case BUTTON_LEFT | BUTTON_REL:
                /* rewind */
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
                wps_display(id3);
#endif
                exit = true;
                break;

            case BUTTON_RIGHT | BUTTON_REL: 
                /* fast forward */
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
                wps_display(id3);
#endif
                exit = true;
                break;

            case SYS_USB_CONNECTED:
                handle_usb();
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

static void update(void)
{
    if (mpeg_has_changed_track())
    {
        lcd_stop_scroll();
        id3 = mpeg_current_track();
        wps_display(id3);
        wps_refresh(id3,0,true);
    }

    if (id3) {
        wps_refresh(id3,0,false);
    }

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
}


static bool keylock(void)
{
    bool exit = false;

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_RECORD, true);
    lcd_icon(ICON_PARAM, false);
#endif
    display_keylock_text(true);
    keys_locked = true;
    wps_refresh(id3,0,true);
    wps_display(id3);
    status_draw();
    while (button_get(false)); /* clear button queue */

    while (!exit) {
        switch ( button_get_w_tmo(HZ/5) ) {
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1 | BUTTON_DOWN:
#else
            case BUTTON_MENU | BUTTON_STOP:
#endif
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
#endif
                display_keylock_text(false);
                keys_locked = false;
                exit = true;
                while (button_get(false)); /* clear button queue */
                break;

            case SYS_USB_CONNECTED:
                handle_usb();
                return true;

            case BUTTON_NONE:
                update();
                break;

#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F1:
#else
            case BUTTON_MENU:
#endif
                /* ignore menu key, to avoid displaying "Keylock ON"
                   every time we unlock the keys */
                break;

            default:
                display_keylock_text(true);
                while (button_get(false)); /* clear button queue */
                wps_refresh(id3,0,true);
                wps_display(id3);
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
    lcd_icon(ICON_PARAM, true);
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
                    button_set_release(old_release_mask);
                    main_menu();
                    old_release_mask = button_set_release(RELEASE_MASK);
                }
                break;

                /* mute */
#ifdef HAVE_PLAYER_KEYPAD
            case BUTTON_MENU | BUTTON_PLAY:
#else
            case BUTTON_F1 | BUTTON_UP:
#endif
                if ( muted )
                    mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                else
                    mpeg_sound_set(SOUND_VOLUME, 0);
                muted = !muted;
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_PARAM, false);
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
                global_settings.volume--;
                if(global_settings.volume < mpeg_sound_min(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_min(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                wps_display(id3);
                status_draw();
                settings_save();
                break;

                /* change volume */
            case BUTTON_MENU | BUTTON_RIGHT:
            case BUTTON_MENU | BUTTON_RIGHT | BUTTON_REPEAT:
                global_settings.volume++;
                if(global_settings.volume > mpeg_sound_max(SOUND_VOLUME))
                    global_settings.volume = mpeg_sound_max(SOUND_VOLUME);
                mpeg_sound_set(SOUND_VOLUME, global_settings.volume);
                display_volume_level(global_settings.volume);
                wps_display(id3);
                status_draw();
                settings_save();
                break;

                /* show id3 tags */
            case BUTTON_MENU | BUTTON_ON:
                lcd_stop_scroll();
                lcd_icon(ICON_PARAM, true);
                lcd_icon(ICON_AUDIO, true);
                if(player_id3_show() == SYS_USB_CONNECTED)
                    return true;
                lcd_icon(ICON_PARAM, false);
                lcd_icon(ICON_AUDIO, true);
                wps_display(id3);
                exit = true;
                break;
#endif

            case SYS_USB_CONNECTED:
                handle_usb();
                return true;
        }
        last_button = button;
    }

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_PARAM, false);
#endif

    wps_display(id3);
    wps_refresh(id3,0,true);
    return false;
}

/* demonstrates showing different formats from playtune */
int wps_show(void)
{
    int button;
    bool ignore_keyup = true;
    bool restore = false;

    id3 = NULL;

    old_release_mask = button_set_release(RELEASE_MASK);

#ifdef HAVE_LCD_CHARCELLS
    lcd_icon(ICON_AUDIO, true);
    lcd_icon(ICON_PARAM, false);
#else
    if(global_settings.statusbar)
        lcd_setmargins(0, STATUSBAR_HEIGHT);
    else
        lcd_setmargins(0, 0);
#endif

    ff_rewind = false;

    if(mpeg_is_playing())
    {
        id3 = mpeg_current_track();
        if (id3) {
            wps_display(id3);
            wps_refresh(id3,0,true);
        }
        restore = true;
    }

    while ( 1 )
    {
        button = button_get_w_tmo(HZ/5);

        /* discard first event if it's a button release */
        if (button && ignore_keyup)
        {
            ignore_keyup = false;
            if (button & BUTTON_REL && button != SYS_USB_CONNECTED)
                continue;
        }
        
        switch(button)
        {
            /* exit to dir browser */
            case BUTTON_ON:
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                button_set_release(old_release_mask);
                return 0;
                
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
#ifdef HAVE_LCD_BITMAP
                if(global_settings.statusbar)
                    lcd_setmargins(0, STATUSBAR_HEIGHT);
                else
                    lcd_setmargins(0, 0);
#endif
                restore = true;
                break;

                /* toggle status bar */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_F3:
#ifdef HAVE_LCD_BITMAP
                global_settings.statusbar = !global_settings.statusbar;
                settings_save();
                if(global_settings.statusbar)
                    lcd_setmargins(0, STATUSBAR_HEIGHT);
                else
                    lcd_setmargins(0, 0);
                restore = true;
#endif
                break;
#endif

                /* stop and exit wps */
#ifdef HAVE_RECORDER_KEYPAD
            case BUTTON_OFF:
#else
            case BUTTON_STOP:
#endif
#ifdef HAVE_LCD_CHARCELLS
                lcd_icon(ICON_RECORD, false);
                lcd_icon(ICON_AUDIO, false);
#endif
                mpeg_stop();
                status_set_playmode(STATUS_STOP);
                button_set_release(old_release_mask);
                return 0;
                    
#ifndef SIMULATOR
            case SYS_USB_CONNECTED:
                handle_usb();
                return SYS_USB_CONNECTED;
#endif

            case BUTTON_NONE: /* Timeout */
                update();
                break;
        }

        if ( button )
            ata_spin();

        if (restore) {
            restore = false;
            wps_display(id3);
            if (id3)
                wps_refresh(id3,0,false);
        }
    }
}

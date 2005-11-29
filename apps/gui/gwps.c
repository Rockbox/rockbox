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
#include "gwps.h"
#include "gwps-common.h"
#include "audio.h"
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
#include "abrepeat.h"
#include "playback.h"

#include "splash.h"

#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"
/* currently only on wps_state is needed */
struct wps_state wps_state;
struct gui_wps gui_wps[NB_SCREENS];
static struct wps_data wps_datas[NB_SCREENS];

bool keys_locked = false;

/* change the path to the current played track */
static void wps_state_update_ctp(const char *path);

#ifdef HAVE_LCD_BITMAP
static void gui_wps_set_margin(struct gui_wps *gwps)
{
    int offset = 0;
    struct wps_data *data = gwps->data;
    if(data->wps_sb_tag && data->show_sb_on_wps)
        offset = STATUSBAR_HEIGHT;
    else if ( global_settings.statusbar && !data->wps_sb_tag)
        offset = STATUSBAR_HEIGHT;
    gwps->display->setmargins(0, offset);
}
#endif

long gui_wps_show(void)
{
    long button = 0, lastbutton = 0;
    bool ignore_keyup = true;
    bool restore = false;
    long restoretimer = 0; /* timer to delay screen redraw temporarily */
    bool exit = false;
    bool update_track = false;
    unsigned long right_lastclick = 0;
    unsigned long left_lastclick = 0;
    int i;

    wps_state_init();

#ifdef HAVE_LCD_CHARCELLS
    status_set_audio(true);
    status_set_param(false);
#else
    FOR_NB_SCREENS(i)
    {
        gui_wps_set_margin(&gui_wps[i]);
    }
#endif

#ifdef AB_REPEAT_ENABLE
    ab_repeat_init();
    ab_reset_markers();
#endif

    if(audio_status() & AUDIO_STATUS_PLAY)
    {
        wps_state.id3 = audio_current_track();
        wps_state.nid3 = audio_next_track();
        if (wps_state.id3) {
            if (gui_wps_display())
                return 0;
            FOR_NB_SCREENS(i)
                gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_ALL);
            wps_state_update_ctp(wps_state.id3->path);
        }

        restore = true;
    }
    while ( 1 )
    {
        bool audio_paused = (audio_status() & AUDIO_STATUS_PAUSE)?true:false;
        
        /* did someone else (i.e power thread) change audio pause mode? */
        if (wps_state.paused != audio_paused) {
            wps_state.paused = audio_paused;

            /* if another thread paused audio, we are probably in car mode,
               about to shut down. lets save the settings. */
            if (wps_state.paused) {
                settings_save();
#if !defined(HAVE_RTC) && !defined(HAVE_SW_POWEROFF)
                ata_flush();
#endif
            }
        }

#ifdef HAVE_LCD_BITMAP
        /* when the peak meter is enabled we want to have a
            few extra updates to make it look smooth. On the
            other hand we don't want to waste energy if it
            isn't displayed */
        bool pm=false;
        FOR_NB_SCREENS(i)
        {
           if(gui_wps[i].data->peak_meter_enabled)
               pm = true;
        }
        
        if (pm) {
            long next_refresh = current_tick;
            long next_big_refresh = current_tick + HZ / 5;
            button = BUTTON_NONE;
            while (TIME_BEFORE(current_tick, next_big_refresh)) {
                button = button_get(false);
                if (button != BUTTON_NONE) {
                    break;
                }
                peak_meter_peek();
                sleep(0);   /* Sleep until end of current tick. */  

                if (TIME_AFTER(current_tick, next_refresh)) {
                    FOR_NB_SCREENS(i)
                    {
                        if(gui_wps[i].data->peak_meter_enabled)
                            gui_wps_refresh(&gui_wps[i], 0,
                                            WPS_REFRESH_PEAK_METER);
                        next_refresh += HZ / PEAK_METER_FPS;
                    }
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
#ifdef WPS_RC_CONTEXT
            case WPS_RC_CONTEXT:
#endif
                onplay(wps_state.id3->path, TREE_ATTR_MPA, CONTEXT_WPS);
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif
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
                FOR_NB_SCREENS(i)
                    gui_wps[i].display->stop_scroll();

                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    wps_state.current_track_path[0] != '\0')
                    set_current_file(wps_state.current_track_path);

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
                if ((button == WPS_RC_PAUSE) &&
                    (lastbutton != WPS_RC_PAUSE_PRE))
                    break;
#endif
#endif
                if ( wps_state.paused )
                {
                    wps_state.paused = false;
                    if ( global_settings.fade_on_stop )
                        fade(1);
                    else
                        audio_resume();
                }
                else
                {
                    wps_state.paused = true;
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
            {
                global_settings.volume++;
                bool res = false;
                setvol();
                FOR_NB_SCREENS(i)
                {
                    if(update_onvol_change(&gui_wps[i]))
                        res = true;
                }
                if (res) {
                    restore = true;
                    restoretimer = current_tick + HZ;
                }
            }
                break;

                /* volume down */
            case WPS_DECVOL:
            case WPS_DECVOL | BUTTON_REPEAT:
#ifdef WPS_RC_DECVOL
            case WPS_RC_DECVOL:
            case WPS_RC_DECVOL | BUTTON_REPEAT:
#endif
            {
                global_settings.volume--;
                setvol();
                bool res = false;
                FOR_NB_SCREENS(i)
                {
                    if(update_onvol_change(&gui_wps[i]))
                        res = true;
                }
                if (res) {
                    restore = true;
                    restoretimer = current_tick + HZ;
                }
            }
                break;

                /* fast forward / rewind */
#ifdef WPS_RC_FFWD
            case WPS_RC_FFWD:
#endif
            case WPS_FFWD:
#ifdef WPS_NEXT_DIR
                if (current_tick - right_lastclick < HZ)
                {
                    audio_next_dir();
                    right_lastclick = 0;
                    break;
                }
#endif
#ifdef WPS_RC_REW
            case WPS_RC_REW:
#endif
            case WPS_REW:
#ifdef WPS_PREV_DIR
                if (current_tick - left_lastclick < HZ)
                {
                    audio_prev_dir();
                    left_lastclick = 0;
                    break;
                }
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
                left_lastclick = current_tick;
                update_track = true;

#ifdef AB_REPEAT_ENABLE
                /* if we're in A/B repeat mode and the current position
                   is past the A marker, jump back to the A marker... */
                if ( ab_repeat_mode_enabled() 
                     && ab_after_A_marker(wps_state.id3->elapsed) )
                {
                    ab_jump_to_A_marker();
                    break;
                }
                /* ...otherwise, do it normally */
#endif

                if (!wps_state.id3 || (wps_state.id3->elapsed < 3*1000)) {
                    audio_prev();
                }
                else {
                    if (!wps_state.paused)
                        audio_pause();

                    audio_ff_rewind(0);

                    if (!wps_state.paused)
                        audio_resume();
                }
                break;

#ifdef WPS_NEXT_DIR
#ifdef WPS_RC_NEXT_DIR
            case WPS_RC_NEXT_DIR:
#endif
            case WPS_NEXT_DIR:
                audio_next_dir();
                break;
#endif
#ifdef WPS_PREV_DIR
#ifdef WPS_RC_PREV_DIR
            case WPS_RC_PREV_DIR:
#endif
            case WPS_PREV_DIR:
                audio_prev_dir();
                break;
#endif

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
                right_lastclick = current_tick;
                update_track = true;

#ifdef AB_REPEAT_ENABLE
                /* if we're in A/B repeat mode and the current position is
                   before the A marker, jump to the A marker... */
                if ( ab_repeat_mode_enabled() 
                     && ab_before_A_marker(wps_state.id3->elapsed) )
                {
                    ab_jump_to_A_marker();
                    break;
                }
                /* ...otherwise, do it normally */
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
                FOR_NB_SCREENS(i)
                    gui_wps[i].display->stop_scroll();

                if (main_menu())
                    return true;
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
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
#ifdef WPS_RC_QUICK
            case WPS_RC_QUICK:
#endif
                if (quick_screen_quick(button))
                    return SYS_USB_CONNECTED;
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif
                restore = true;
                lastbutton = 0;
                break;

                /* screen settings */
#ifdef BUTTON_F3
            case BUTTON_F3:
                if (quick_screen_f3(button))
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif

                /* pitch screen */
#if CONFIG_KEYPAD == RECORDER_PAD || CONFIG_KEYPAD == IRIVER_H100_PAD
            case BUTTON_ON | BUTTON_UP:
            case BUTTON_ON | BUTTON_DOWN:
                if (2 == pitch_screen())
                    return SYS_USB_CONNECTED;
                restore = true;
                break;
#endif
#endif

#ifdef AB_REPEAT_ENABLE

#ifdef WPS_AB_SET_A_MARKER
            /* set A marker for A-B repeat */
            case WPS_AB_SET_A_MARKER:
                if (ab_repeat_mode_enabled())
                    ab_set_A_marker(wps_state.id3->elapsed);
                break;
#endif

#ifdef WPS_AB_SET_B_MARKER
            /* set B marker for A-B repeat and jump to A */
            case WPS_AB_SET_B_MARKER:
                if (ab_repeat_mode_enabled())
                {
                    ab_set_B_marker(wps_state.id3->elapsed);
                    ab_jump_to_A_marker();
                    update_track = true;
                }
                break;
#endif

#ifdef WPS_AB_RESET_AB_MARKERS
            /* reset A&B markers */
            case WPS_AB_RESET_AB_MARKERS:
                if (ab_repeat_mode_enabled())
                {
                    ab_reset_markers();
                    update_track = true;
                }
                break;
#endif

#endif /* AB_REPEAT_ENABLE */

                /* stop and exit wps */
#ifdef WPS_EXIT
            case WPS_EXIT:
# ifdef WPS_EXIT_PRE
                if (lastbutton != WPS_EXIT_PRE)
                    break;
# endif
                exit = true;
#ifdef WPS_RC_EXIT
            case WPS_RC_EXIT:
#ifdef WPS_RC_EXIT_PRE
                 if (lastbutton != WPS_RC_EXIT_PRE)
                     break;
#endif
                exit = true;
#endif
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

            case SYS_POWEROFF:
                bookmark_autobookmark();
                default_event_handler(SYS_POWEROFF);
                break;
                
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return SYS_USB_CONNECTED;
                update_track = true;
                break;
        }

        if (update_track)
        {
            bool update_failed = false;
            FOR_NB_SCREENS(i)
            {
                if(update(&gui_wps[i]))
                    update_failed = true;
            }
            if (update_failed)
            {
                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    wps_state.current_track_path[0] != '\0')
                    set_current_file(wps_state.current_track_path);
                
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
                
            FOR_NB_SCREENS(i)
                gui_wps[i].display->stop_scroll();
            bookmark_autobookmark();
            audio_stop();
#ifdef AB_REPEAT_ENABLE
            ab_reset_markers();
#endif

            /* Keys can be locked when exiting, so either unlock here
               or implement key locking in tree.c too */
            keys_locked=false;

            /* set dir browser to current playing song */
            if (global_settings.browse_current &&
                wps_state.current_track_path[0] != '\0')
                set_current_file(wps_state.current_track_path);
            
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
            if (gui_wps_display())
            {
                /* set dir browser to current playing song */
                if (global_settings.browse_current &&
                    wps_state.current_track_path[0] != '\0')
                    set_current_file(wps_state.current_track_path);

                return 0;
            }

            if (wps_state.id3){
                FOR_NB_SCREENS(i)
                    gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_NON_STATIC);
            }
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
    return 0; /* unreachable - just to reduce compiler warnings */
}

/* needs checking if needed end*/

/* wps_data*/
/* initial setup of wps_data */
void wps_data_init(struct wps_data *wps_data)
{
    int i;
#ifdef HAVE_LCD_BITMAP
    for (i = 0; i < MAX_IMAGES; i++) {
        wps_data->img[i].loaded = false;
        wps_data->img[i].display = false;
        wps_data->img[i].always_display = false;
    }
    wps_data->wps_sb_tag = false;
    wps_data->show_sb_on_wps = false;
#else /* HAVE_LCD_CHARCELLS */
    for(i = 0; i < 8; i++)
        wps_data->wps_progress_pat[i] = 0;
    wps_data->full_line_progressbar = 0;
#endif
    wps_data->format_buffer[0] = '\0';
    wps_data->wps_loaded = false;
    wps_data->peak_meter_enabled = false;
}

#ifdef HAVE_LCD_BITMAP
/* Clear the WPS image cache */
static void wps_clear(struct wps_data *data )
{
    int i;
    /* set images to unloaded and not displayed */
    for (i = 0; i < MAX_IMAGES; i++) {
       data->img[i].loaded = false;
       data->img[i].display = false;
       data->img[i].always_display = false;
    }
}
#else
#define wps_clear(a)
#endif

static void wps_reset(struct wps_data *data)
{
    data->wps_loaded = false;
    memset(&data->format_buffer, 0, sizeof data->format_buffer);
    wps_clear(data);
}

/* to setup up the wps-data from a format-buffer (isfile = false)
   from a (wps-)file (isfile = true)*/
bool wps_data_load(struct wps_data *wps_data,
                   const char *buf,
                   bool isfile,
                   bool display)
{
    int i, s;
    int fd;

    if(!wps_data || !buf)
        return false;

    if(!isfile)
    {
        wps_clear(wps_data);
        strncpy(wps_data->format_buffer, buf, sizeof(wps_data->format_buffer));
        wps_data->format_buffer[sizeof(wps_data->format_buffer) - 1] = 0;
        gui_wps_format(wps_data, NULL, 0);
        return true;
    }
    else
    {
        /* 
         * Hardcode loading WPS_DEFAULTCFG to cause a reset ideally this 
         * wants to be a virtual file.  Feel free to modify dirbrowse()
         * if you're feeling brave.
         */
        if (! strcmp(buf, WPS_DEFAULTCFG) )
        {
            wps_reset(wps_data);
	    global_settings.wps_file[0] = 0;
            return false;
        } 

#ifdef HAVE_REMOTE_LCD
	if (! strcmp(buf, RWPS_DEFAULTCFG) )
       	{
            wps_reset(wps_data);
	    global_settings.rwps_file[0] = 0;
	    return false;
	}
#endif
	
        size_t bmpdirlen;
        char *bmpdir = strrchr(buf, '.');
        bmpdirlen = bmpdir - buf;
        
        fd = open(buf, O_RDONLY);

        if (fd >= 0)
        {
            int numread = read(fd, wps_data->format_buffer,
                               sizeof(wps_data->format_buffer) - 1);
    
            if (numread > 0)
            {
#ifdef HAVE_LCD_BITMAP
                wps_clear(wps_data);
#endif
                wps_data->format_buffer[numread] = 0;
                gui_wps_format(wps_data, buf, bmpdirlen);
            }
    
            close(fd);
    
            if ( display ) {
                bool any_defined_line;
                int z;
                FOR_NB_SCREENS(z)
                    screens[z].clear_display();
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(z)
                    screens[z].setmargins(0,0);
#endif
                for (s=0; s<WPS_MAX_SUBLINES; s++)
                {
                    any_defined_line = false;
                    for (i=0; i<WPS_MAX_LINES; i++)
                    {
                        if (wps_data->format_lines[i][s] &&
                            wps_data->format_lines[i][s][0])
                        {
                            FOR_NB_SCREENS(z)
                                screens[z].puts(0, i,
                                                wps_data->
                                                format_lines[i][s]);
                            any_defined_line = true;
                        }
                        else
                        {
                            FOR_NB_SCREENS(z)
                                screens[z].puts(0, i, " ");
                        }
                    }
                    if (any_defined_line)
                    {
#ifdef HAVE_LCD_BITMAP
                        FOR_NB_SCREENS(z)
                            screens[z].update();
#endif
                        sleep(HZ/2);
                    }
                }
            }
            wps_data->wps_loaded = true;
    
            return numread > 0;
        }
    }

    return false;
}

/* wps_data end */

/* wps_state */

void wps_state_init(void)
{
    wps_state.ff_rewind = false;
    wps_state.paused = false;
    wps_state.id3 = NULL;
    wps_state.nid3 = NULL;
    wps_state.current_track_path[0] = '\0';
}

#if 0
/* these are obviously not used? */

void wps_state_update_ff_rew(bool ff_rew)
{
    wps_state.ff_rewind = ff_rew;
}

void wps_state_update_paused(bool paused)
{
    wps_state.paused = paused;
}
void wps_state_update_id3_nid3(struct mp3entry *id3, struct mp3entry *nid3)
{
    wps_state.id3 = id3;
    wps_state.nid3 = nid3;
}
#endif

static void wps_state_update_ctp(const char *path)
{
    memcpy(wps_state.current_track_path, path,
           sizeof(wps_state.current_track_path));
}
/* wps_state end*/

/* initial setup of a wps */
void gui_wps_init(struct gui_wps *gui_wps)
{
    gui_wps->data = NULL;
    gui_wps->display = NULL;
    gui_wps->statusbar = NULL;
    /* Currently no seperate wps_state needed/possible
       so use the only aviable ( "global" ) one */
    gui_wps->state = &wps_state;
}

/* connects a wps with a format-description of the displayed content */
void gui_wps_set_data(struct gui_wps *gui_wps, struct wps_data *data)
{
    gui_wps->data = data;
}

/* connects a wps with a screen */
void gui_wps_set_disp(struct gui_wps *gui_wps, struct screen *display)
{
    gui_wps->display = display;
}

void gui_wps_set_statusbar(struct gui_wps *gui_wps, struct gui_statusbar *statusbar)
{
    gui_wps->statusbar = statusbar;
}
/* gui_wps end */

void gui_sync_wps_screen_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_wps_set_disp(&gui_wps[i], &screens[i]);
}

void gui_sync_wps_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        wps_data_init(&wps_datas[i]);
        gui_wps_init(&gui_wps[i]);
        gui_wps_set_data(&gui_wps[i], &wps_datas[i]);
        gui_wps_set_statusbar(&gui_wps[i], &statusbars.statusbars[i]);
    }
}

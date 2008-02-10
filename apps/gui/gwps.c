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
#include "config.h"

#include "system.h"
#include "file.h"
#include "lcd.h"
#include "font.h"
#include "backlight.h"
#include "action.h"
#include "kernel.h"
#include "filetypes.h"
#include "debug.h"
#include "sprintf.h"
#include "settings.h"
#include "gwps.h"
#include "gwps-common.h"
#include "audio.h"
#include "usb.h"
#include "status.h"
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
#include "cuesheet.h"
#include "ata_idle_notify.h"
#include "root_menu.h"

#if (LCD_DEPTH > 1) || (defined(HAVE_LCD_REMOTE) && (LCD_REMOTE_DEPTH > 1))
#include "backdrop.h"
#endif

/* currently only on wps_state is needed */
struct wps_state wps_state;
struct gui_wps gui_wps[NB_SCREENS];
static struct wps_data wps_datas[NB_SCREENS];

/* change the path to the current played track */
static void wps_state_update_ctp(const char *path);
/* initial setup of wps_data  */
static void wps_state_init(void);
/* initial setup of a wps */
static void gui_wps_init(struct gui_wps *gui_wps);
/* connects a wps with a format-description of the displayed content */
static void gui_wps_set_data(struct gui_wps *gui_wps, struct wps_data *data);
/* connects a wps with a screen */
static void gui_wps_set_disp(struct gui_wps *gui_wps, struct screen *display);
/* connects a wps with a statusbar*/
static void gui_wps_set_statusbar(struct gui_wps *gui_wps, struct gui_statusbar *statusbar);

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
    long button = 0;
    bool restore = false;
    long restoretimer = 0; /* timer to delay screen redraw temporarily */
    bool exit = false;
    bool bookmark = false;
    bool update_track = false;
    int i;
    long last_left = 0, last_right = 0;
    
    wps_state_init();

#ifdef HAVE_LCD_CHARCELLS
    status_set_audio(true);
    status_set_param(false);
#else
    FOR_NB_SCREENS(i)
    {
        gui_wps_set_margin(&gui_wps[i]);
    }
#if LCD_DEPTH > 1
    show_wps_backdrop();
#endif /* LCD_DEPTH > 1 */
#endif

#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    show_remote_wps_backdrop();
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
#if !defined(HAVE_RTC_RAM) && !defined(HAVE_SW_POWEROFF)
                call_ata_idle_notifys(true);
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
                button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,TIMEOUT_NOBLOCK);
                if (button != ACTION_NONE) {
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
            button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,HZ/5);
        }
#else
        button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,HZ/5);
#endif

        /* Exit if audio has stopped playing. This can happen if using the
           sleep timer with the charger plugged or if starting a recording
           from F1 */
        if (!audio_status())
            exit = true;
#ifdef ACTION_WPSAB_SINGLE
        if (!global_settings.party_mode && ab_repeat_mode_enabled())
        {
            static int wps_ab_state = 0;
            if (button == ACTION_WPSAB_SINGLE)
            {
                switch (wps_ab_state)
                {
                    case 0: /* set the A spot */
                        button = ACTION_WPS_ABSETA_PREVDIR;
                        break;
                    case 1: /* set the B spot */
                        button = ACTION_WPS_ABSETB_NEXTDIR;
                        break;
                    case 2:
                        button = ACTION_WPS_ABRESET;
                        break;
                }
                wps_ab_state = (wps_ab_state+1) % 3;
            }
        }
#endif
        switch(button)
        {
            case ACTION_WPS_CONTEXT:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                /* if music is stopped in the context menu we want to exit the wps */
                if (onplay(wps_state.id3->path, 
                           FILE_ATTR_AUDIO, CONTEXT_WPS) == ONPLAY_MAINMENU 
                    || !audio_status())
                    return GO_TO_ROOT;

                /* track might have changed */
                update_track = true;

#if LCD_DEPTH > 1
                show_wps_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_wps_backdrop();
#endif
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif
                restore = true;
                break;

            case ACTION_WPS_BROWSE:
#ifdef HAVE_LCD_CHARCELLS
                status_set_record(false);
                status_set_audio(false);
#endif
                FOR_NB_SCREENS(i)
                    gui_wps[i].display->stop_scroll();
                return GO_TO_PREVIOUS_BROWSER;
                break;

                /* play/pause */
            case ACTION_WPS_PLAY:
                if (global_settings.party_mode)
                    break;
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
#if !defined(HAVE_RTC_RAM) && !defined(HAVE_SW_POWEROFF)
                    call_ata_idle_notifys(true);   /* make sure resume info is saved */
#endif
                }
                break;

                /* volume up */
            case ACTION_WPS_VOLUP:
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
            case ACTION_WPS_VOLDOWN:
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
            /* fast forward 
                OR next dir if this is straight after ACTION_WPS_SKIPNEXT */
            case ACTION_WPS_SEEKFWD:
                if (global_settings.party_mode)
                    break;
                if (current_tick -last_right < HZ)
                {
                    if (cuesheet_is_enabled() && wps_state.id3->cuesheet_type)
                    {
                        audio_next();
                    }
                    else
                    {
                        audio_next_dir();
                    }
                }
                else ffwd_rew(ACTION_WPS_SEEKFWD);
                last_right = 0;
                break;
            /* fast rewind 
                OR prev dir if this is straight after ACTION_WPS_SKIPPREV */
            case ACTION_WPS_SEEKBACK:
                if (global_settings.party_mode)
                    break;
                if (current_tick -last_left < HZ)
                {
                    if (cuesheet_is_enabled() && wps_state.id3->cuesheet_type)
                    {
                        if (!wps_state.paused)
#if (CONFIG_CODEC == SWCODEC)
                            audio_pre_ff_rewind();
#else
                            audio_pause();
#endif
                        audio_ff_rewind(0);
                    }
                    else
                    {
                        audio_prev_dir();
                    }
                }
                else ffwd_rew(ACTION_WPS_SEEKBACK);
                last_left = 0;
                break;

                /* prev / restart */
            case ACTION_WPS_SKIPPREV:
                if (global_settings.party_mode)
                    break;
                last_left = current_tick;
                update_track = true;

#ifdef AB_REPEAT_ENABLE
                /* if we're in A/B repeat mode and the current position
                   is past the A marker, jump back to the A marker... */
                if ( ab_repeat_mode_enabled() )
                {
                    if ( ab_after_A_marker(wps_state.id3->elapsed) )
                    {
                        ab_jump_to_A_marker();
                        break;
#if (AB_REPEAT_ENABLE == 2)
                    } else {
                        ab_reset_markers();
#endif
                    }
                }
                /* ...otherwise, do it normally */
#endif

                if (!wps_state.id3 || (wps_state.id3->elapsed < 3*1000)) {
                    audio_prev();
                }
                else {

                    if (cuesheet_is_enabled() && wps_state.id3->cuesheet_type)
                    {
                        curr_cuesheet_skip(-1, wps_state.id3->elapsed);
                        break;
                    }

                    if (!wps_state.paused)
#if (CONFIG_CODEC == SWCODEC)
                        audio_pre_ff_rewind();
#else
                        audio_pause();
#endif

                    audio_ff_rewind(0);

#if (CONFIG_CODEC != SWCODEC)
                    if (!wps_state.paused)
                        audio_resume();
#endif
                }
                break;

                /* next */
            case ACTION_WPS_SKIPNEXT:
                if (global_settings.party_mode)
                    break;
                last_right = current_tick;
                update_track = true;

#ifdef AB_REPEAT_ENABLE
                /* if we're in A/B repeat mode and the current position is
                   before the A marker, jump to the A marker... */
                if ( ab_repeat_mode_enabled() )
                {
                    if ( ab_before_A_marker(wps_state.id3->elapsed) )
                    {
                        ab_jump_to_A_marker();
                        break;
#if (AB_REPEAT_ENABLE == 2)
                    } else {
                        ab_reset_markers();
#endif
                    }
                }
                /* ...otherwise, do it normally */
#endif

                /* take care of if we're playing a cuesheet */
                if (cuesheet_is_enabled() && wps_state.id3->cuesheet_type)
                {
                    if (curr_cuesheet_skip(1, wps_state.id3->elapsed))
                    {
                        /* if the result was false, then we really want
                           to skip to the next track */
                        break;
                    }
                }

                audio_next();
                break;
                /* next / prev directories */
                /* and set A-B markers if in a-b mode */
            case ACTION_WPS_ABSETB_NEXTDIR:
                if (global_settings.party_mode)
                    break;
#if defined(AB_REPEAT_ENABLE)
                if (ab_repeat_mode_enabled())
                {
                    ab_set_B_marker(wps_state.id3->elapsed);
                    ab_jump_to_A_marker();
                    update_track = true;
                }
                else
#endif
                {
                    audio_next_dir();
                }
                break;
            case ACTION_WPS_ABSETA_PREVDIR:
                if (global_settings.party_mode)
                    break;
#if defined(AB_REPEAT_ENABLE)
                if (ab_repeat_mode_enabled())
                    ab_set_A_marker(wps_state.id3->elapsed);
                else
#endif
                {
                    audio_prev_dir();
                }
                break;
            /* menu key functions */
            case ACTION_WPS_MENU:
                FOR_NB_SCREENS(i)
                    gui_wps[i].display->stop_scroll();
                return GO_TO_ROOT;
                break;


#ifdef HAVE_QUICKSCREEN
            case ACTION_WPS_QUICKSCREEN:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                if (quick_screen_quick(button))
                    return SYS_USB_CONNECTED;
#if LCD_DEPTH > 1
                show_wps_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_wps_backdrop();
#endif
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif
                restore = true;
                break;
#endif /* HAVE_QUICKSCREEN */

                /* screen settings */
#ifdef BUTTON_F3
            case ACTION_F3:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                if (quick_screen_f3(BUTTON_F3))
                    return SYS_USB_CONNECTED;
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif 
                restore = true;
                break;
#endif /* BUTTON_F3 */

                /* pitch screen */
#ifdef HAVE_PITCHSCREEN
            case ACTION_WPS_PITCHSCREEN:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                if (1 == pitch_screen())
                    return SYS_USB_CONNECTED;
#if LCD_DEPTH > 1
                show_wps_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_wps_backdrop();
#endif
                restore = true;
                break;
#endif /* HAVE_PITCHSCREEN */

#ifdef AB_REPEAT_ENABLE
            /* reset A&B markers */
            case ACTION_WPS_ABRESET:
                if (ab_repeat_mode_enabled())
                {
                    ab_reset_markers();
                    update_track = true;
                }
                break;
#endif /* AB_REPEAT_ENABLE */

                /* stop and exit wps */
            case ACTION_WPS_STOP:
                if (global_settings.party_mode)
                    break;
                bookmark = true;
                exit = true;
                break;

            case ACTION_WPS_ID3SCREEN:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                browse_id3();
#if LCD_DEPTH > 1
                show_wps_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_wps_backdrop();
#endif
#ifdef HAVE_LCD_BITMAP
                FOR_NB_SCREENS(i)
                {
                    gui_wps_set_margin(&gui_wps[i]);
                }
#endif
                restore = true;
                break;

            case ACTION_REDRAW: /* yes are locked, just redraw */
                restore = true;
                break;
            case ACTION_NONE: /* Timeout */
                update_track = true;
                ffwd_rew(button); /* hopefully fix the ffw/rwd bug */
                break;
#ifdef HAVE_RECORDING
            case ACTION_WPS_REC:
                exit = true;
                break;
#endif
            case SYS_POWEROFF:
#if LCD_DEPTH > 1
                show_main_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                show_remote_main_backdrop();
#endif
                default_event_handler(SYS_POWEROFF);
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return GO_TO_ROOT;
                update_track = true;
                break;
        }

        if (update_track)
        {
            FOR_NB_SCREENS(i)
            {
                if(update(&gui_wps[i]))
                    exit = true;
            }
            update_track = false;
        }

        if (restore &&
            ((restoretimer == 0) ||
             (restoretimer < current_tick)))
        {
            restore = false;
            restoretimer = 0;
            if (gui_wps_display()) {
                exit = true;
            }
            else if (wps_state.id3){
                FOR_NB_SCREENS(i)
                    gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_NON_STATIC);
            }
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
            if (bookmark)
                bookmark_autobookmark();
            audio_stop();
#ifdef AB_REPEAT_ENABLE
            ab_reset_markers();
#endif
#ifdef HAVE_RECORDING
            if (button == ACTION_WPS_REC)
                return GO_TO_RECSCREEN;
#endif
            if (global_settings.browse_current)
                return GO_TO_PREVIOUS_BROWSER;
            return GO_TO_PREVIOUS;
        }

        if ( button )
            ata_spin();
    }
    return GO_TO_ROOT; /* unreachable - just to reduce compiler warnings */
}

/* needs checking if needed end*/

/* wps_state */

static void wps_state_init(void)
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
    strncpy(wps_state.current_track_path, path,
           sizeof(wps_state.current_track_path));
    wps_state.current_track_path[sizeof(wps_state.current_track_path)-1] = '\0';
}
/* wps_state end*/

/* initial setup of a wps */
static void gui_wps_init(struct gui_wps *gui_wps)
{
    gui_wps->data = NULL;
    gui_wps->display = NULL;
    gui_wps->statusbar = NULL;
    /* Currently no seperate wps_state needed/possible
       so use the only aviable ( "global" ) one */
    gui_wps->state = &wps_state;
}

/* connects a wps with a format-description of the displayed content */
static void gui_wps_set_data(struct gui_wps *gui_wps, struct wps_data *data)
{
    gui_wps->data = data;
}

/* connects a wps with a screen */
static void gui_wps_set_disp(struct gui_wps *gui_wps, struct screen *display)
{
    gui_wps->display = display;
}

static void gui_wps_set_statusbar(struct gui_wps *gui_wps, struct gui_statusbar *statusbar)
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
#ifdef HAVE_REMOTE_LCD
        wps_datas[i].remote_wps = (i != 0);
#endif
        gui_wps_init(&gui_wps[i]);
        gui_wps_set_data(&gui_wps[i], &wps_datas[i]);
        gui_wps_set_statusbar(&gui_wps[i], &statusbars.statusbars[i]);
    }
#if LCD_DEPTH > 1
    unload_wps_backdrop();
#endif
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
    unload_remote_wps_backdrop();
#endif
}

#ifdef HAVE_ALBUMART
/* Returns true if at least one of the gui_wps screens has an album art
   tag in its wps structure */
bool gui_sync_wps_uses_albumart(void)
{
    int  i;
    FOR_NB_SCREENS(i) {
        struct gui_wps *gwps = &gui_wps[i];
        if (gwps->data && (gwps->data->wps_uses_albumart != WPS_ALBUMART_NONE))
            return true;
    }
    return false;
}
#endif

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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "settings.h"
#include "skin_engine/skin_engine.h"
#include "audio.h"
#include "usb.h"
#include "status.h"
#include "storage.h"
#include "screens.h"
#include "playlist.h"
#include "icons.h"
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
#include "backdrop.h"
#include "quickscreen.h"
#include "shortcuts.h"
#include "pitchscreen.h"
#include "appevents.h"
#include "viewport.h"
#include "pcmbuf.h"
#include "option_select.h"
#include "playlist_viewer.h"
#include "wps.h"
#include "statusbar-skinned.h"
#include "skin_engine/wps_internals.h"
#include "open_plugin.h"

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

static struct wps_state wps_state;

/* initial setup of wps_data  */
static void wps_state_init(void);
static void track_info_callback(unsigned short id, void *param);

#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#ifdef HAVE_REMOTE_LCD
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"
#define DEFAULT_WPS(screen) ((screen) == SCREEN_MAIN ? \
                            WPS_DEFAULTCFG:RWPS_DEFAULTCFG)
#else
#define DEFAULT_WPS(screen) (WPS_DEFAULTCFG)
#endif

char* wps_default_skin(enum screen_type screen)
{
    static char *skin_buf[NB_SCREENS] = {
#if LCD_DEPTH > 1
            "%X(d)\n"
#endif
            "%s%?it<%?in<%in. |>%it|%fn>\n"
            "%s%?ia<%ia|%?d(2)<%d(2)|%(root%)>>\n"
            "%s%?id<%id|%?d(1)<%d(1)|%(root%)>> %?iy<%(%iy%)|>\n\n"
            "%al%pc/%pt%ar[%pp:%pe]\n"
            "%fbkBit %?fv<avg|> %?iv<%(id3v%iv%)|%(no id3%)>\n"
            "%pb\n%pm\n",
#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH > 1
            "%X(d)\n"
#endif
            "%s%?ia<%ia|%?d(2)<%d(2)|%(root%)>>\n"
            "%s%?it<%?in<%in. |>%it|%fn>\n"
            "%al%pc/%pt%ar[%pp:%pe]\n"
            "%fbkBit %?fv<avg|> %?iv<%(id3v%iv%)|%(no id3%)>\n"
            "%pb\n",
#endif
        };
    return skin_buf[screen];
}

static void update_non_static(void)
{
    FOR_NB_SCREENS(i)
        skin_update(WPS, i, SKIN_REFRESH_NON_STATIC);
}

void wps_do_action(enum wps_do_action_type action, bool updatewps)
{
    if (action == WPS_PLAYPAUSE) /* toggle the status */
    {
        struct wps_state *state = get_wps_state();
        if (state->paused)
            action = WPS_PLAY;

        state->paused = !state->paused;
    }

    if (action == WPS_PLAY) /* unpause_action */
    {
        audio_resume();
    }
    else /* WPS_PAUSE pause_action */
    {
        audio_pause();

        if (global_settings.pause_rewind) {
            unsigned long elapsed = audio_current_track()->elapsed;
            long newpos = elapsed - (global_settings.pause_rewind * 1000);

            audio_pre_ff_rewind();
            audio_ff_rewind(newpos > 0 ? newpos : 0);
        }
        if (action == WPS_PLAYPAUSE)
        {
            settings_save();
    #if !defined(HAVE_SW_POWEROFF)
            call_storage_idle_notifys(true);   /* make sure resume info is saved */
    #endif
        }
    }

    /* Bugfix only do a skin refresh if in one of the below screens */
    enum current_activity act = get_current_activity();

    bool refresh = (act == ACTIVITY_FM ||
                    act == ACTIVITY_WPS ||
                    act == ACTIVITY_RECORDING);

    if (updatewps && refresh)
        update_non_static();
}

#ifdef HAVE_TOUCHSCREEN
static int skintouch_to_wps(void)
{
    int offset = 0;
    struct wps_state *gstate = get_wps_state();
    struct gui_wps *gwps = skin_get_gwps(WPS, SCREEN_MAIN);
    int button = skin_get_touchaction(gwps, &offset);
    switch (button)
    {
        case ACTION_STD_PREV:
            return ACTION_WPS_SKIPPREV;
        case ACTION_STD_PREVREPEAT:
            return ACTION_WPS_SEEKBACK;
        case ACTION_STD_NEXT:
            return ACTION_WPS_SKIPNEXT;
        case ACTION_STD_NEXTREPEAT:
            return ACTION_WPS_SEEKFWD;
        case ACTION_STD_MENU:
            return ACTION_WPS_MENU;
        case ACTION_STD_CONTEXT:
            return ACTION_WPS_CONTEXT;
        case ACTION_STD_QUICKSCREEN:
            return ACTION_WPS_QUICKSCREEN;
#ifdef HAVE_HOTKEY
        case ACTION_STD_HOTKEY:
            return ACTION_WPS_HOTKEY;
#endif
        case ACTION_TOUCH_SCROLLBAR:
            gstate->id3->elapsed = gstate->id3->length*offset/1000;
            audio_pre_ff_rewind();
            audio_ff_rewind(gstate->id3->elapsed);
            return ACTION_TOUCHSCREEN;
        case ACTION_TOUCH_VOLUME:
        {
            const int min_vol = sound_min(SOUND_VOLUME);
            const int max_vol = sound_max(SOUND_VOLUME);
            const int step_vol = sound_steps(SOUND_VOLUME);

            global_status.volume = from_normalized_volume(offset, min_vol, max_vol, 1000);
            global_status.volume -= (global_status.volume % step_vol);
            setvol();
        }
        return ACTION_TOUCHSCREEN;
    }
    return button;
}
#endif /* HAVE_TOUCHSCREEN */

static bool ffwd_rew(int button, bool seek_from_end)
{
    unsigned int step = 0;     /* current ff/rewind step */
    unsigned int max_step = 0; /* maximum ff/rewind step */
    int ff_rewind_count = 0;   /* current ff/rewind count (in ticks) */
    int direction = -1;         /* forward=1 or backward=-1 */
    bool exit = false;
    bool usb = false;
    bool ff_rewind = false;
    const long ff_rw_accel = (global_settings.ff_rewind_accel + 3);
    struct wps_state *gstate = get_wps_state();
    struct mp3entry *old_id3 = gstate->id3;

    if (button == ACTION_NONE)
    {
        status_set_ffmode(0);
        return usb;
    }
    while (!exit)
    {
        struct mp3entry *id3 = gstate->id3;
        if (id3 != old_id3)
        {
            ff_rewind = false;
            ff_rewind_count = 0;
            old_id3 = id3;
        }
        if (id3 && seek_from_end)
            id3->elapsed = id3->length;

        switch ( button )
        {
            case ACTION_WPS_SEEKFWD:
                 direction = 1;
                 /* Fallthrough */
            case ACTION_WPS_SEEKBACK:
                if (ff_rewind)
                {
                    if (direction == 1)
                    {
                        /* fast forwarding, calc max step relative to end */
                        max_step = (id3->length -
                                    (id3->elapsed +
                                     ff_rewind_count)) *
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

                    /* smooth seeking by multiplying step by: 1 + (2 ^ -accel) */
                    step += step >> ff_rw_accel;
                }
                else
                {
                    if ((audio_status() & AUDIO_STATUS_PLAY) && id3 && id3->length )
                    {
                        audio_pre_ff_rewind();
                        if (direction > 0)
                            status_set_ffmode(STATUS_FASTFORWARD);
                        else
                            status_set_ffmode(STATUS_FASTBACKWARD);

                        ff_rewind = true;

                        step = 1000 * global_settings.ff_rewind_min_step;
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

                /* set the wps state ff_rewind_count so the progess info
                   displays corectly */
                gstate->ff_rewind_count = ff_rewind_count;

                FOR_NB_SCREENS(i)
                {
                    skin_update(WPS, i,
                                SKIN_REFRESH_PLAYER_PROGRESS |
                                SKIN_REFRESH_DYNAMIC);
                }

                break;

            case ACTION_WPS_STOPSEEK:
                id3->elapsed = id3->elapsed + ff_rewind_count;
                audio_ff_rewind(id3->elapsed);
                gstate->ff_rewind_count = 0;
                ff_rewind = false;
                status_set_ffmode(0);
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
        {
            button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,TIMEOUT_BLOCK);
#ifdef HAVE_TOUCHSCREEN
            if (button == ACTION_TOUCHSCREEN)
                button = skintouch_to_wps();
#endif
            if (button != ACTION_WPS_SEEKFWD
                && button != ACTION_WPS_SEEKBACK
                && button != 0 && !IS_SYSEVENT(button))
                button = ACTION_WPS_STOPSEEK;
        }
    }
    return usb;
}

static void gwps_caption_backlight(struct wps_state *state)
{
#if defined(HAVE_BACKLIGHT) || defined(HAVE_REMOTE_LCD)
    if (state->id3)
    {
#ifdef HAVE_BACKLIGHT
        if (global_settings.caption_backlight)
        {
            /* turn on backlight n seconds before track ends, and turn it off n
               seconds into the new track. n == backlight_timeout, or 5s */
            int n = global_settings.backlight_timeout * 1000;

            if ( n < 1000 )
                n = 5000; /* use 5s if backlight is always on or off */

            if (((state->id3->elapsed < 1000) ||
                 ((state->id3->length - state->id3->elapsed) < (unsigned)n)) &&
                (state->paused == false))
                backlight_on();
        }
#endif
#ifdef HAVE_REMOTE_LCD
        if (global_settings.remote_caption_backlight)
        {
            /* turn on remote backlight n seconds before track ends, and turn it
               off n seconds into the new track. n == remote_backlight_timeout,
               or 5s */
            int n = global_settings.remote_backlight_timeout * 1000;

            if ( n < 1000 )
                n = 5000; /* use 5s if backlight is always on or off */

            if (((state->id3->elapsed < 1000) ||
                 ((state->id3->length - state->id3->elapsed) < (unsigned)n)) &&
                (state->paused == false))
                remote_backlight_on();
        }
#endif
    }
#else
    (void) state;
#endif /* def HAVE_BACKLIGHT || def HAVE_REMOTE_LCD */
}

static void change_dir(int direction)
{
    if (global_settings.prevent_skip)
        return;

    if (direction < 0)
        audio_prev_dir();
    else if (direction > 0)
        audio_next_dir();
    /* prevent the next dir to immediatly start being ffw'd */
    action_wait_for_release();
}

static void prev_track(unsigned long skip_thresh)
{
    struct wps_state *state = get_wps_state();
    if (state->id3->elapsed < skip_thresh)
    {
        audio_prev();
        return;
    }
    else
    {
        if (state->id3->cuesheet)
        {
            curr_cuesheet_skip(state->id3->cuesheet, -1, state->id3->elapsed);
            return;
        }

        audio_pre_ff_rewind();
        audio_ff_rewind(0);
    }
}

static void next_track(void)
{
    struct wps_state *state = get_wps_state();
    /* take care of if we're playing a cuesheet */
    if (state->id3->cuesheet)
    {
        if (curr_cuesheet_skip(state->id3->cuesheet, 1, state->id3->elapsed))
        {
            /* if the result was false, then we really want
               to skip to the next track */
            return;
        }
    }

    audio_next();
}

static void play_hop(int direction)
{
    struct wps_state *state = get_wps_state();
    struct cuesheet *cue = state->id3->cuesheet;
    long step = global_settings.skip_length*1000;
    long elapsed = state->id3->elapsed;
    long remaining = state->id3->length - elapsed;

    /* if cuesheet is active, then we want the current tracks end instead of
     * the total end */
    if (cue && (cue->curr_track_idx+1 < cue->track_count))
    {
        int next = cue->curr_track_idx+1;
        struct cue_track_info *t = &cue->tracks[next];
        remaining = t->offset - elapsed;
    }

    if (step < 0)
    {
        if (direction < 0)
        {
            prev_track(DEFAULT_SKIP_THRESH);
            return;
        }
        else if (remaining < DEFAULT_SKIP_THRESH*2)
        {
            next_track();
            return;
        }
        else
            elapsed += (remaining - DEFAULT_SKIP_THRESH*2);
    }
    else if (!global_settings.prevent_skip &&
           (!step ||
            (direction > 0 && step >= remaining) ||
            (direction < 0 && elapsed < DEFAULT_SKIP_THRESH)))
    {   /* Do normal track skipping */
        if (direction > 0)
            next_track();
        else if (direction < 0)
        {
            if (step > 0 && global_settings.rewind_across_tracks && elapsed < DEFAULT_SKIP_THRESH && playlist_check(-1))
            {
                bool audio_paused = (audio_status() & AUDIO_STATUS_PAUSE)?true:false;
                if (!audio_paused)
                    audio_pause();
                audio_prev();
                audio_ff_rewind(-step);
                if (!audio_paused)
                    audio_resume();
                return;
            }

            prev_track(DEFAULT_SKIP_THRESH);
        }
        return;
    }
    else if (direction == 1 && step >= remaining)
    {
        system_sound_play(SOUND_TRACK_NO_MORE);
        return;
    }
    else if (direction == -1 && elapsed < step)
    {
        elapsed = 0;
    }
    else
    {
        elapsed += step * direction;
    }
    if(audio_status() & AUDIO_STATUS_PLAY)
    {
        audio_pre_ff_rewind();
    }

    audio_ff_rewind(elapsed);
}

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
/*
 * If the user is unable to see the wps, because the display is deactivated,
 * we suppress updates until the wps is activated again (the lcd driver will
 * call this hook to issue an instant update)
 * */
static void wps_lcd_activation_hook(unsigned short id, void *param)
{
    (void)id;
    (void)param;
    skin_request_full_update(WPS);
    /* force timeout in wps main loop, so that the update is instantly */
    button_queue_post(BUTTON_NONE, 0);
}
#endif

static void gwps_leave_wps(bool theme_enabled)
{
    FOR_NB_SCREENS(i)
    {
        struct gui_wps *gwps = skin_get_gwps(WPS, i);
        gwps->display->scroll_stop();
        if (theme_enabled)
        {
#ifdef HAVE_BACKDROP_IMAGE
            skin_backdrop_show(sb_get_backdrop(i));

            /* The following is supposed to erase any traces of %VB
               viewports drawn by the WPS. May need further thought... */
            struct wps_data *sbs = skin_get_gwps(CUSTOM_STATUSBAR, i)->data;
            if (gwps->data->use_extra_framebuffer && sbs->use_extra_framebuffer)
                skin_update(CUSTOM_STATUSBAR, i, SKIN_REFRESH_ALL);
#endif
            viewportmanager_theme_undo(i, skin_has_sbs(gwps));
        }
    }

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    /* Play safe and unregister the hook */
    remove_event(LCD_EVENT_ACTIVATION, wps_lcd_activation_hook);
#endif
    /* unhandle statusbar update delay */
    sb_skin_set_update_delay(DEFAULT_UPDATE_DELAY);
#ifdef HAVE_TOUCHSCREEN
    touchscreen_set_mode(global_settings.touch_mode);
#endif
}

static void restore_theme(void)
{
    FOR_NB_SCREENS(i)
    {
        struct gui_wps *gwps = skin_get_gwps(WPS, i);
        struct screen *display = gwps->display;
        display->scroll_stop();
        viewportmanager_theme_enable(i, skin_has_sbs(gwps), NULL);
    }
}

/*
 * display the wps on entering or restoring */
static void gwps_enter_wps(bool theme_enabled)
{
    struct gui_wps *gwps;
    struct screen *display;
    if (theme_enabled)
        restore_theme();
    FOR_NB_SCREENS(i)
    {
        gwps = skin_get_gwps(WPS, i);
        display = gwps->display;
        display->scroll_stop();
        /* Update the values in the first (default) viewport - in case the user
           has modified the statusbar or colour settings */
#if LCD_DEPTH > 1
        if (display->depth > 1)
        {
            struct skin_viewport *svp = skin_find_item(VP_DEFAULT_LABEL_STRING,
                                                       SKIN_FIND_VP, gwps->data);
            if (svp)
            {
                struct viewport *vp = &svp->vp;
                vp->fg_pattern = display->get_foreground();
                vp->bg_pattern = display->get_background();
            }
        }
#endif
        /* make the backdrop actually take effect */
#ifdef HAVE_BACKDROP_IMAGE
        skin_backdrop_show(gwps->data->backdrop_id);
#endif
        display->clear_display();
        skin_update(WPS, i, SKIN_REFRESH_ALL);

    }
#ifdef HAVE_TOUCHSCREEN
    gwps = skin_get_gwps(WPS, SCREEN_MAIN);
    skin_disarm_touchregions(gwps);
    if (gwps->data->touchregions < 0)
        touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    /* force statusbar/skin update since we just cleared the whole screen */
    send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
}

static long do_wps_exit(long action, bool bookmark)
{
    audio_pause();
    update_non_static();
    if (bookmark)
        bookmark_autobookmark(true);
    audio_stop();

    ab_reset_markers();

    gwps_leave_wps(true);
#ifdef HAVE_RECORDING
    if (action == ACTION_WPS_REC)
        return GO_TO_RECSCREEN;
#else
    (void)action;
#endif
    if (global_settings.browse_current)
        return GO_TO_PREVIOUS_BROWSER;
    return GO_TO_PREVIOUS;
}

static long do_party_mode(long action)
{
    if (global_settings.party_mode)
    {
        switch (action)
        {
#ifdef ACTION_WPSAB_SINGLE
            case ACTION_WPSAB_SINGLE:
                if (!ab_repeat_mode_enabled())
                    break;
                /* Note: currently all targets use ACTION_WPS_BROWSE
                 * if mapped to any of below actions this will cause problems */
#endif
            case ACTION_WPS_PLAY:
            case ACTION_WPS_SEEKFWD:
            case ACTION_WPS_SEEKBACK:
            case ACTION_WPS_SKIPPREV:
            case ACTION_WPS_SKIPNEXT:
            case ACTION_WPS_ABSETB_NEXTDIR:
            case ACTION_WPS_ABSETA_PREVDIR:
            case ACTION_WPS_STOP:
                return ACTION_NONE;
                break;
            default:
                break;
        }
    }
    return action;
}

static inline int action_wpsab_single(long button)
{
/* The iPods/X5/M5 use a single button for the A-B mode markers,
   defined as ACTION_WPSAB_SINGLE in their config files. */
#ifdef ACTION_WPSAB_SINGLE
        static int wps_ab_state = 0;
        if (button == ACTION_WPSAB_SINGLE && ab_repeat_mode_enabled())
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
#endif /* def ACTION_WPSAB_SINGLE */
    return button;
}

/* The WPS can be left in two ways:
 *      a)  call a function, which draws over the wps. In this case, the wps
 *          will be still active (i.e. the below function didn't return)
 *      b)  return with a value evaluated by root_menu.c, in this case the wps
 *          is really left, and root_menu will handle the next screen
 *
 * In either way, call gwps_leave_wps(true), in order to restore the correct
 * "main screen" backdrops and statusbars
 */
long gui_wps_show(void)
{
    long button = 0;
    bool restore = true;
    bool exit = false;
    bool bookmark = false;
    bool update = false;
    bool theme_enabled = true;
    long last_left = 0, last_right = 0;
    struct wps_state *state = get_wps_state();

    ab_reset_markers();

    wps_state_init();
    while ( 1 )
    {
        bool hotkey = false;
        bool audio_paused = (audio_status() & AUDIO_STATUS_PAUSE)?true:false;
        /* did someone else (i.e power thread) change audio pause mode? */
        if (state->paused != audio_paused) {
            state->paused = audio_paused;

            /* if another thread paused audio, we are probably in car mode,
               about to shut down. lets save the settings. */
            if (state->paused) {
                settings_save();
#if !defined(HAVE_SW_POWEROFF)
                call_storage_idle_notifys(true);
#endif
            }
        }

        if (restore)
        {
            restore = false;
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
            add_event(LCD_EVENT_ACTIVATION, wps_lcd_activation_hook);
#endif
        /* we remove the update delay since it's not very usable in the wps,
         * e.g. during volume changing or ffwd/rewind */
            sb_skin_set_update_delay(0);
            skin_request_full_update(WPS);
            update = true;
            gwps_enter_wps(theme_enabled);
            theme_enabled = true;
        }
        else
        {
            gwps_caption_backlight(state);

            FOR_NB_SCREENS(i)
            {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
                /* currently, all remotes are readable without backlight
                 * so still update those */
                if (lcd_active() || (i != SCREEN_MAIN))
#endif
                {
                    bool full_update = skin_do_full_update(WPS, i);
                    if (update || full_update)
                    {
                        skin_update(WPS, i, full_update ?
                                     SKIN_REFRESH_ALL : SKIN_REFRESH_NON_STATIC);
                    }
                }
            }
            update = false;
        }

        if (exit)
        {
            return do_wps_exit(button, bookmark);
        }

        if (button && !IS_SYSEVENT(button) )
            storage_spin();

        button = skin_wait_for_action(WPS, CONTEXT_WPS|ALLOW_SOFTLOCK, HZ/5);

        /* Exit if audio has stopped playing. This happens e.g. at end of
           playlist or if using the sleep timer. */
        if (!(audio_status() & AUDIO_STATUS_PLAY))
            exit = true;
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
            button = skintouch_to_wps();
#endif
        button = do_party_mode(button); /* block select actions in party mode */

        button = action_wpsab_single(button); /* iPods/X5/M5 */

        switch(button)
        {
#ifdef HAVE_HOTKEY
            case ACTION_WPS_HOTKEY:
            {
                hotkey = true;
                if (!global_settings.hotkey_wps)
                    break;
                if (get_hotkey(global_settings.hotkey_wps)->flags & HOTKEY_FLAG_NOSBS)
                {
                    /* leave WPS without re-enabling theme */
                    theme_enabled = false;
                    gwps_leave_wps(theme_enabled);
                    onplay(state->id3->path,
                           FILE_ATTR_AUDIO, CONTEXT_WPS, hotkey, ONPLAY_NO_CUSTOMACTION);
                    if (!audio_status())
                    {
                        /* re-enable theme since we're returning to SBS */
                        gwps_leave_wps(true);
                        return GO_TO_ROOT;
                    }
                    restore = true;
                    break;
                }
            }
            /* fall through */
#endif /* def HAVE_HOTKEY */
            case ACTION_WPS_CONTEXT:
            {
                gwps_leave_wps(true);
                int retval = onplay(state->id3->path,
                       FILE_ATTR_AUDIO, CONTEXT_WPS, hotkey, ONPLAY_NO_CUSTOMACTION);
                /* if music is stopped in the context menu we want to exit the wps */
                if (retval == ONPLAY_MAINMENU
                    || !audio_status())
                    return GO_TO_ROOT;
                else if (retval == ONPLAY_PLAYLIST)
                    return GO_TO_PLAYLIST_VIEWER;
                else if (retval == ONPLAY_PLUGIN)
                {
                    restore_theme();
                    theme_enabled = false;
                    open_plugin_run(ID2P(LANG_OPEN_PLUGIN_SET_WPS_CONTEXT_PLUGIN));
                }

                restore = true;
            }
            break;

            case ACTION_WPS_BROWSE:
                gwps_leave_wps(true);
                return GO_TO_PREVIOUS_BROWSER;
                break;

                /* play/pause */
            case ACTION_WPS_PLAY:
                wps_do_action(WPS_PLAYPAUSE, true);
                break;

            case ACTION_WPS_VOLUP: /* fall through */
            case ACTION_WPS_VOLDOWN:
                if (button == ACTION_WPS_VOLUP)
                    adjust_volume(1);
                else
                    adjust_volume(-1);

                setvol();
                FOR_NB_SCREENS(i)
                {
                    skin_update(WPS, i, SKIN_REFRESH_NON_STATIC);
                }
                update = false;
                break;
            /* fast forward
                OR next dir if this is straight after ACTION_WPS_SKIPNEXT */
            case ACTION_WPS_SEEKFWD:
                if (current_tick -last_right < HZ)
                {
                    if (state->id3->cuesheet && playlist_check(1))
                    {
                        audio_next();
                    }
                    else
                    {
                        change_dir(1);
                    }
                }
                else
                    ffwd_rew(ACTION_WPS_SEEKFWD, false);
                last_right = last_left = 0;
                break;
            /* fast rewind
                OR prev dir if this is straight after ACTION_WPS_SKIPPREV,*/
            case ACTION_WPS_SEEKBACK:
                if (current_tick - last_left < HZ)
                {
                    if (state->id3->cuesheet && playlist_check(-1))
                    {
                        audio_prev();
                    }
                    else
                    {
                        change_dir(-1);
                    }
                } else if (global_settings.rewind_across_tracks
                           && get_wps_state()->id3->elapsed < DEFAULT_SKIP_THRESH
                           && playlist_check(-1))
                {
                    if (!audio_paused)
                        audio_pause();
                    audio_prev();
                    ffwd_rew(ACTION_WPS_SEEKBACK, true);
                    if (!audio_paused)
                        audio_resume();
                }
                else
                    ffwd_rew(ACTION_WPS_SEEKBACK, false);
                last_left = last_right = 0;
                break;

                /* prev / restart */
            case ACTION_WPS_SKIPPREV:
                last_left = current_tick;

                /* if we're in A/B repeat mode and the current position
                   is past the A marker, jump back to the A marker... */
                if ( ab_repeat_mode_enabled() && ab_after_A_marker(state->id3->elapsed) )
                {
                    ab_jump_to_A_marker();
                    break;
                }
                else /* ...otherwise, do it normally */
                    play_hop(-1);
                break;

                /* next
                   OR if skip length set, hop by predetermined amount. */
            case ACTION_WPS_SKIPNEXT:
                last_right = current_tick;

                /* if we're in A/B repeat mode and the current position is
                   before the A marker, jump to the A marker... */
                if ( ab_repeat_mode_enabled() )
                {
                    if ( ab_before_A_marker(state->id3->elapsed) )
                    {
                        ab_jump_to_A_marker();
                        break;
                    }
                }
                else /* ...otherwise, do it normally */
                    play_hop(1);
                break;
                /* next / prev directories */
                /* and set A-B markers if in a-b mode */
            case ACTION_WPS_ABSETB_NEXTDIR:
                if (ab_repeat_mode_enabled())
                {
                    ab_set_B_marker(state->id3->elapsed);
                    ab_jump_to_A_marker();
                }
                else
                {
                    change_dir(1);
                }
                break;
            case ACTION_WPS_ABSETA_PREVDIR:
                if (ab_repeat_mode_enabled())
                    ab_set_A_marker(state->id3->elapsed);
                else
                {
                    change_dir(-1);
                }
                break;
            /* menu key functions */
            case ACTION_WPS_MENU:
                gwps_leave_wps(true);
                return GO_TO_ROOT;
                break;


#ifdef HAVE_QUICKSCREEN
            case ACTION_WPS_QUICKSCREEN:
            {
                gwps_leave_wps(true);
                bool enter_shortcuts_menu = global_settings.shortcuts_replaces_qs;
                if (!enter_shortcuts_menu)
                {
                    int ret = quick_screen_quick(button);
                    if (ret == QUICKSCREEN_IN_USB)
                        return GO_TO_ROOT;
                    else if (ret == QUICKSCREEN_GOTO_SHORTCUTS_MENU)
                        enter_shortcuts_menu = true;
                    else
                        restore = true;
                }

                if (enter_shortcuts_menu) /* enter_shortcuts_menu */
                {
                    global_status.last_screen = GO_TO_SHORTCUTMENU;
                    int ret = do_shortcut_menu(NULL);
                    return (ret == GO_TO_PREVIOUS ? GO_TO_WPS : ret);
                }
            }
            break;
#endif /* HAVE_QUICKSCREEN */

                /* screen settings */

                /* pitch screen */
#ifdef HAVE_PITCHCONTROL
            case ACTION_WPS_PITCHSCREEN:
            {
                gwps_leave_wps(true);
                if (1 == gui_syncpitchscreen_run())
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* HAVE_PITCHCONTROL */

            /* reset A&B markers */
            case ACTION_WPS_ABRESET:
                if (ab_repeat_mode_enabled())
                {
                    ab_reset_markers();
                    update = true;
                }
                break;

                /* stop and exit wps */
            case ACTION_WPS_STOP:
                bookmark = true;
                exit = true;
                break;

            case ACTION_WPS_LIST_BOOKMARKS:
                gwps_leave_wps(true);
                if (bookmark_load_menu() == BOOKMARK_USB_CONNECTED)
                {
                    return GO_TO_ROOT;
                }
                restore = true;
                break;

            case ACTION_WPS_CREATE_BOOKMARK:
                gwps_leave_wps(true);
                bookmark_create_menu();
                restore = true;
                break;

            case ACTION_WPS_ID3SCREEN:
            {
                gwps_leave_wps(true);
                if (browse_id3(audio_current_track(),
                        playlist_get_display_index(),
                        playlist_amount(), NULL, 1, NULL))
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
             /* this case is used by the softlock feature
              * it requests a full update here */
            case ACTION_REDRAW:
                skin_request_full_update(WPS);
                skin_request_full_update(CUSTOM_STATUSBAR); /* if SBS is used */
                break;
            case ACTION_NONE: /* Timeout, do a partial update */
                update = true;
                ffwd_rew(button, false); /* hopefully fix the ffw/rwd bug */
                break;
#ifdef HAVE_RECORDING
            case ACTION_WPS_REC:
                exit = true;
                break;
#endif
            case ACTION_WPS_VIEW_PLAYLIST:
                gwps_leave_wps(true);
                return GO_TO_PLAYLIST_VIEWER;
                break;
            default:
                switch(default_event_handler(button))
                {   /* music has been stopped by the default handler */
                    case SYS_USB_CONNECTED:
                    case SYS_CALL_INCOMING:
                    case BUTTON_MULTIMEDIA_STOP:
                        gwps_leave_wps(true);
                        return GO_TO_ROOT;
                }
                update = true;
                break;
        }
    }
    return GO_TO_ROOT; /* unreachable - just to reduce compiler warnings */
}

struct wps_state *get_wps_state(void)
{
    return &wps_state;
}

/* this is called from the playback thread so NO DRAWING! */
static void track_info_callback(unsigned short id, void *param)
{
    struct wps_state *state = get_wps_state();

    if (id == PLAYBACK_EVENT_TRACK_CHANGE || id == PLAYBACK_EVENT_CUR_TRACK_READY)
    {
        state->id3 = ((struct track_event *)param)->id3;
        if (state->id3->cuesheet)
        {
            cue_find_current_track(state->id3->cuesheet, state->id3->elapsed);
        }
    }
#ifdef AUDIO_FAST_SKIP_PREVIEW
    else if (id == PLAYBACK_EVENT_TRACK_SKIP)
    {
        state->id3 = audio_current_track();
    }
#endif
    if (id == PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE)
        state->nid3 = audio_next_track();
    skin_request_full_update(WPS);
}

static void wps_state_init(void)
{
    struct wps_state *state = get_wps_state();
    state->paused = false;
    if(audio_status() & AUDIO_STATUS_PLAY)
    {
        state->id3 = audio_current_track();
        state->nid3 = audio_next_track();
    }
    else
    {
        state->id3 = NULL;
        state->nid3 = NULL;
    }
    /* We'll be updating due to restore initialized with true */
    skin_request_full_update(WPS);
    /* add the WPS track event callbacks */
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, track_info_callback);
    add_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, track_info_callback);
    /* Use the same callback as ..._TRACK_CHANGE for when remaining handles have
       finished */
    add_event(PLAYBACK_EVENT_CUR_TRACK_READY, track_info_callback);
#ifdef AUDIO_FAST_SKIP_PREVIEW
    add_event(PLAYBACK_EVENT_TRACK_SKIP, track_info_callback);
#endif
}

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
#include "pitchscreen.h"
#include "appevents.h"
#include "viewport.h"
#include "pcmbuf.h"
#include "option_select.h"
#include "playlist_viewer.h"
#include "wps.h"
#include "statusbar-skinned.h"

#define RESTORE_WPS_INSTANTLY       0l
#define RESTORE_WPS_NEXT_SECOND     ((long)(HZ+current_tick))

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

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

void pause_action(bool may_fade, bool updatewps)
{
    /* Do audio first, then update, unless skin were to use its local
       status in which case, reverse it */
    audio_pause();

    if (updatewps)
        update_non_static();

    if (global_settings.pause_rewind) {
        long newpos;

        audio_pre_ff_rewind();
        newpos = audio_current_track()->elapsed
            - global_settings.pause_rewind * 1000;
        audio_ff_rewind(newpos > 0 ? newpos : 0);
    }

    (void)may_fade;
}

void unpause_action(bool may_fade, bool updatewps)
{
    /* Do audio first, then update, unless skin were to use its local
       status in which case, reverse it */
    audio_resume();

    if (updatewps)
        update_non_static();

    (void)may_fade;
}

static bool update_onvol_change(enum screen_type screen)
{
    skin_update(WPS, screen, SKIN_REFRESH_NON_STATIC);

    return false;
}


#ifdef HAVE_TOUCHSCREEN
static int skintouch_to_wps(struct wps_data *data)
{
    int offset = 0;
    struct touchregion *region;
    int button = skin_get_touchaction(data, &offset, &region);
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
            skin_get_global_state()->id3->elapsed = skin_get_global_state()->id3->length*offset/1000;
            audio_pre_ff_rewind();
            audio_ff_rewind(skin_get_global_state()->id3->elapsed);
            return ACTION_TOUCHSCREEN;
        case ACTION_TOUCH_VOLUME:
        {
            const int min_vol = sound_min(SOUND_VOLUME);
            const int max_vol = sound_max(SOUND_VOLUME);
            const int step_vol = sound_steps(SOUND_VOLUME);
            global_settings.volume = (offset * (max_vol - min_vol)) / 1000;
            global_settings.volume += min_vol;
            global_settings.volume -= (global_settings.volume % step_vol);
            setvol();
        }
        return ACTION_TOUCHSCREEN;
    }
    return button;
}
#endif /* HAVE_TOUCHSCREEN */

bool ffwd_rew(int button)
{
    unsigned int step = 0;     /* current ff/rewind step */
    unsigned int max_step = 0; /* maximum ff/rewind step */
    int ff_rewind_count = 0;   /* current ff/rewind count (in ticks) */
    int direction = -1;         /* forward=1 or backward=-1 */
    bool exit = false;
    bool usb = false;
    const long ff_rw_accel = (global_settings.ff_rewind_accel + 3);

    if (button == ACTION_NONE)
    {
        status_set_ffmode(0);
        return usb;
    }
    while (!exit)
    {
        switch ( button )
        {
            case ACTION_WPS_SEEKFWD:
                 direction = 1;
                 /* Fallthrough */
            case ACTION_WPS_SEEKBACK:
                if (skin_get_global_state()->ff_rewind)
                {
                    if (direction == 1)
                    {
                        /* fast forwarding, calc max step relative to end */
                        max_step = (skin_get_global_state()->id3->length -
                                    (skin_get_global_state()->id3->elapsed +
                                     ff_rewind_count)) *
                                     FF_REWIND_MAX_PERCENT / 100;
                    }
                    else
                    {
                        /* rewinding, calc max step relative to start */
                        max_step = (skin_get_global_state()->id3->elapsed + ff_rewind_count) *
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
                    if ( (audio_status() & AUDIO_STATUS_PLAY) &&
                          skin_get_global_state()->id3 && skin_get_global_state()->id3->length )
                    {
                        audio_pre_ff_rewind();
                        if (direction > 0)
                            status_set_ffmode(STATUS_FASTFORWARD);
                        else
                            status_set_ffmode(STATUS_FASTBACKWARD);

                        skin_get_global_state()->ff_rewind = true;

                        step = 1000 * global_settings.ff_rewind_min_step;
                    }
                    else
                        break;
                }

                if (direction > 0) {
                    if ((skin_get_global_state()->id3->elapsed + ff_rewind_count) >
                        skin_get_global_state()->id3->length)
                        ff_rewind_count = skin_get_global_state()->id3->length -
                            skin_get_global_state()->id3->elapsed;
                }
                else {
                    if ((int)(skin_get_global_state()->id3->elapsed + ff_rewind_count) < 0)
                        ff_rewind_count = -skin_get_global_state()->id3->elapsed;
                }

                /* set the wps state ff_rewind_count so the progess info
                   displays corectly */
                skin_get_global_state()->ff_rewind_count = ff_rewind_count;

                FOR_NB_SCREENS(i)
                {
                    skin_update(WPS, i,
                                SKIN_REFRESH_PLAYER_PROGRESS |
                                SKIN_REFRESH_DYNAMIC);
                }

                break;

            case ACTION_WPS_STOPSEEK:
                skin_get_global_state()->id3->elapsed = skin_get_global_state()->id3->elapsed+ff_rewind_count;
                audio_ff_rewind(skin_get_global_state()->id3->elapsed);
                skin_get_global_state()->ff_rewind_count = 0;
                skin_get_global_state()->ff_rewind = false;
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
                button = skintouch_to_wps(skin_get_gwps(WPS, SCREEN_MAIN)->data);
#endif
            if (button != ACTION_WPS_SEEKFWD &&
                button != ACTION_WPS_SEEKBACK)
                button = ACTION_WPS_STOPSEEK;
        }
    }
    return usb;
}

#if defined(HAVE_BACKLIGHT) || defined(HAVE_REMOTE_LCD)
static void gwps_caption_backlight(struct wps_state *state)
{
    if (state && state->id3)
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
}
#endif


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
    struct wps_state *state = skin_get_global_state();
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
    struct wps_state *state = skin_get_global_state();
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
    struct wps_state *state = skin_get_global_state();
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
            prev_track(DEFAULT_SKIP_TRESH);
            return;
        }
        else if (remaining < DEFAULT_SKIP_TRESH*2)
        {
            next_track();
            return;
        }
        else
            elapsed += (remaining - DEFAULT_SKIP_TRESH*2);
    }
    else if (!global_settings.prevent_skip &&
           (!step ||
            (direction > 0 && step >= remaining) ||
            (direction < 0 && elapsed < DEFAULT_SKIP_TRESH)))
    {   /* Do normal track skipping */
        if (direction > 0)
            next_track();
        else if (direction < 0)
            prev_track(DEFAULT_SKIP_TRESH);
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
    queue_post(&button_queue, BUTTON_NONE, 0);
}
#endif

static void gwps_leave_wps(void)
{
    FOR_NB_SCREENS(i)
    {
        skin_get_gwps(WPS, i)->display->scroll_stop();
#ifdef HAVE_BACKDROP_IMAGE
        skin_backdrop_show(sb_get_backdrop(i));
#endif
        viewportmanager_theme_undo(i, skin_has_sbs(i, skin_get_gwps(WPS, i)->data));

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

/*
 * display the wps on entering or restoring */
static void gwps_enter_wps(void)
{
    struct gui_wps *gwps;
    struct screen *display;
    FOR_NB_SCREENS(i)
    {
        gwps = skin_get_gwps(WPS, i);
        display = gwps->display;
        display->scroll_stop();
        viewportmanager_theme_enable(i, skin_has_sbs(i, skin_get_gwps(WPS, i)->data), NULL);

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
    skin_disarm_touchregions(gwps->data);
    if (gwps->data->touchregions < 0)
        touchscreen_set_mode(TOUCHSCREEN_BUTTON);
#endif
    /* force statusbar/skin update since we just cleared the whole screen */
    send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
}

void wps_do_playpause(bool updatewps)
{
    struct wps_state *state = skin_get_global_state();
    if ( state->paused )
    {
        state->paused = false;
        unpause_action(true, updatewps);
    }
    else
    {
        state->paused = true;
        pause_action(true, updatewps);
        settings_save();
#if !defined(HAVE_SW_POWEROFF)
        call_storage_idle_notifys(true);   /* make sure resume info is saved */
#endif
    }
}


/* The WPS can be left in two ways:
 *      a)  call a function, which draws over the wps. In this case, the wps
 *          will be still active (i.e. the below function didn't return)
 *      b)  return with a value evaluated by root_menu.c, in this case the wps
 *          is really left, and root_menu will handle the next screen
 *
 * In either way, call gwps_leave_wps(), in order to restore the correct
 * "main screen" backdrops and statusbars
 */
long gui_wps_show(void)
{
    long button = 0;
    bool restore = true;
    long restoretimer = RESTORE_WPS_INSTANTLY; /* timer to delay screen redraw temporarily */
    bool exit = false;
    bool bookmark = false;
    bool update = false;
    bool vol_changed = false;
    long last_left = 0, last_right = 0;
    struct wps_state *state = skin_get_global_state();

#ifdef AB_REPEAT_ENABLE
    ab_repeat_init();
    ab_reset_markers();
#endif
    wps_state_init();

    while ( 1 )
    {
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
        button = skin_wait_for_action(WPS, CONTEXT_WPS|ALLOW_SOFTLOCK,
                                      restore ? 1 : HZ/5);

        /* Exit if audio has stopped playing. This happens e.g. at end of
           playlist or if using the sleep timer. */
        if (!(audio_status() & AUDIO_STATUS_PLAY))
            exit = true;
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
            button = skintouch_to_wps(skin_get_gwps(WPS, SCREEN_MAIN)->data);
#endif
/* The iPods/X5/M5 use a single button for the A-B mode markers,
   defined as ACTION_WPSAB_SINGLE in their config files. */
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
#ifdef HAVE_HOTKEY
            case ACTION_WPS_HOTKEY:
                if (!global_settings.hotkey_wps)
                    break;
                /* fall through */
#endif
            case ACTION_WPS_CONTEXT:
            {
                bool hotkey = button == ACTION_WPS_HOTKEY;
                gwps_leave_wps();
                int retval = onplay(state->id3->path,
                           FILE_ATTR_AUDIO, CONTEXT_WPS, hotkey);
                /* if music is stopped in the context menu we want to exit the wps */
                if (retval == ONPLAY_MAINMENU
                    || !audio_status())
                    return GO_TO_ROOT;
                else if (retval == ONPLAY_PLAYLIST)
                    return GO_TO_PLAYLIST_VIEWER;
                else if (retval == ONPLAY_PLUGIN)
                    return GO_TO_PLUGIN;
                restore = true;
            }
            break;

            case ACTION_WPS_BROWSE:
                gwps_leave_wps();
                return GO_TO_PREVIOUS_BROWSER;
                break;

                /* play/pause */
            case ACTION_WPS_PLAY:
                if (global_settings.party_mode)
                    break;
                wps_do_playpause(true);
                break;

            case ACTION_WPS_VOLUP:
                global_settings.volume += sound_steps(SOUND_VOLUME);
                vol_changed = true;
                break;
            case ACTION_WPS_VOLDOWN:
                global_settings.volume -= sound_steps(SOUND_VOLUME);
                vol_changed = true;
                break;
            /* fast forward
                OR next dir if this is straight after ACTION_WPS_SKIPNEXT */
            case ACTION_WPS_SEEKFWD:
                if (global_settings.party_mode)
                    break;
                if (current_tick -last_right < HZ)
                {
                    if (state->id3->cuesheet)
                    {
                        audio_next();
                    }
                    else
                    {
                        change_dir(1);
                    }
                }
                else
                    ffwd_rew(ACTION_WPS_SEEKFWD);
                last_right = last_left = 0;
                break;
            /* fast rewind
                OR prev dir if this is straight after ACTION_WPS_SKIPPREV,*/
            case ACTION_WPS_SEEKBACK:
                if (global_settings.party_mode)
                    break;
                if (current_tick -last_left < HZ)
                {
                    if (state->id3->cuesheet)
                    {
                        audio_pre_ff_rewind();
                        audio_ff_rewind(0);
                    }
                    else
                    {
                        change_dir(-1);
                    }
                }
                else
                    ffwd_rew(ACTION_WPS_SEEKBACK);
                last_left = last_right = 0;
                break;

                /* prev / restart */
            case ACTION_WPS_SKIPPREV:
                if (global_settings.party_mode)
                    break;
                last_left = current_tick;
#ifdef AB_REPEAT_ENABLE
                /* if we're in A/B repeat mode and the current position
                   is past the A marker, jump back to the A marker... */
                if ( ab_repeat_mode_enabled() && ab_after_A_marker(state->id3->elapsed) )
                {
                    ab_jump_to_A_marker();
                    break;
                }
                else
                /* ...otherwise, do it normally */
#endif
                    play_hop(-1);
                break;

                /* next
                   OR if skip length set, hop by predetermined amount. */
            case ACTION_WPS_SKIPNEXT:
                if (global_settings.party_mode)
                    break;
                last_right = current_tick;
#ifdef AB_REPEAT_ENABLE
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
                else
                /* ...otherwise, do it normally */
#endif
                    play_hop(1);
                break;
                /* next / prev directories */
                /* and set A-B markers if in a-b mode */
            case ACTION_WPS_ABSETB_NEXTDIR:
                if (global_settings.party_mode)
                    break;
#if defined(AB_REPEAT_ENABLE)
                if (ab_repeat_mode_enabled())
                {
                    ab_set_B_marker(state->id3->elapsed);
                    ab_jump_to_A_marker();
                }
                else
#endif
                {
                    change_dir(1);
                }
                break;
            case ACTION_WPS_ABSETA_PREVDIR:
                if (global_settings.party_mode)
                    break;
#if defined(AB_REPEAT_ENABLE)
                if (ab_repeat_mode_enabled())
                    ab_set_A_marker(state->id3->elapsed);
                else
#endif
                {
                    change_dir(-1);
                }
                break;
            /* menu key functions */
            case ACTION_WPS_MENU:
                gwps_leave_wps();
                return GO_TO_ROOT;
                break;


#ifdef HAVE_QUICKSCREEN
            case ACTION_WPS_QUICKSCREEN:
            {
                gwps_leave_wps();
                if (global_settings.shortcuts_replaces_qs)
                {
                    global_status.last_screen = GO_TO_SHORTCUTMENU;
                    int ret = quick_screen_quick(button);
                    return (ret == GO_TO_PREVIOUS ? GO_TO_WPS : ret);
                }
                else if (quick_screen_quick(button) > 0)
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* HAVE_QUICKSCREEN */

                /* screen settings */

                /* pitch screen */
#ifdef HAVE_PITCHCONTROL
            case ACTION_WPS_PITCHSCREEN:
            {
                gwps_leave_wps();
                if (1 == gui_syncpitchscreen_run())
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* HAVE_PITCHCONTROL */

#ifdef AB_REPEAT_ENABLE
            /* reset A&B markers */
            case ACTION_WPS_ABRESET:
                if (ab_repeat_mode_enabled())
                {
                    ab_reset_markers();
                    update = true;
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

            case ACTION_WPS_LIST_BOOKMARKS:
                gwps_leave_wps();
                if (bookmark_load_menu() == BOOKMARK_USB_CONNECTED)
                {
                    return GO_TO_ROOT;
                }
                restore = true;
                break;

            case ACTION_WPS_CREATE_BOOKMARK:
                gwps_leave_wps();
                bookmark_create_menu();
                restore = true;
                break;

            case ACTION_WPS_ID3SCREEN:
            {
                gwps_leave_wps();
                if (browse_id3(audio_current_track(),
                        playlist_get_display_index(),
                        playlist_amount()))
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
             /* this case is used by the softlock feature
              * it requests a full update here */
            case ACTION_REDRAW:
                skin_request_full_update(WPS);
                break;
            case ACTION_NONE: /* Timeout, do a partial update */
                update = true;
                ffwd_rew(button); /* hopefully fix the ffw/rwd bug */
                break;
#ifdef HAVE_RECORDING
            case ACTION_WPS_REC:
                exit = true;
                break;
#endif
            case ACTION_WPS_VIEW_PLAYLIST:
                gwps_leave_wps();
                return GO_TO_PLAYLIST_VIEWER;
                break;
            default:
                switch(default_event_handler(button))
                {   /* music has been stopped by the default handler */
                    case SYS_USB_CONNECTED:
                    case SYS_CALL_INCOMING:
                    case BUTTON_MULTIMEDIA_STOP:
                        gwps_leave_wps();
                        return GO_TO_ROOT;
                }
                update = true;
                break;
        }

        if (vol_changed)
        {
            bool res = false;
            vol_changed = false;
            setvol();
            FOR_NB_SCREENS(i)
            {
                if(update_onvol_change(i))
                    res = true;
            }
            if (res) {
                restore = true;
                restoretimer = RESTORE_WPS_NEXT_SECOND;
            }
        }


        if (restore &&
            ((restoretimer == RESTORE_WPS_INSTANTLY) ||
             TIME_AFTER(current_tick, restoretimer)))
        {
            restore = false;
            restoretimer = RESTORE_WPS_INSTANTLY;
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
            add_event(LCD_EVENT_ACTIVATION, wps_lcd_activation_hook);
#endif
        /* we remove the update delay since it's not very usable in the wps,
         * e.g. during volume changing or ffwd/rewind */
            sb_skin_set_update_delay(0);
            skin_request_full_update(WPS);
            update = true;
            gwps_enter_wps();
        }
        else
        {
#if defined(HAVE_BACKLIGHT) || defined(HAVE_REMOTE_LCD)
            gwps_caption_backlight(state);
#endif
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

        if (exit) {
            audio_pause();
            update_non_static();
            if (bookmark)
                bookmark_autobookmark(true);
            audio_stop();
#ifdef AB_REPEAT_ENABLE
            ab_reset_markers();
#endif
            gwps_leave_wps();
#ifdef HAVE_RECORDING
            if (button == ACTION_WPS_REC)
                return GO_TO_RECSCREEN;
#endif
            if (global_settings.browse_current)
                return GO_TO_PREVIOUS_BROWSER;
            return GO_TO_PREVIOUS;
        }

        if (button && !IS_SYSEVENT(button) )
            storage_spin();
    }
    return GO_TO_ROOT; /* unreachable - just to reduce compiler warnings */
}

/* this is called from the playback thread so NO DRAWING! */
static void track_info_callback(unsigned short id, void *param)
{
    struct wps_state *state = skin_get_global_state();

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
    skin_get_global_state()->nid3 = audio_next_track();
    skin_request_full_update(WPS);
}

static void wps_state_init(void)
{
    struct wps_state *state = skin_get_global_state();
    state->ff_rewind = false;
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


#ifdef IPOD_ACCESSORY_PROTOCOL
bool is_wps_fading(void)
{
    return skin_get_global_state()->is_fading;
}

int wps_get_ff_rewind_count(void)
{
    return skin_get_global_state()->ff_rewind_count;
}
#endif

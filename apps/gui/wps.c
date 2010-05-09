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
#include "mp3_playback.h"
#include "audio.h"
#include "usb.h"
#include "status.h"
#include "storage.h"
#include "screens.h"
#include "playlist.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "peakmeter.h"
#endif
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
#include "dsp.h"
#include "playlist_viewer.h"
#include "wps.h"
#include "statusbar-skinned.h"

#define RESTORE_WPS_INSTANTLY       0l
#define RESTORE_WPS_NEXT_SECOND     ((long)(HZ+current_tick))

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

/* currently only one wps_state is needed, initialize to 0 */
       struct wps_state     wps_state               = { .id3 = NULL };
static struct gui_wps       gui_wps[NB_SCREENS]     = {{ .data = NULL }};
static struct wps_data      wps_datas[NB_SCREENS]   = {{ .wps_loaded = 0 }};
static struct wps_sync_data wps_sync_data           = { .do_full_update = false };

/* initial setup of wps_data  */
static void wps_state_init(void);
static void track_changed_callback(void *param);
static void nextid3available_callback(void* param);

#ifdef HAVE_TOUCHSCREEN
static void wps_disarm_touchregions(struct wps_data *data);
#endif

#define WPS_DEFAULTCFG WPS_DIR "/rockbox_default.wps"
#ifdef HAVE_REMOTE_LCD
#define RWPS_DEFAULTCFG WPS_DIR "/rockbox_default.rwps"
#define DEFAULT_WPS(screen) ((screen) == SCREEN_MAIN ? \
                            WPS_DEFAULTCFG:RWPS_DEFAULTCFG)
#else
#define DEFAULT_WPS(screen) (WPS_DEFAULTCFG)
#endif

void wps_data_load(enum screen_type screen, const char *buf, bool isfile)
{
    bool loaded_ok;

#ifndef __PCTOOL__
    /*
     * Hardcode loading WPS_DEFAULTCFG to cause a reset ideally this
     * wants to be a virtual file.  Feel free to modify dirbrowse()
     * if you're feeling brave.
     */

    if (buf && ! strcmp(buf, DEFAULT_WPS(screen)) )
    {
#ifdef HAVE_REMOTE_LCD
        if (screen == SCREEN_REMOTE)
            global_settings.rwps_file[0] = '\0';
        else
#endif
            global_settings.wps_file[0] = '\0';
        buf = NULL;
    }

#endif /* __PCTOOL__ */

    loaded_ok = buf && skin_data_load(screen, gui_wps[screen].data, buf, isfile);

    if (!loaded_ok) /* load the hardcoded default */
    {
        char *skin_buf[NB_SCREENS] = {
#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
            "%Xd\n"
#endif
            "%s%?it<%?in<%in. |>%it|%fn>\n"
            "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
            "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n\n"
            "%al%pc/%pt%ar[%pp:%pe]\n"
            "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
            "%pb\n%pm\n",
#else
            "%s%pp/%pe: %?it<%it|%fn> - %?ia<%ia|%d2> - %?id<%id|%d1>\n"
            "%pc%?ps<*|/>%pt\n",
#endif
#ifdef HAVE_REMOTE_LCD
#if LCD_REMOTE_DEPTH > 1
            "%Xd\n"
#endif
            "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
            "%s%?it<%?in<%in. |>%it|%fn>\n"
            "%al%pc/%pt%ar[%pp:%pe]\n"
            "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
            "%pb\n",
#endif
        };
        skin_data_load(screen, gui_wps[screen].data, skin_buf[screen], false);
    }
}

void fade(bool fade_in, bool updatewps)
{
    int fp_global_vol = global_settings.volume << 8;
    int fp_min_vol = sound_min(SOUND_VOLUME) << 8;
    int fp_step = (fp_global_vol - fp_min_vol) / 30;
    int i;
    wps_state.is_fading = !fade_in;
    if (fade_in) {
        /* fade in */
        int fp_volume = fp_min_vol;

        /* zero out the sound */
        sound_set_volume(fp_min_vol >> 8);

        sleep(HZ/10); /* let audio thread run */
        audio_resume();
        
        while (fp_volume < fp_global_vol - fp_step) {
            fp_volume += fp_step;
            sound_set_volume(fp_volume >> 8);
            if (updatewps)
            {
                FOR_NB_SCREENS(i)
                    skin_update(&gui_wps[i], WPS_REFRESH_NON_STATIC);
            }
            sleep(1);
        }
        sound_set_volume(global_settings.volume);
    }
    else {
        /* fade out */
        int fp_volume = fp_global_vol;

        while (fp_volume > fp_min_vol + fp_step) {
            fp_volume -= fp_step;
            sound_set_volume(fp_volume >> 8);
            if (updatewps)
            {
                FOR_NB_SCREENS(i)
                    skin_update(&gui_wps[i], WPS_REFRESH_NON_STATIC);
            }
            sleep(1);
        }
        audio_pause();
        wps_state.is_fading = false;
#if CONFIG_CODEC != SWCODEC
#ifndef SIMULATOR
        /* let audio thread run and wait for the mas to run out of data */
        while (!mp3_pause_done())
#endif
            sleep(HZ/10);
#endif

        /* reset volume to what it was before the fade */
        sound_set_volume(global_settings.volume);
    }
}

static bool update_onvol_change(struct gui_wps * gwps)
{
    skin_update(gwps, WPS_REFRESH_NON_STATIC);

#ifdef HAVE_LCD_CHARCELLS
    splashf(0, "Vol: %3d dB",
               sound_val2phys(SOUND_VOLUME, global_settings.volume));
    return true;
#endif
    return false;
}


bool ffwd_rew(int button)
{
    unsigned int step = 0;     /* current ff/rewind step */ 
    unsigned int max_step = 0; /* maximum ff/rewind step */ 
    int ff_rewind_count = 0;   /* current ff/rewind count (in ticks) */
    int direction = -1;         /* forward=1 or backward=-1 */
    bool exit = false;
    bool usb = false;
    int i = 0;
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
            case ACTION_WPS_SEEKBACK:
                if (wps_state.ff_rewind)
                {
                    if (direction == 1)
                    {
                        /* fast forwarding, calc max step relative to end */
                        max_step = (wps_state.id3->length - 
                                    (wps_state.id3->elapsed +
                                     ff_rewind_count)) *
                                     FF_REWIND_MAX_PERCENT / 100;
                    }
                    else
                    {
                        /* rewinding, calc max step relative to start */
                        max_step = (wps_state.id3->elapsed + ff_rewind_count) *
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
                          wps_state.id3 && wps_state.id3->length )
                    {
                        if (!wps_state.paused)
#if (CONFIG_CODEC == SWCODEC)
                            audio_pre_ff_rewind();
#else
                            audio_pause();
#endif
#if CONFIG_KEYPAD == PLAYER_PAD
                        FOR_NB_SCREENS(i)
                            gui_wps[i].display->stop_scroll();
#endif
                        if (direction > 0) 
                            status_set_ffmode(STATUS_FASTFORWARD);
                        else
                            status_set_ffmode(STATUS_FASTBACKWARD);

                        wps_state.ff_rewind = true;

                        step = 1000 * global_settings.ff_rewind_min_step;
                    }
                    else
                        break;
                }

                if (direction > 0) {
                    if ((wps_state.id3->elapsed + ff_rewind_count) >
                        wps_state.id3->length)
                        ff_rewind_count = wps_state.id3->length -
                            wps_state.id3->elapsed;
                }
                else {
                    if ((int)(wps_state.id3->elapsed + ff_rewind_count) < 0)
                        ff_rewind_count = -wps_state.id3->elapsed;
                }

                /* set the wps state ff_rewind_count so the progess info
                   displays corectly */
                wps_state.ff_rewind_count = (wps_state.wps_time_countup == false)?
                                            ff_rewind_count:-ff_rewind_count;
                FOR_NB_SCREENS(i)
                {
                    skin_update(&gui_wps[i],
                                WPS_REFRESH_PLAYER_PROGRESS |
                                WPS_REFRESH_DYNAMIC);
                }

                break;

            case ACTION_WPS_STOPSEEK:
                wps_state.id3->elapsed = wps_state.id3->elapsed+ff_rewind_count;
                audio_ff_rewind(wps_state.id3->elapsed);
                wps_state.ff_rewind_count = 0;
                wps_state.ff_rewind = false;
                status_set_ffmode(0);
#if (CONFIG_CODEC != SWCODEC)
                if (!wps_state.paused)
                    audio_resume();
#endif
#ifdef HAVE_LCD_CHARCELLS
                FOR_NB_SCREENS(i)
                    skin_update(&gui_wps[i], WPS_REFRESH_ALL);
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
        {
            button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,TIMEOUT_BLOCK);
#ifdef HAVE_TOUCHSCREEN
            if (button == ACTION_TOUCHSCREEN)
                button = wps_get_touchaction(gui_wps[SCREEN_MAIN].data);
#endif
        }
    }
    return usb;
}


void display_keylock_text(bool locked)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_wps[i].display->stop_scroll();

    splash(HZ, locked ? ID2P(LANG_KEYLOCK_ON) : ID2P(LANG_KEYLOCK_OFF));
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
    if (wps_state.id3->elapsed < skip_thresh)
    {
        audio_prev();
        return;
    }
    else
    {
        if (wps_state.id3->cuesheet)
        {
            curr_cuesheet_skip(wps_state.id3->cuesheet, -1, wps_state.id3->elapsed);
            return;
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
}

static void next_track(void)
{
    /* take care of if we're playing a cuesheet */
    if (wps_state.id3->cuesheet)
    {
        if (curr_cuesheet_skip(wps_state.id3->cuesheet, 1, wps_state.id3->elapsed))
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
    long step = global_settings.skip_length*1000;
    long elapsed = wps_state.id3->elapsed;
    long remaining = wps_state.id3->length - elapsed;

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
#if CONFIG_CODEC == SWCODEC
        if(global_settings.beep)
            pcmbuf_beep(1000, 150, 1500*global_settings.beep);
#endif
        return;
    }
    else if ((direction == -1 && elapsed < step))
    {
        elapsed = 0;
    }
    else
    {
        elapsed += step * direction;
    }
    if((audio_status() & AUDIO_STATUS_PLAY) && !wps_state.paused)
    {
#if (CONFIG_CODEC == SWCODEC)
        audio_pre_ff_rewind();
#else
        audio_pause();
#endif
    }
    audio_ff_rewind(wps_state.id3->elapsed = elapsed);
#if (CONFIG_CODEC != SWCODEC)
    if (!wps_state.paused)
        audio_resume();
#endif
}


#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
/*
 * If the user is unable to see the wps, because the display is deactivated,
 * we suppress updates until the wps is activated again (the lcd driver will
 * call this hook to issue an instant update)
 * */
static void wps_lcd_activation_hook(void *param)
{
    (void)param;
    wps_sync_data.do_full_update = true;
    /* force timeout in wps main loop, so that the update is instantly */
    queue_post(&button_queue, BUTTON_NONE, 0);
}
#endif

static void gwps_leave_wps(void)
{
    int i;

    FOR_NB_SCREENS(i)
    {
        gui_wps[i].display->stop_scroll();
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
        gui_wps[i].display->backdrop_show(sb_get_backdrop(i));
#endif
        
#ifdef HAVE_LCD_BITMAP
        bool draw = false;
        if (gui_wps[i].data->wps_sb_tag)
            draw = gui_wps[i].data->show_sb_on_wps;
        else if (statusbar_position(i) != STATUSBAR_OFF)
            draw = true;
#endif
        viewportmanager_theme_undo(i, draw);
        
    }

#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
    /* Play safe and unregister the hook */
    remove_event(LCD_EVENT_ACTIVATION, wps_lcd_activation_hook);
#endif
    /* unhandle statusbar update delay */
    sb_skin_set_update_delay(DEFAULT_UPDATE_DELAY);
}

/*
 * display the wps on entering or restoring */
static void gwps_enter_wps(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
        struct gui_wps *gwps = &gui_wps[i];
        struct screen *display = gwps->display;
#ifdef HAVE_LCD_BITMAP
        bool draw = false;
        if (gui_wps[i].data->wps_sb_tag)
            draw = gui_wps[i].data->show_sb_on_wps;
        else if (statusbar_position(i) != STATUSBAR_OFF)
            draw = true;
#endif
        display->stop_scroll();
        viewportmanager_theme_enable(i, draw, NULL);

        /* Update the values in the first (default) viewport - in case the user
           has modified the statusbar or colour settings */
#if LCD_DEPTH > 1
        if (display->depth > 1)
        {
            struct viewport *vp = &find_viewport(VP_DEFAULT_LABEL, gwps->data)->vp;
            vp->fg_pattern = display->get_foreground();
            vp->bg_pattern = display->get_background();
        }
#endif
        /* make the backdrop actually take effect */
#if LCD_DEPTH > 1 || defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
        display->backdrop_show(gwps->data->backdrop);
#endif
        display->clear_display();
        skin_update(gwps, WPS_REFRESH_ALL);

#ifdef HAVE_TOUCHSCREEN
        wps_disarm_touchregions(gui_wps[i].data);
#endif
    }
    /* force statusbar/skin update since we just cleared the whole screen */
    send_event(GUI_EVENT_ACTIONUPDATE, (void*)1);
}

#ifdef HAVE_TOUCHSCREEN
/** Disarms all touchregions. */
static void wps_disarm_touchregions(struct wps_data *data)
{
    struct skin_token_list *regions = data->touchregions;
    while (regions)
    {
        ((struct touchregion *)regions->token->value.data)->armed = false;
        regions = regions->next;
    }
}

int wps_get_touchaction(struct wps_data *data)
{
    int returncode = ACTION_NONE;
    short x,y;
    short vx, vy;
    int type = action_get_touchscreen_press(&x, &y);
    static int last_action = ACTION_NONE;
    struct touchregion *r;
    bool repeated = (type == BUTTON_REPEAT);
    bool released = (type == BUTTON_REL);
    bool pressed = (type == BUTTON_TOUCHSCREEN);
    struct skin_token_list *regions = data->touchregions;

    while (regions)
    {
        r = (struct touchregion *)regions->token->value.data;
        /* make sure this region's viewport is visible */
        if (r->wvp->hidden_flags&VP_DRAW_HIDDEN)
        {
            regions = regions->next;
            continue;
        }
        /* check if it's inside this viewport */
        if (viewport_point_within_vp(&(r->wvp->vp), x, y))
        {   /* reposition the touch inside the viewport since touchregions
             * are relative to a preceding viewport */
            vx = x - r->wvp->vp.x;
            vy = y - r->wvp->vp.y;
            /* now see if the point is inside this region */
            if (vx >= r->x && vx < r->x+r->width &&
                vy >= r->y && vy < r->y+r->height)
            {
                /* reposition the touch within the area */
                vx -= r->x;
                vy -= r->y;

                switch(r->type)
                {
                    case WPS_TOUCHREGION_ACTION:
                        if (r->armed && ((repeated && r->repeat) || (released && !r->repeat)))
                        {
                            last_action = r->action;
                            returncode = r->action;
                        }
                        if (pressed)
                            r->armed = true;
                        break;
                    case WPS_TOUCHREGION_SCROLLBAR:
                        if(r->width > r->height)
                            /* landscape */
                            wps_state.id3->elapsed = (vx *
                                    wps_state.id3->length) / r->width;
                        else
                            /* portrait */
                            wps_state.id3->elapsed = (vy *
                                    wps_state.id3->length) / r->height;

                        if (!wps_state.paused)
#if (CONFIG_CODEC == SWCODEC)
                            audio_pre_ff_rewind();
#else
                            audio_pause();
#endif
                        audio_ff_rewind(wps_state.id3->elapsed);
#if (CONFIG_CODEC != SWCODEC)
                        if (!wps_state.paused)
                            audio_resume();
#endif
                        break;
                    case WPS_TOUCHREGION_VOLUME:
                    {
                        const int min_vol = sound_min(SOUND_VOLUME);
                        const int max_vol = sound_max(SOUND_VOLUME);
                        if(r->width > r->height)
                            /* landscape */
                            global_settings.volume = (vx *
                                            (max_vol - min_vol)) / r->width;
                        else
                            /* portrait */
                            global_settings.volume = ((r->height - vy) *
                                                (max_vol-min_vol)) / r->height;

                        global_settings.volume += min_vol;
                        setvol();
                        returncode = ACTION_REDRAW;
                    }
                }
            }
        }
        regions = regions->next;
    }

    /* On release, all regions are disarmed. */
    if (released)
    	wps_disarm_touchregions(data);

    if (returncode != ACTION_NONE)
    	return returncode;

    if ((last_action == ACTION_WPS_SEEKBACK || last_action == ACTION_WPS_SEEKFWD))
        return ACTION_WPS_STOPSEEK;
    last_action = ACTION_TOUCHSCREEN;
    return ACTION_TOUCHSCREEN;
}
#endif
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
    int i;
    long last_left = 0, last_right = 0;

#ifdef HAVE_LCD_CHARCELLS
    status_set_audio(true);
    status_set_param(false);
#endif

#ifdef AB_REPEAT_ENABLE
    ab_repeat_init();
    ab_reset_markers();
#endif
    wps_state_init();
    
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
                call_storage_idle_notifys(true);
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
                /* check for restore to not let the peakmeter delay the
                 * initial draw of the wps, don't delay handling of button
                 * presses either */
                if (button != ACTION_NONE || restore) {
                    break;
                }
                peak_meter_peek();
                sleep(0);   /* Sleep until end of current tick. */

                if (TIME_AFTER(current_tick, next_refresh)) {
                    FOR_NB_SCREENS(i)
                    {
                        if(gui_wps[i].data->peak_meter_enabled)
                            skin_update(&gui_wps[i], WPS_REFRESH_PEAK_METER);
                        next_refresh += HZ / PEAK_METER_FPS;
                    }
                }
            }

        }

        /* The peak meter is disabled
           -> no additional screen updates needed */
        else
#endif
        {   /* 1 is the minimum timeout which lets other threads run.
             * audio thread (apprently) needs to run before displaying the wps
             * or bad things happen with regards to cuesheet
             * (probably a race condition, on sh at least) */
            button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,
                restore ? 1 : HZ/5);
        }

        /* Exit if audio has stopped playing. This happens e.g. at end of
           playlist or if using the sleep timer. */
        if (!(audio_status() & AUDIO_STATUS_PLAY))
            exit = true;
#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
            button = wps_get_touchaction(gui_wps[SCREEN_MAIN].data);
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
            case ACTION_WPS_HOTKEY:
                if (!global_settings.hotkey_wps)
                    break;
                /* fall through */
            case ACTION_WPS_CONTEXT:
            {
                bool hotkey = button == ACTION_WPS_HOTKEY;
                gwps_leave_wps();
                int retval = onplay(wps_state.id3->path, 
                           FILE_ATTR_AUDIO, CONTEXT_WPS, hotkey);
                /* if music is stopped in the context menu we want to exit the wps */
                if (retval == ONPLAY_MAINMENU 
                    || !audio_status())
                    return GO_TO_ROOT;
                else if (retval == ONPLAY_PLAYLIST)
                    return GO_TO_PLAYLIST_VIEWER;
                restore = true;
            }
            break;

            case ACTION_WPS_BROWSE:
#ifdef HAVE_LCD_CHARCELLS
                status_set_record(false);
                status_set_audio(false);
#endif
                gwps_leave_wps();
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
                        fade(true, true);
                    else
                        audio_resume();
                }
                else
                {
                    wps_state.paused = true;
                    if ( global_settings.fade_on_stop )
                        fade(false, true);
                    else
                        audio_pause();
                    settings_save();
#if !defined(HAVE_RTC_RAM) && !defined(HAVE_SW_POWEROFF)
                    call_storage_idle_notifys(true);   /* make sure resume info is saved */
#endif
                }
                break;

            case ACTION_WPS_VOLUP:
                global_settings.volume++;
                vol_changed = true;
                break;
            case ACTION_WPS_VOLDOWN:
                global_settings.volume--;
                vol_changed = true;
                break;
            /* fast forward 
                OR next dir if this is straight after ACTION_WPS_SKIPNEXT */
            case ACTION_WPS_SEEKFWD:
                if (global_settings.party_mode)
                    break;
                if (current_tick -last_right < HZ)
                {
                    if (wps_state.id3->cuesheet)
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
                    if (wps_state.id3->cuesheet)
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
                    ab_set_B_marker(wps_state.id3->elapsed);
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
                    ab_set_A_marker(wps_state.id3->elapsed);
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
                if (quick_screen_quick(button))
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* HAVE_QUICKSCREEN */

                /* screen settings */
#ifdef BUTTON_F3
            case ACTION_F3:
            {
                gwps_leave_wps();
                if (quick_screen_f3(BUTTON_F3))
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* BUTTON_F3 */

                /* pitch screen */
#ifdef HAVE_PITCHSCREEN
            case ACTION_WPS_PITCHSCREEN:
            {
                gwps_leave_wps();
                if (1 == gui_syncpitchscreen_run())
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#endif /* HAVE_PITCHSCREEN */

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

            case ACTION_WPS_ID3SCREEN:
            {
                gwps_leave_wps();
                if (browse_id3())
                    return GO_TO_ROOT;
                restore = true;
            }
            break;
#ifdef HAVE_TOUCHSCREEN
            case ACTION_TOUCH_SHUFFLE: /* toggle shuffle mode */
            {
                global_settings.playlist_shuffle = 
                                                !global_settings.playlist_shuffle;
#if CONFIG_CODEC == SWCODEC
                dsp_set_replaygain();
#endif
                if (global_settings.playlist_shuffle)
                    playlist_randomise(NULL, current_tick, true);
                else
                    playlist_sort(NULL, true);
            }
            break;
            case ACTION_TOUCH_REPMODE: /* cycle the repeat mode setting */
            {
                const struct settings_list *rep_setting = 
                                find_setting(&global_settings.repeat_mode, NULL);
                option_select_next_val(rep_setting, false, true);
                audio_flush_and_reload_tracks();
            }
            break;
#endif /* HAVE_TOUCHSCREEN */
             /* this case is used by the softlock feature
              * it requests a full update here */
            case ACTION_REDRAW:
                wps_sync_data.do_full_update = true;
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
            case SYS_POWEROFF:
                default_event_handler(SYS_POWEROFF);
                break;
            case ACTION_WPS_VIEW_PLAYLIST:
                gwps_leave_wps();
                return GO_TO_PLAYLIST_VIEWER;
                break;
            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                {
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
                if(update_onvol_change(&gui_wps[i]))
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
            add_event(LCD_EVENT_ACTIVATION, false, wps_lcd_activation_hook);
#endif
        /* we remove the update delay since it's not very usable in the wps,
         * e.g. during volume changing or ffwd/rewind */
            sb_skin_set_update_delay(0);
            wps_sync_data.do_full_update = update = false;
            gwps_enter_wps();
        }
        else if (wps_sync_data.do_full_update || update)
        {
#if defined(HAVE_BACKLIGHT) || defined(HAVE_REMOTE_LCD)
            gwps_caption_backlight(&wps_state);
#endif
            FOR_NB_SCREENS(i)
            {
#if defined(HAVE_LCD_ENABLE) || defined(HAVE_LCD_SLEEP)
                /* currently, all remotes are readable without backlight
                 * so still update those */
                if (lcd_active() || (i != SCREEN_MAIN))
#endif
                {
                    skin_update(&gui_wps[i], wps_sync_data.do_full_update ?
                                            WPS_REFRESH_ALL : WPS_REFRESH_NON_STATIC);
                }
            }
            wps_sync_data.do_full_update = false;
            update = false;
        }

        if (exit) {
#ifdef HAVE_LCD_CHARCELLS
            status_set_record(false);
            status_set_audio(false);
#endif
            if (global_settings.fade_on_stop)
                fade(false, true);

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
static void track_changed_callback(void *param)
{
    wps_state.id3 = (struct mp3entry*)param;
    wps_state.nid3 = audio_next_track();
    if (wps_state.id3->cuesheet)
    {
        cue_find_current_track(wps_state.id3->cuesheet, wps_state.id3->elapsed);
        cue_spoof_id3(wps_state.id3->cuesheet, wps_state.id3);
    }
    wps_sync_data.do_full_update = true;
}
static void nextid3available_callback(void* param)
{
    (void)param;
    wps_state.nid3 = audio_next_track();
    wps_sync_data.do_full_update = true;
}


static void wps_state_init(void)
{
    wps_state.ff_rewind = false;
    wps_state.paused = false;
    if(audio_status() & AUDIO_STATUS_PLAY)
    {
        wps_state.id3 = audio_current_track();
        wps_state.nid3 = audio_next_track();
    }
    else
    {
        wps_state.id3 = NULL;
        wps_state.nid3 = NULL;
    }
    /* We'll be updating due to restore initialized with true */
    wps_sync_data.do_full_update = false;
    /* add the WPS track event callbacks */
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, false, track_changed_callback);
    add_event(PLAYBACK_EVENT_NEXTTRACKID3_AVAILABLE, false, nextid3available_callback);
}


void gui_sync_wps_init(void)
{
    int i;
    FOR_NB_SCREENS(i)
    {
#ifdef HAVE_ALBUMART
        wps_datas[i].albumart = NULL;
        wps_datas[i].playback_aa_slot = -1;
#endif
        gui_wps[i].data = &wps_datas[i];
        gui_wps[i].display = &screens[i];
        /* Currently no seperate wps_state needed/possible
           so use the only available ( "global" ) one */
        gui_wps[i].state = &wps_state;
        /* must point to the same struct for both screens */
        gui_wps[i].sync_data = &wps_sync_data;
    }
}


#ifdef IPOD_ACCESSORY_PROTOCOL
bool is_wps_fading(void)
{
    return wps_state.is_fading;
}

int wps_get_ff_rewind_count(void)
{
    return wps_state.ff_rewind_count;
}
#endif

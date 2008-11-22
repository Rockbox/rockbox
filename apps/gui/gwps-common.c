/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002-2007 Björn Stenberg
 * Copyright (C) 2007-2008 Nicolas Pennequin
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
#include "gwps-common.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "settings.h"
#include "rbunicode.h"
#include "rtc.h"
#include "audio.h"
#include "status.h"
#include "power.h"
#include "powermgmt.h"
#include "sound.h"
#include "debug.h"
#ifdef HAVE_LCD_CHARCELLS
#include "hwcompat.h"
#endif
#include "abrepeat.h"
#include "mp3_playback.h"
#include "backlight.h"
#include "lang.h"
#include "misc.h"
#include "splash.h"
#include "scrollbar.h"
#include "led.h"
#include "lcd.h"
#ifdef HAVE_LCD_BITMAP
#include "peakmeter.h"
/* Image stuff */
#include "bmp.h"
#include "albumart.h"
#endif
#include "dsp.h"
#include "action.h"
#include "cuesheet.h"
#include "playlist.h"
#if CONFIG_CODEC == SWCODEC
#include "playback.h"
#endif
#include "backdrop.h"

#define FF_REWIND_MAX_PERCENT 3 /* cap ff/rewind step size at max % of file */ 
                                /* 3% of 30min file == 54s step size */
#define MIN_FF_REWIND_STEP 500

/* Timeout unit expressed in HZ. In WPS, all timeouts are given in seconds
   (possibly with a decimal fraction) but stored as integer values.
   E.g. 2.5 is stored as 25. This means 25 tenth of a second, i.e. 25 units.
*/
#define TIMEOUT_UNIT (HZ/10) /* I.e. 0.1 sec */
#define DEFAULT_SUBLINE_TIME_MULTIPLIER 20 /* In TIMEOUT_UNIT's */


/* draws the statusbar on the given wps-screen */
#ifdef HAVE_LCD_BITMAP
static void gui_wps_statusbar_draw(struct gui_wps *wps, bool force)
{
    bool draw = global_settings.statusbar;

    if (wps->data->wps_sb_tag)
        draw = wps->data->show_sb_on_wps;

    if (draw)
        gui_statusbar_draw(wps->statusbar, force);
}
#else
#define gui_wps_statusbar_draw(wps, force) \
    gui_statusbar_draw((wps)->statusbar, (force))
#endif
#include "pcmbuf.h"

/* fades the volume */
bool wps_fading_out = false;
void fade(bool fade_in, bool updatewps)
{
    int fp_global_vol = global_settings.volume << 8;
    int fp_min_vol = sound_min(SOUND_VOLUME) << 8;
    int fp_step = (fp_global_vol - fp_min_vol) / 30;
    int i;
    wps_fading_out = !fade_in;
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
                    gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_NON_STATIC);
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
                    gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_NON_STATIC);
            }
            sleep(1);
        }
        audio_pause();
        wps_fading_out = false;
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

/* return true if screen restore is needed
   return false otherwise
*/
bool update_onvol_change(struct gui_wps * gwps)
{
    gui_wps_statusbar_draw(gwps, false);
    gui_wps_refresh(gwps, 0, WPS_REFRESH_NON_STATIC);

#ifdef HAVE_LCD_CHARCELLS
    splashf(0, "Vol: %3d dB",
               sound_val2phys(SOUND_VOLUME, global_settings.volume));
    return true;
#endif
    return false;
}

void play_hop(int direction)
{
    if(!wps_state.id3 || !wps_state.id3->length
       || global_settings.skip_length == 0)
        return;
#define STEP ((unsigned)global_settings.skip_length*1000)
    if(direction == 1
       && wps_state.id3->length - wps_state.id3->elapsed < STEP+1000) {
#if CONFIG_CODEC == SWCODEC
        if(global_settings.beep)
            pcmbuf_beep(1000, 150, 1500*global_settings.beep);
#endif
        return;
    }
    if((direction == -1 && wps_state.id3->elapsed < STEP))
        wps_state.id3->elapsed = 0;
    else
        wps_state.id3->elapsed += STEP *direction;
    if((audio_status() & AUDIO_STATUS_PLAY) && !wps_state.paused) {
#if (CONFIG_CODEC == SWCODEC)
        audio_pre_ff_rewind();
#else
        audio_pause();
#endif
    }
    audio_ff_rewind(wps_state.id3->elapsed);
#if (CONFIG_CODEC != SWCODEC)
    if (!wps_state.paused)
        audio_resume();
#endif
#undef STEP
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

                FOR_NB_SCREENS(i)
                    gui_wps_refresh(&gui_wps[i],
                                    (wps_state.wps_time_countup == false)?
                                    ff_rewind_count:-ff_rewind_count,
                                    WPS_REFRESH_PLAYER_PROGRESS |
                                    WPS_REFRESH_DYNAMIC);

                break;

            case ACTION_WPS_STOPSEEK:
                wps_state.id3->elapsed = wps_state.id3->elapsed+ff_rewind_count;
                audio_ff_rewind(wps_state.id3->elapsed);
                ff_rewind_count = 0;
                wps_state.ff_rewind = false;
                status_set_ffmode(0);
#if (CONFIG_CODEC != SWCODEC)
                if (!wps_state.paused)
                    audio_resume();
#endif
#ifdef HAVE_LCD_CHARCELLS
                gui_wps_display();
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
            button = get_action(CONTEXT_WPS|ALLOW_SOFTLOCK,TIMEOUT_BLOCK);
    }
    return usb;
}

bool gui_wps_display(void)
{
    int i;
    if (!wps_state.id3 && !(audio_status() & AUDIO_STATUS_PLAY))
    {
        global_status.resume_index = -1;
#ifdef HAVE_LCD_BITMAP
        gui_syncstatusbar_draw(&statusbars, true);
#endif
        splash(HZ, ID2P(LANG_END_PLAYLIST));
        return true;
    }
    else
    {
        FOR_NB_SCREENS(i)
        {
            /* Update the values in the first (default) viewport - in case the user
               has modified the statusbar or colour settings */
#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
            if (gui_wps[i].display->depth > 1)
            {
                gui_wps[i].data->viewports[0].vp.fg_pattern = gui_wps[i].display->get_foreground();
                gui_wps[i].data->viewports[0].vp.bg_pattern = gui_wps[i].display->get_background();
            }
#endif
#endif

            gui_wps[i].display->clear_display();
            if (!gui_wps[i].data->wps_loaded) {
                if ( !gui_wps[i].data->num_tokens ) {
                    /* set the default wps for the main-screen */
                    if(i == 0)
                    {
#ifdef HAVE_LCD_BITMAP
#if LCD_DEPTH > 1
                        unload_wps_backdrop();
#endif
                        wps_data_load(gui_wps[i].data,
                                      gui_wps[i].display,
                                      "%s%?it<%?in<%in. |>%it|%fn>\n"
                                      "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                                      "%s%?id<%id|%?d1<%d1|(root)>> %?iy<(%iy)|>\n"
                                      "\n"
                                      "%al%pc/%pt%ar[%pp:%pe]\n"
                                      "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
                                      "%pb\n"
                                      "%pm\n", false);
#else
                        wps_data_load(gui_wps[i].data,
                                      gui_wps[i].display,
                                      "%s%pp/%pe: %?it<%it|%fn> - %?ia<%ia|%d2> - %?id<%id|%d1>\n"
                                      "%pc%?ps<*|/>%pt\n", false);
#endif
                    }
#if NB_SCREENS == 2
                     /* set the default wps for the remote-screen */
                     else if(i == 1)
                     {
#if defined(HAVE_REMOTE_LCD) && LCD_REMOTE_DEPTH > 1
                         unload_remote_wps_backdrop();
#endif
                         wps_data_load(gui_wps[i].data,
                                       gui_wps[i].display,
                                       "%s%?ia<%ia|%?d2<%d2|(root)>>\n"
                                       "%s%?it<%?in<%in. |>%it|%fn>\n"
                                       "%al%pc/%pt%ar[%pp:%pe]\n"
                                       "%fbkBit %?fv<avg|> %?iv<(id3v%iv)|(no id3)>\n"
                                       "%pb\n", false);
                     }
#endif
                }
            }
        }
    }
    yield();
    FOR_NB_SCREENS(i)
    {
        gui_wps_refresh(&gui_wps[i], 0, WPS_REFRESH_ALL);
    }
    return false;
}

bool update(struct gui_wps *gwps)
{
    bool track_changed = audio_has_changed_track();
    bool retcode = false;

    gwps->state->nid3 = audio_next_track();
    if (track_changed)
    {
        gwps->display->stop_scroll();
        gwps->state->id3 = audio_current_track();

        if (cuesheet_is_enabled() && gwps->state->id3->cuesheet_type
            && strcmp(gwps->state->id3->path, curr_cue->audio_filename))
        {
            /* the current cuesheet isn't the right one any more */

            if (!strcmp(gwps->state->id3->path, temp_cue->audio_filename)) {
                /* We have the new cuesheet in memory (temp_cue),
                   let's make it the current one ! */
                memcpy(curr_cue, temp_cue, sizeof(struct cuesheet));
            }
            else {
                /* We need to parse the new cuesheet */

                char cuepath[MAX_PATH];

                if (look_for_cuesheet_file(gwps->state->id3->path, cuepath) &&
                    parse_cuesheet(cuepath, curr_cue))
                {
                    gwps->state->id3->cuesheet_type = 1;
                    strcpy(curr_cue->audio_filename, gwps->state->id3->path);
                }
            }

            cue_spoof_id3(curr_cue, gwps->state->id3);
        }

        if (gui_wps_display())
            retcode = true;
        else{
            gui_wps_refresh(gwps, 0, WPS_REFRESH_ALL);
        }

        if (gwps->state->id3)
        {
            strncpy(gwps->state->current_track_path, gwps->state->id3->path,
                   sizeof(gwps->state->current_track_path));
            gwps->state->current_track_path[sizeof(gwps->state->current_track_path)-1] = '\0';
        }
    }

    if (gwps->state->id3)
    {
        if (cuesheet_is_enabled() && gwps->state->id3->cuesheet_type
            && (gwps->state->id3->elapsed < curr_cue->curr_track->offset
                || (curr_cue->curr_track_idx < curr_cue->track_count - 1
                    && gwps->state->id3->elapsed >= (curr_cue->curr_track+1)->offset)))
        {
            /* We've changed tracks within the cuesheet :
               we need to update the ID3 info and refresh the WPS */

            cue_find_current_track(curr_cue, gwps->state->id3->elapsed);
            cue_spoof_id3(curr_cue, gwps->state->id3);

            gwps->display->stop_scroll();
            if (gui_wps_display())
                retcode = true;
            else
                gui_wps_refresh(gwps, 0, WPS_REFRESH_ALL);
        }
        else
            gui_wps_refresh(gwps, 0, WPS_REFRESH_NON_STATIC);
    }

    gui_wps_statusbar_draw(gwps, false);

    return retcode;
}


void display_keylock_text(bool locked)
{
    int i;
    FOR_NB_SCREENS(i)
        gui_wps[i].display->stop_scroll();

    splash(HZ, locked ? ID2P(LANG_KEYLOCK_ON) : ID2P(LANG_KEYLOCK_OFF));
}

#ifdef HAVE_LCD_BITMAP

static void draw_progressbar(struct gui_wps *gwps,
                             struct progressbar *pb)
{
    struct screen *display = gwps->display;
    struct wps_state *state = gwps->state;
    if (pb->have_bitmap_pb)
        gui_bitmap_scrollbar_draw(display, pb->bm,
                                  pb->x, pb->y, pb->width, pb->bm.height,
                                  state->id3->length ? state->id3->length : 1, 0,
                                  state->id3->length ? state->id3->elapsed
                                          + state->ff_rewind_count : 0,
                                          HORIZONTAL);
    else
        gui_scrollbar_draw(display, pb->x, pb->y, pb->width, pb->height,
                           state->id3->length ? state->id3->length : 1, 0,
                           state->id3->length ? state->id3->elapsed
                                   + state->ff_rewind_count : 0,
                                   HORIZONTAL);
#ifdef AB_REPEAT_ENABLE
    if ( ab_repeat_mode_enabled() && state->id3->length != 0 )
        ab_draw_markers(display, state->id3->length,
                        pb->x, pb->x + pb->width, pb->y, pb->height);
#endif

    if ( cuesheet_is_enabled() && state->id3->cuesheet_type )
        cue_draw_markers(display, state->id3->length,
                         pb->x, pb->x + pb->width, pb->y+1, pb->height-2);
}

/* clears the area where the image was shown */
static void clear_image_pos(struct gui_wps *gwps, int n)
{
    if(!gwps)
        return;
    struct wps_data *data = gwps->data;
    gwps->display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    gwps->display->fillrect(data->img[n].x, data->img[n].y,
                            data->img[n].bm.width, data->img[n].subimage_height);
    gwps->display->set_drawmode(DRMODE_SOLID);
}

static void wps_draw_image(struct gui_wps *gwps, int n, int subimage)
{
    struct screen *display = gwps->display;
    struct wps_data *data = gwps->data;
    if(data->img[n].always_display)
        display->set_drawmode(DRMODE_FG);
    else
        display->set_drawmode(DRMODE_SOLID);

#if LCD_DEPTH > 1
    if(data->img[n].bm.format == FORMAT_MONO) {
#endif
        display->mono_bitmap_part(data->img[n].bm.data, 
                                  0, data->img[n].subimage_height * subimage,
                                  data->img[n].bm.width, data->img[n].x,
                                  data->img[n].y, data->img[n].bm.width,
                                  data->img[n].subimage_height);
#if LCD_DEPTH > 1
    } else {
        display->transparent_bitmap_part((fb_data *)data->img[n].bm.data,
                                         0, data->img[n].subimage_height * subimage,
                                         data->img[n].bm.width, data->img[n].x,
                                         data->img[n].y, data->img[n].bm.width,
                                         data->img[n].subimage_height);
    }
#endif
}

static void wps_display_images(struct gui_wps *gwps, struct viewport* vp)
{
    if(!gwps || !gwps->data || !gwps->display)
        return;

    int n;
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;

    for (n = 0; n < MAX_IMAGES; n++)
    {
        if (data->img[n].loaded)
        {
            if (data->img[n].display >= 0)
            {
                wps_draw_image(gwps, n, data->img[n].display);
            } else if (data->img[n].always_display && data->img[n].vp == vp)
            {
                wps_draw_image(gwps, n, 0);
            }
        }
    }
    display->set_drawmode(DRMODE_SOLID);
}

#else /* HAVE_LCD_CHARCELL */

static bool draw_player_progress(struct gui_wps *gwps)
{
    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    unsigned char progress_pattern[7];
    int pos = 0;
    int i;

    if (!state->id3)
        return false;

    if (state->id3->length)
        pos = 36 * (state->id3->elapsed + state->ff_rewind_count)
              / state->id3->length;

    for (i = 0; i < 7; i++, pos -= 5)
    {
        if (pos <= 0)
            progress_pattern[i] = 0x1f;
        else if (pos >= 5)
            progress_pattern[i] = 0x00;
        else
            progress_pattern[i] = 0x1f >> pos;
    }

    display->define_pattern(gwps->data->wps_progress_pat[0], progress_pattern);
    return true;
}

static void draw_player_fullbar(struct gui_wps *gwps, char* buf, int buf_size)
{
    static const unsigned char numbers[10][4] = {
        {0x0e, 0x0a, 0x0a, 0x0e}, /* 0 */
        {0x04, 0x0c, 0x04, 0x04}, /* 1 */
        {0x0e, 0x02, 0x04, 0x0e}, /* 2 */
        {0x0e, 0x02, 0x06, 0x0e}, /* 3 */
        {0x08, 0x0c, 0x0e, 0x04}, /* 4 */
        {0x0e, 0x0c, 0x02, 0x0c}, /* 5 */
        {0x0e, 0x08, 0x0e, 0x0e}, /* 6 */
        {0x0e, 0x02, 0x04, 0x08}, /* 7 */
        {0x0e, 0x0e, 0x0a, 0x0e}, /* 8 */
        {0x0e, 0x0e, 0x02, 0x0e}, /* 9 */
    };

    struct wps_state *state = gwps->state;
    struct screen *display = gwps->display;
    struct wps_data *data = gwps->data;
    unsigned char progress_pattern[7];
    char timestr[10];
    int time;
    int time_idx = 0;
    int pos = 0;
    int pat_idx = 1;
    int digit, i, j;
    bool softchar;

    if (!state->id3 || buf_size < 34) /* worst case: 11x UTF-8 char + \0 */
        return;

    time = state->id3->elapsed + state->ff_rewind_count;
    if (state->id3->length)
        pos = 55 * time / state->id3->length;

    memset(timestr, 0, sizeof(timestr));
    format_time(timestr, sizeof(timestr)-2, time);
    timestr[strlen(timestr)] = ':';   /* always safe */

    for (i = 0; i < 11; i++, pos -= 5)
    {
        softchar = false;
        memset(progress_pattern, 0, sizeof(progress_pattern));
        
        if ((digit = timestr[time_idx]))
        {
            softchar = true;
            digit -= '0';

            if (timestr[time_idx + 1] == ':')  /* ones, left aligned */
            {   
                memcpy(progress_pattern, numbers[digit], 4);
                time_idx += 2;
            }
            else  /* tens, shifted right */
            {
                for (j = 0; j < 4; j++)
                    progress_pattern[j] = numbers[digit][j] >> 1;

                if (time_idx > 0)  /* not the first group, add colon in front */
                {
                    progress_pattern[1] |= 0x10;
                    progress_pattern[3] |= 0x10;
                }
                time_idx++;
            }

            if (pos >= 5)
                progress_pattern[5] = progress_pattern[6] = 0x1f;
        }

        if (pos > 0 && pos < 5)
        {
            softchar = true;
            progress_pattern[5] = progress_pattern[6] = (~0x1f >> pos) & 0x1f;
        }

        if (softchar && pat_idx < 8)
        {
            display->define_pattern(data->wps_progress_pat[pat_idx],
                                    progress_pattern);
            buf = utf8encode(data->wps_progress_pat[pat_idx], buf);
            pat_idx++;
        }
        else if (pos <= 0)
            buf = utf8encode(' ', buf);
        else
            buf = utf8encode(0xe115, buf); /* 2/7 _ */
    }
    *buf = '\0';
}

#endif /* HAVE_LCD_CHARCELL */

static char* get_codectype(const struct mp3entry* id3)
{
    if (id3->codectype < AFMT_NUM_CODECS) {
        return (char*)audio_formats[id3->codectype].label;
    } else {
        return NULL;
    }
}

/* Extract a part from a path.
 *
 * buf      - buffer extract part to.
 * buf_size - size of buffer.
 * path     - path to extract from.
 * level    - what to extract. 0 is file name, 1 is parent of file, 2 is
 *            parent of parent, etc.
 *
 * Returns buf if the desired level was found, NULL otherwise.
 */
static char* get_dir(char* buf, int buf_size, const char* path, int level)
{
    const char* sep;
    const char* last_sep;
    int len;

    sep = path + strlen(path);
    last_sep = sep;

    while (sep > path)
    {
        if ('/' == *(--sep))
        {
            if (!level)
                break;

            level--;
            last_sep = sep - 1;
        }
    }

    if (level || (last_sep <= sep))
        return NULL;

    len = MIN(last_sep - sep, buf_size - 1);
    strncpy(buf, sep + 1, len);
    buf[len] = 0;
    return buf;
}

/* Return the tag found at index i and write its value in buf.
   The return value is buf if the tag had a value, or NULL if not.

   intval is used with conditionals/enums: when this function is called,
   intval should contain the number of options in the conditional/enum.
   When this function returns, intval is -1 if the tag is non numeric or,
   if the tag is numeric, *intval is the enum case we want to go to (between 1
   and the original value of *intval, inclusive).
   When not treating a conditional/enum, intval should be NULL.
*/
static const char *get_token_value(struct gui_wps *gwps,
                                   struct wps_token *token,
                                   char *buf, int buf_size,
                                   int *intval)
{
    if (!gwps)
        return NULL;

    struct wps_data *data = gwps->data;
    struct wps_state *state = gwps->state;

    if (!data || !state)
        return NULL;

    struct mp3entry *id3;

    if (token->next)
        id3 = state->nid3;
    else
        id3 = state->id3;

    if (!id3)
        return NULL;

#if CONFIG_RTC
    struct tm* tm = NULL;

    /* if the token is an RTC one, update the time
       and do the necessary checks */
    if (token->type >= WPS_TOKENS_RTC_BEGIN
        && token->type <= WPS_TOKENS_RTC_END)
    {
        tm = get_time();

        if (!valid_time(tm))
            return NULL;
    }
#endif

    int limit = 1;
    if (intval)
    {
        limit = *intval;
        *intval = -1;
    }

    switch (token->type)
    {
        case WPS_TOKEN_CHARACTER:
            return &(token->value.c);

        case WPS_TOKEN_STRING:
            return data->strings[token->value.i];

        case WPS_TOKEN_TRACK_TIME_ELAPSED:
            format_time(buf, buf_size,
                        id3->elapsed + state->ff_rewind_count);
            return buf;

        case WPS_TOKEN_TRACK_TIME_REMAINING:
            format_time(buf, buf_size,
                        id3->length - id3->elapsed -
                        state->ff_rewind_count);
            return buf;

        case WPS_TOKEN_TRACK_LENGTH:
            format_time(buf, buf_size, id3->length);
            return buf;

        case WPS_TOKEN_PLAYLIST_ENTRIES:
            snprintf(buf, buf_size, "%d", playlist_amount());
            return buf;

        case WPS_TOKEN_PLAYLIST_NAME:
            return playlist_name(NULL, buf, buf_size);

        case WPS_TOKEN_PLAYLIST_POSITION:
            snprintf(buf, buf_size, "%d", playlist_get_display_index());
            return buf;

        case WPS_TOKEN_PLAYLIST_SHUFFLE:
            if ( global_settings.playlist_shuffle )
                return "s";
            else
                return NULL;
            break;

        case WPS_TOKEN_VOLUME:
            snprintf(buf, buf_size, "%d", global_settings.volume);
            if (intval)
            {
                if (global_settings.volume == sound_min(SOUND_VOLUME))
                {
                    *intval = 1;
                }
                else if (global_settings.volume == 0)
                {
                    *intval = limit - 1;
                }
                else if (global_settings.volume > 0)
                {
                    *intval = limit;
                }
                else
                {
                    *intval = (limit - 3) * (global_settings.volume
                                             - sound_min(SOUND_VOLUME) - 1)
                              / (-1 - sound_min(SOUND_VOLUME)) + 2;
                }
            }
            return buf;

        case WPS_TOKEN_TRACK_ELAPSED_PERCENT:
            if (id3->length <= 0)
                return NULL;

            if (intval)
            {
                *intval = limit * (id3->elapsed + state->ff_rewind_count)
                          / id3->length + 1;
            }
            snprintf(buf, buf_size, "%d",
                     100*(id3->elapsed + state->ff_rewind_count) / id3->length);
            return buf;

        case WPS_TOKEN_METADATA_ARTIST:
            return id3->artist;

        case WPS_TOKEN_METADATA_COMPOSER:
            return id3->composer;

        case WPS_TOKEN_METADATA_ALBUM:
            return id3->album;

        case WPS_TOKEN_METADATA_ALBUM_ARTIST:
            return id3->albumartist;

        case WPS_TOKEN_METADATA_GROUPING:
            return id3->grouping;

        case WPS_TOKEN_METADATA_GENRE:
            return id3->genre_string;

        case WPS_TOKEN_METADATA_DISC_NUMBER:
            if (id3->disc_string)
                return id3->disc_string;
            if (id3->discnum) {
                snprintf(buf, buf_size, "%d", id3->discnum);
                return buf;
            }
            return NULL;

        case WPS_TOKEN_METADATA_TRACK_NUMBER:
            if (id3->track_string)
                return id3->track_string;

            if (id3->tracknum) {
                snprintf(buf, buf_size, "%d", id3->tracknum);
                return buf;
            }
            return NULL;

        case WPS_TOKEN_METADATA_TRACK_TITLE:
            return id3->title;

        case WPS_TOKEN_METADATA_VERSION:
            switch (id3->id3version)
            {
                case ID3_VER_1_0:
                    return "1";

                case ID3_VER_1_1:
                    return "1.1";

                case ID3_VER_2_2:
                    return "2.2";

                case ID3_VER_2_3:
                    return "2.3";

                case ID3_VER_2_4:
                    return "2.4";

                default:
                    return NULL;
            }

        case WPS_TOKEN_METADATA_YEAR:
            if( id3->year_string )
                return id3->year_string;

            if (id3->year) {
                snprintf(buf, buf_size, "%d", id3->year);
                return buf;
            }
            return NULL;

        case WPS_TOKEN_METADATA_COMMENT:
            return id3->comment;

#ifdef HAVE_ALBUMART
        case WPS_TOKEN_ALBUMART_DISPLAY:
            draw_album_art(gwps, audio_current_aa_hid(), false);
            return NULL;

        case WPS_TOKEN_ALBUMART_FOUND:
            if (audio_current_aa_hid() >= 0) {
                return "C";
            }
            return NULL;
#endif

        case WPS_TOKEN_FILE_BITRATE:
            if(id3->bitrate)
                snprintf(buf, buf_size, "%d", id3->bitrate);
            else
                return "?";
            return buf;

        case WPS_TOKEN_FILE_CODEC:
            if (intval)
            {
                if(id3->codectype == AFMT_UNKNOWN)
                    *intval = AFMT_NUM_CODECS;
                else
                    *intval = id3->codectype;
            }
            return get_codectype(id3);

        case WPS_TOKEN_FILE_FREQUENCY:
            snprintf(buf, buf_size, "%ld", id3->frequency);
            return buf;

        case WPS_TOKEN_FILE_FREQUENCY_KHZ:
            /* ignore remainders < 100, so 22050 Hz becomes just 22k */
            if ((id3->frequency % 1000) < 100)
                snprintf(buf, buf_size, "%ld", id3->frequency / 1000);
            else
                snprintf(buf, buf_size, "%ld.%d",
                        id3->frequency / 1000,
                        (id3->frequency % 1000) / 100);
            return buf;

        case WPS_TOKEN_FILE_NAME:
            if (get_dir(buf, buf_size, id3->path, 0)) {
                /* Remove extension */
                char* sep = strrchr(buf, '.');
                if (NULL != sep) {
                    *sep = 0;
                }
                return buf;
            }
            else {
                return NULL;
            }

        case WPS_TOKEN_FILE_NAME_WITH_EXTENSION:
            return get_dir(buf, buf_size, id3->path, 0);

        case WPS_TOKEN_FILE_PATH:
            return id3->path;

        case WPS_TOKEN_FILE_SIZE:
            snprintf(buf, buf_size, "%ld", id3->filesize / 1024);
            return buf;

        case WPS_TOKEN_FILE_VBR:
            return id3->vbr ? "(avg)" : NULL;

        case WPS_TOKEN_FILE_DIRECTORY:
            return get_dir(buf, buf_size, id3->path, token->value.i);

        case WPS_TOKEN_BATTERY_PERCENT:
        {
            int l = battery_level();

            if (intval)
            {
                limit = MAX(limit, 2);
                if (l > -1) {
                    /* First enum is used for "unknown level". */
                    *intval = (limit - 1) * l / 100 + 2;
                } else {
                    *intval = 1;
                }
            }

            if (l > -1) {
                snprintf(buf, buf_size, "%d", l);
                return buf;
            } else {
                return "?";
            }
        }

        case WPS_TOKEN_BATTERY_VOLTS:
        {
            unsigned int v = battery_voltage();
            snprintf(buf, buf_size, "%d.%02d", v / 1000, (v % 1000) / 10);
            return buf;
        }

        case WPS_TOKEN_BATTERY_TIME:
        {
            int t = battery_time();
            if (t >= 0)
                snprintf(buf, buf_size, "%dh %dm", t / 60, t % 60);
            else
                return "?h ?m";
            return buf;
        }

#if CONFIG_CHARGING
        case WPS_TOKEN_BATTERY_CHARGER_CONNECTED:
        {
            if(charger_input_state==CHARGER)
                return "p";
            else
                return NULL;
        }
#endif
#if CONFIG_CHARGING >= CHARGING_MONITOR
        case WPS_TOKEN_BATTERY_CHARGING:
        {
            if (charge_state == CHARGING || charge_state == TOPOFF) {
                return "c";
            } else {
                return NULL;
            }
        }
#endif
        case WPS_TOKEN_BATTERY_SLEEPTIME:
        {
            if (get_sleep_timer() == 0)
                return NULL;
            else
            {
                format_time(buf, buf_size, get_sleep_timer() * 1000);
                return buf;
            }
        }

        case WPS_TOKEN_PLAYBACK_STATUS:
        {
            int status = audio_status();
            int mode = 1;
            if (status == AUDIO_STATUS_PLAY)
                mode = 2;
            if (wps_fading_out || 
               (status & AUDIO_STATUS_PAUSE && !status_get_ffmode()))
                mode = 3;
            if (status_get_ffmode() == STATUS_FASTFORWARD)
                mode = 4;
            if (status_get_ffmode() == STATUS_FASTBACKWARD)
                mode = 5;

            if (intval) {
                *intval = mode;
            }

            snprintf(buf, buf_size, "%d", mode-1);
            return buf;
        }

        case WPS_TOKEN_REPEAT_MODE:
            if (intval)
                *intval = global_settings.repeat_mode + 1;
            snprintf(buf, buf_size, "%d", global_settings.repeat_mode);
            return buf;
        case WPS_TOKEN_RTC_12HOUR_CFG:
            if (intval)
                *intval = global_settings.timeformat + 1;
            snprintf(buf, buf_size, "%d", global_settings.timeformat);
            return buf;
#if CONFIG_RTC
        case WPS_TOKEN_RTC_DAY_OF_MONTH:
            /* d: day of month (01..31) */
            snprintf(buf, buf_size, "%02d", tm->tm_mday);
            return buf;

        case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
            /* e: day of month, blank padded ( 1..31) */
            snprintf(buf, buf_size, "%2d", tm->tm_mday);
            return buf;

        case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
            /* H: hour (00..23) */
            snprintf(buf, buf_size, "%02d", tm->tm_hour);
            return buf;

        case WPS_TOKEN_RTC_HOUR_24:
            /* k: hour ( 0..23) */
            snprintf(buf, buf_size, "%2d", tm->tm_hour);
            return buf;

        case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
            /* I: hour (01..12) */
            snprintf(buf, buf_size, "%02d",
                     (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
            return buf;

        case WPS_TOKEN_RTC_HOUR_12:
            /* l: hour ( 1..12) */
            snprintf(buf, buf_size, "%2d",
                     (tm->tm_hour % 12 == 0) ? 12 : tm->tm_hour % 12);
            return buf;

        case WPS_TOKEN_RTC_MONTH:
            /* m: month (01..12) */
            if (intval)
                *intval = tm->tm_mon + 1;
            snprintf(buf, buf_size, "%02d", tm->tm_mon + 1);
            return buf;

        case WPS_TOKEN_RTC_MINUTE:
            /* M: minute (00..59) */
            snprintf(buf, buf_size, "%02d", tm->tm_min);
            return buf;

        case WPS_TOKEN_RTC_SECOND:
            /* S: second (00..59) */
            snprintf(buf, buf_size, "%02d", tm->tm_sec);
            return buf;

        case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            /* y: last two digits of year (00..99) */
            snprintf(buf, buf_size, "%02d", tm->tm_year % 100);
            return buf;

        case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            /* Y: year (1970...) */
            snprintf(buf, buf_size, "%04d", tm->tm_year + 1900);
            return buf;

        case WPS_TOKEN_RTC_AM_PM_UPPER:
            /* p: upper case AM or PM indicator */
            return tm->tm_hour/12 == 0 ? "AM" : "PM";

        case WPS_TOKEN_RTC_AM_PM_LOWER:
            /* P: lower case am or pm indicator */
            return tm->tm_hour/12 == 0 ? "am" : "pm";

        case WPS_TOKEN_RTC_WEEKDAY_NAME:
            /* a: abbreviated weekday name (Sun..Sat) */
            return str(LANG_WEEKDAY_SUNDAY + tm->tm_wday);

        case WPS_TOKEN_RTC_MONTH_NAME:
            /* b: abbreviated month name (Jan..Dec) */
            return str(LANG_MONTH_JANUARY + tm->tm_mon);

        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
            /* u: day of week (1..7); 1 is Monday */
            if (intval)
                *intval = (tm->tm_wday == 0) ? 7 : tm->tm_wday;
            snprintf(buf, buf_size, "%1d", tm->tm_wday + 1);
            return buf;

        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            /* w: day of week (0..6); 0 is Sunday */
            if (intval)
                *intval = tm->tm_wday + 1;
            snprintf(buf, buf_size, "%1d", tm->tm_wday);
            return buf;
#else
        case WPS_TOKEN_RTC_DAY_OF_MONTH:
        case WPS_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED:
        case WPS_TOKEN_RTC_HOUR_24_ZERO_PADDED:
        case WPS_TOKEN_RTC_HOUR_24:
        case WPS_TOKEN_RTC_HOUR_12_ZERO_PADDED:
        case WPS_TOKEN_RTC_HOUR_12:
        case WPS_TOKEN_RTC_MONTH:
        case WPS_TOKEN_RTC_MINUTE:
        case WPS_TOKEN_RTC_SECOND:
        case WPS_TOKEN_RTC_AM_PM_UPPER:
        case WPS_TOKEN_RTC_AM_PM_LOWER:
        case WPS_TOKEN_RTC_YEAR_2_DIGITS:
            return "--";
        case WPS_TOKEN_RTC_YEAR_4_DIGITS:
            return "----";
        case WPS_TOKEN_RTC_WEEKDAY_NAME:
        case WPS_TOKEN_RTC_MONTH_NAME:
            return "---";
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_MON:
        case WPS_TOKEN_RTC_DAY_OF_WEEK_START_SUN:
            return "-";
#endif

#ifdef HAVE_LCD_CHARCELLS
        case WPS_TOKEN_PROGRESSBAR:
        {
            char *end = utf8encode(data->wps_progress_pat[0], buf);
            *end = '\0';
            return buf;
        }

        case WPS_TOKEN_PLAYER_PROGRESSBAR:
            if(is_new_player())
            {
                /* we need 11 characters (full line) for
                    progress-bar */
                strncpy(buf, "           ", buf_size);
            }
            else
            {
                /* Tell the user if we have an OldPlayer */
                strncpy(buf, " <Old LCD> ", buf_size);
            }
            return buf;
#endif

#ifdef HAVE_TAGCACHE
        case WPS_TOKEN_DATABASE_PLAYCOUNT:
            if (intval) {
                *intval = id3->playcount + 1;
            }
            snprintf(buf, buf_size, "%ld", id3->playcount);
            return buf;

        case WPS_TOKEN_DATABASE_RATING:
            if (intval) {
                *intval = id3->rating + 1;
            }
            snprintf(buf, buf_size, "%d", id3->rating);
            return buf;

        case WPS_TOKEN_DATABASE_AUTOSCORE:
            if (intval)
                *intval = id3->score + 1;

            snprintf(buf, buf_size, "%d", id3->score);
            return buf;
#endif

#if (CONFIG_CODEC == SWCODEC)
        case WPS_TOKEN_CROSSFADE:
            if (intval)
                *intval = global_settings.crossfade + 1;
            snprintf(buf, buf_size, "%d", global_settings.crossfade);
            return buf;

        case WPS_TOKEN_REPLAYGAIN:
        {
            int val;

            if (global_settings.replaygain == 0)
                val = 1; /* off */
            else
            {
                int type =
                    get_replaygain_mode(id3->track_gain_string != NULL,
                                        id3->album_gain_string != NULL);
                if (type < 0)
                    val = 6;    /* no tag */
                else
                    val = type + 2;

                if (global_settings.replaygain_type == REPLAYGAIN_SHUFFLE)
                    val += 2;
            }

            if (intval)
                *intval = val;

            switch (val)
            {
                case 1:
                case 6:
                    return "+0.00 dB";
                    break;
                case 2:
                case 4:
                    strncpy(buf, id3->track_gain_string, buf_size);
                    break;
                case 3:
                case 5:
                    strncpy(buf, id3->album_gain_string, buf_size);
                    break;
            }
            return buf;
        }
#endif  /* (CONFIG_CODEC == SWCODEC) */

#if (CONFIG_CODEC != MAS3507D)
        case WPS_TOKEN_SOUND_PITCH:
        {
            int val = sound_get_pitch();
            snprintf(buf, buf_size, "%d.%d",
                    val / 10, val % 10);
            return buf;
        }
#endif

        case WPS_TOKEN_MAIN_HOLD:
#ifdef HAS_BUTTON_HOLD
            if (button_hold())
#else
            if (is_keys_locked())
#endif /*hold switch or softlock*/
                return "h";
            else
                return NULL;

#ifdef HAS_REMOTE_BUTTON_HOLD
        case WPS_TOKEN_REMOTE_HOLD:
            if (remote_button_hold())
                return "r";
            else
                return NULL;
#endif

#if (CONFIG_LED == LED_VIRTUAL) || defined(HAVE_REMOTE_LCD)
        case WPS_TOKEN_VLED_HDD:
            if(led_read(HZ/2))
                return "h";
            else
                return NULL;
#endif
        case WPS_TOKEN_BUTTON_VOLUME:
            if (data->button_time_volume && 
                TIME_BEFORE(current_tick, data->button_time_volume +
                                          token->value.i * TIMEOUT_UNIT))
                return "v";
            return NULL;
        default:
            return NULL;
    }
}

/* Return the index to the end token for the conditional token at index.
   The conditional token can be either a start token or a separator
   (i.e. option) token.
*/
static int find_conditional_end(struct wps_data *data, int index)
{
    int ret = index;
    while (data->tokens[ret].type != WPS_TOKEN_CONDITIONAL_END)
        ret = data->tokens[ret].value.i;

    /* ret now is the index to the end token for the conditional. */
    return ret;
}

/* Evaluate the conditional that is at *token_index and return whether a skip
   has ocurred. *token_index is updated with the new position.
*/
static bool evaluate_conditional(struct gui_wps *gwps, int *token_index)
{
    if (!gwps)
        return false;

    struct wps_data *data = gwps->data;

    int i, cond_end;
    int cond_index = *token_index;
    char result[128];
    const char *value;
    unsigned char num_options = data->tokens[cond_index].value.i & 0xFF;
    unsigned char prev_val = (data->tokens[cond_index].value.i & 0xFF00) >> 8;

    /* treat ?xx<true> constructs as if they had 2 options. */
    if (num_options < 2)
        num_options = 2;

    int intval = num_options;
    /* get_token_value needs to know the number of options in the enum */
    value = get_token_value(gwps, &data->tokens[cond_index + 1],
                            result, sizeof(result), &intval);

    /* intval is now the number of the enum option we want to read,
       starting from 1. If intval is -1, we check if value is empty. */
    if (intval == -1)
        intval = (value && *value) ? 1 : num_options;
    else if (intval > num_options || intval < 1)
        intval = num_options;

    data->tokens[cond_index].value.i = (intval << 8) + num_options;

    /* skip to the appropriate enum case */
    int next = cond_index + 2;
    for (i = 1; i < intval; i++)
    {
        next = data->tokens[next].value.i;
    }
    *token_index = next;

    if (prev_val == intval)
    {
        /* Same conditional case as previously. Return without clearing the
           pictures */
        return false;
    }

    cond_end = find_conditional_end(data, cond_index + 2);
    for (i = cond_index + 3; i < cond_end; i++)
    {
#ifdef HAVE_LCD_BITMAP
        /* clear all pictures in the conditional and nested ones */
        if (data->tokens[i].type == WPS_TOKEN_IMAGE_PRELOAD_DISPLAY)
            clear_image_pos(gwps, data->tokens[i].value.i & 0xFF);
#endif
#ifdef HAVE_ALBUMART
        if (data->tokens[i].type == WPS_TOKEN_ALBUMART_DISPLAY)
            draw_album_art(gwps, audio_current_aa_hid(), true);
#endif
    }

    return true;
}

/* Read a (sub)line to the given alignment format buffer.
   linebuf is the buffer where the data is actually stored.
   align is the alignment format that'll be used to display the text.
   The return value indicates whether the line needs to be updated.
*/
static bool get_line(struct gui_wps *gwps,
                     int line, int subline,
                     struct align_pos *align,
                     char *linebuf,
                     int linebuf_size)
{
    struct wps_data *data = gwps->data;

    char temp_buf[128];
    char *buf = linebuf;  /* will always point to the writing position */
    char *linebuf_end = linebuf + linebuf_size - 1;
    int i, last_token_idx;
    bool update = false;

    /* alignment-related variables */
    int cur_align;
    char* cur_align_start;
    cur_align_start = buf;
    cur_align = WPS_ALIGN_LEFT;
    align->left = NULL;
    align->center = NULL;
    align->right = NULL;

    /* Process all tokens of the desired subline */
    last_token_idx = wps_last_token_index(data, line, subline);
    for (i = wps_first_token_index(data, line, subline);
         i <= last_token_idx; i++)
    {
        switch(data->tokens[i].type)
        {
            case WPS_TOKEN_CONDITIONAL:
                /* place ourselves in the right conditional case */
                update |= evaluate_conditional(gwps, &i);
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                /* we've finished in the curent conditional case,
                    skip to the end of the conditional structure */
                i = find_conditional_end(data, i);
                break;

#ifdef HAVE_LCD_BITMAP
            case WPS_TOKEN_IMAGE_PRELOAD_DISPLAY:
            {
                struct gui_img *img = data->img;
                int n = data->tokens[i].value.i & 0xFF;
                int subimage = data->tokens[i].value.i >> 8;

                if (n >= 0 && n < MAX_IMAGES && img[n].loaded)
                    img[n].display = subimage;
                break;
            }
#endif

            case WPS_TOKEN_ALIGN_LEFT:
            case WPS_TOKEN_ALIGN_CENTER:
            case WPS_TOKEN_ALIGN_RIGHT:
                /* remember where the current aligned text started */
                switch (cur_align)
                {
                    case WPS_ALIGN_LEFT:
                        align->left = cur_align_start;
                        break;

                    case WPS_ALIGN_CENTER:
                        align->center = cur_align_start;
                        break;

                    case WPS_ALIGN_RIGHT:
                        align->right = cur_align_start;
                        break;
                }
                /* start a new alignment */
                switch (data->tokens[i].type)
                {
                    case WPS_TOKEN_ALIGN_LEFT:
                        cur_align = WPS_ALIGN_LEFT;
                        break;
                    case WPS_TOKEN_ALIGN_CENTER:
                        cur_align = WPS_ALIGN_CENTER;
                        break;
                    case WPS_TOKEN_ALIGN_RIGHT:
                        cur_align = WPS_ALIGN_RIGHT;
                        break;
                    default:
                        break;
                }
                *buf++ = 0;
                cur_align_start = buf;
                break;
            case WPS_VIEWPORT_ENABLE:
            {
                char label = data->tokens[i].value.i;
                int j;
                char temp = VP_DRAW_HIDEABLE;
                for(j=0;j<data->num_viewports;j++)
                {
                    temp = VP_DRAW_HIDEABLE;
                    if ((data->viewports[j].hidden_flags&VP_DRAW_HIDEABLE) &&
                        (data->viewports[j].label == label))
                    {
                        if (data->viewports[j].hidden_flags&VP_DRAW_WASHIDDEN)
                            temp |= VP_DRAW_WASHIDDEN;
                        data->viewports[j].hidden_flags = temp;
                    }
                }
            }
                break;
            default:
            {
                /* get the value of the tag and copy it to the buffer */
                const char *value = get_token_value(gwps, &data->tokens[i],
                                              temp_buf, sizeof(temp_buf), NULL);
                if (value)
                {
                    update = true;
                    while (*value && (buf < linebuf_end))
                        *buf++ = *value++;
                }
                break;
            }
        }
    }

    /* close the current alignment */
    switch (cur_align)
    {
        case WPS_ALIGN_LEFT:
            align->left = cur_align_start;
            break;

        case WPS_ALIGN_CENTER:
            align->center = cur_align_start;
            break;

        case WPS_ALIGN_RIGHT:
            align->right = cur_align_start;
            break;
    }

    return update;
}

static void get_subline_timeout(struct gui_wps *gwps, int line, int subline)
{
    struct wps_data *data = gwps->data;
    int i;
    int subline_idx = wps_subline_index(data, line, subline);
    int last_token_idx = wps_last_token_index(data, line, subline);

    data->sublines[subline_idx].time_mult = DEFAULT_SUBLINE_TIME_MULTIPLIER;

    for (i = wps_first_token_index(data, line, subline);
         i <= last_token_idx; i++)
    {
        switch(data->tokens[i].type)
        {
            case WPS_TOKEN_CONDITIONAL:
                /* place ourselves in the right conditional case */
                evaluate_conditional(gwps, &i);
                break;

            case WPS_TOKEN_CONDITIONAL_OPTION:
                /* we've finished in the curent conditional case,
                    skip to the end of the conditional structure */
                i = find_conditional_end(data, i);
                break;

            case WPS_TOKEN_SUBLINE_TIMEOUT:
                data->sublines[subline_idx].time_mult = data->tokens[i].value.i;
                break;

            default:
                break;
        }
    }
}

/* Calculates which subline should be displayed for the specified line
   Returns true iff the subline must be refreshed */
static bool update_curr_subline(struct gui_wps *gwps, int line)
{
    struct wps_data *data = gwps->data;

    int search, search_start, num_sublines;
    bool reset_subline;
    bool new_subline_refresh;
    bool only_one_subline;

    num_sublines = data->lines[line].num_sublines;
    reset_subline = (data->lines[line].curr_subline == SUBLINE_RESET);
    new_subline_refresh = false;
    only_one_subline = false;

    /* if time to advance to next sub-line  */
    if (TIME_AFTER(current_tick, data->lines[line].subline_expire_time - 1) ||
        reset_subline)
    {
        /* search all sublines until the next subline with time > 0
            is found or we get back to the subline we started with */
        if (reset_subline)
            search_start = 0;
        else
            search_start = data->lines[line].curr_subline;

        for (search = 0; search < num_sublines; search++)
        {
            data->lines[line].curr_subline++;

            /* wrap around if beyond last defined subline or WPS_MAX_SUBLINES */
            if (data->lines[line].curr_subline == num_sublines)
            {
                if (data->lines[line].curr_subline == 1)
                    only_one_subline = true;
                data->lines[line].curr_subline = 0;
            }

            /* if back where we started after search or
                only one subline is defined on the line */
            if (((search > 0) &&
                 (data->lines[line].curr_subline == search_start)) ||
                only_one_subline)
            {
                /* no other subline with a time > 0 exists */
                data->lines[line].subline_expire_time = (reset_subline ?
                    current_tick :
                    data->lines[line].subline_expire_time) + 100 * HZ;
                break;
            }
            else
            {
                /* get initial time multiplier for this subline */
                get_subline_timeout(gwps, line, data->lines[line].curr_subline);

                int subline_idx = wps_subline_index(data, line,
                                               data->lines[line].curr_subline);

                /* only use this subline if subline time > 0 */
                if (data->sublines[subline_idx].time_mult > 0)
                {
                    new_subline_refresh = true;
                    data->lines[line].subline_expire_time = (reset_subline ?
                        current_tick : data->lines[line].subline_expire_time) +
                        TIMEOUT_UNIT*data->sublines[subline_idx].time_mult;
                    break;
                }
            }
        }
    }

    return new_subline_refresh;
}

/* Display a line appropriately according to its alignment format.
   format_align contains the text, separated between left, center and right.
   line is the index of the line on the screen.
   scroll indicates whether the line is a scrolling one or not.
*/
static void write_line(struct screen *display,
                       struct align_pos *format_align,
                       int line,
                       bool scroll)
{

    int left_width = 0, left_xpos;
    int center_width = 0, center_xpos;
    int right_width = 0,  right_xpos;
    int ypos;
    int space_width;
    int string_height;
    int scroll_width;

    /* calculate different string sizes and positions */
    display->getstringsize((unsigned char *)" ", &space_width, &string_height);
    if (format_align->left != 0) {
        display->getstringsize((unsigned char *)format_align->left,
                                &left_width, &string_height);
    }

    if (format_align->right != 0) {
        display->getstringsize((unsigned char *)format_align->right,
                                &right_width, &string_height);
    }

    if (format_align->center != 0) {
        display->getstringsize((unsigned char *)format_align->center,
                                &center_width, &string_height);
    }

    left_xpos = 0;
    right_xpos = (display->getwidth() - right_width);
    center_xpos = (display->getwidth() + left_xpos - center_width) / 2;

    scroll_width = display->getwidth() - left_xpos;

    /* Checks for overlapping strings.
        If needed the overlapping strings will be merged, separated by a
        space */

    /* CASE 1: left and centered string overlap */
    /* there is a left string, need to merge left and center */
    if ((left_width != 0 && center_width != 0) &&
        (left_xpos + left_width + space_width > center_xpos)) {
        /* replace the former separator '\0' of left and
            center string with a space */
        *(--format_align->center) = ' ';
        /* calculate the new width and position of the merged string */
        left_width = left_width + space_width + center_width;
        /* there is no centered string anymore */
        center_width = 0;
    }
    /* there is no left string, move center to left */
    if ((left_width == 0 && center_width != 0) &&
        (left_xpos + left_width > center_xpos)) {
        /* move the center string to the left string */
        format_align->left = format_align->center;
        /* calculate the new width and position of the string */
        left_width = center_width;
        /* there is no centered string anymore */
        center_width = 0;
    }

    /* CASE 2: centered and right string overlap */
    /* there is a right string, need to merge center and right */
    if ((center_width != 0 && right_width != 0) &&
        (center_xpos + center_width + space_width > right_xpos)) {
        /* replace the former separator '\0' of center and
            right string with a space */
        *(--format_align->right) = ' ';
        /* move the center string to the right after merge */
        format_align->right = format_align->center;
        /* calculate the new width and position of the merged string */
        right_width = center_width + space_width + right_width;
        right_xpos = (display->getwidth() - right_width);
        /* there is no centered string anymore */
        center_width = 0;
    }
    /* there is no right string, move center to right */
    if ((center_width != 0 && right_width == 0) &&
        (center_xpos + center_width > right_xpos)) {
        /* move the center string to the right string */
        format_align->right = format_align->center;
        /* calculate the new width and position of the string */
        right_width = center_width;
        right_xpos = (display->getwidth() - right_width);
        /* there is no centered string anymore */
        center_width = 0;
    }

    /* CASE 3: left and right overlap
        There is no center string anymore, either there never
        was one or it has been merged in case 1 or 2 */
    /* there is a left string, need to merge left and right */
    if ((left_width != 0 && center_width == 0 && right_width != 0) &&
        (left_xpos + left_width + space_width > right_xpos)) {
        /* replace the former separator '\0' of left and
            right string with a space */
        *(--format_align->right) = ' ';
        /* calculate the new width and position of the string */
        left_width = left_width + space_width + right_width;
        /* there is no right string anymore */
        right_width = 0;
    }
    /* there is no left string, move right to left */
    if ((left_width == 0 && center_width == 0 && right_width != 0) &&
        (left_width > right_xpos)) {
        /* move the right string to the left string */
        format_align->left = format_align->right;
        /* calculate the new width and position of the string */
        left_width = right_width;
        /* there is no right string anymore */
        right_width = 0;
    }

    ypos = (line * string_height);


    if (scroll && ((left_width > scroll_width) || 
                   (center_width > scroll_width) ||
                   (right_width > scroll_width)))
    {
        display->puts_scroll(0, line,
                             (unsigned char *)format_align->left);
    }
    else
    {
#ifdef HAVE_LCD_BITMAP
        /* clear the line first */
        display->set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        display->fillrect(left_xpos, ypos, display->getwidth(), string_height);
        display->set_drawmode(DRMODE_SOLID);
#endif

        /* Nasty hack: we output an empty scrolling string,
        which will reset the scroller for that line */
        display->puts_scroll(0, line, (unsigned char *)"");

        /* print aligned strings */
        if (left_width != 0)
        {
            display->putsxy(left_xpos, ypos,
                            (unsigned char *)format_align->left);
        }
        if (center_width != 0)
        {
            display->putsxy(center_xpos, ypos,
                            (unsigned char *)format_align->center);
        }
        if (right_width != 0)
        {
            display->putsxy(right_xpos, ypos,
                            (unsigned char *)format_align->right);
        }
    }
}

/* Refresh the WPS according to refresh_mode. */
bool gui_wps_refresh(struct gui_wps *gwps,
                     int ffwd_offset,
                     unsigned char refresh_mode)
{
    struct wps_data *data = gwps->data;
    struct screen *display = gwps->display;
    struct wps_state *state = gwps->state;

    if(!gwps || !data || !state || !display)
        return false;

    int v, line, i, subline_idx;
    unsigned char flags;
    char linebuf[MAX_PATH];
    unsigned char vp_refresh_mode;

    struct align_pos align;
    align.left = NULL;
    align.center = NULL;
    align.right = NULL;

    bool update_line, new_subline_refresh;

#ifdef HAVE_LCD_BITMAP
    gui_wps_statusbar_draw(gwps, true);

    /* to find out wether the peak meter is enabled we
       assume it wasn't until we find a line that contains
       the peak meter. We can't use peak_meter_enabled itself
       because that would mean to turn off the meter thread
       temporarily. (That shouldn't matter unless yield
       or sleep is called but who knows...)
    */
    bool enable_pm = false;

#endif

    /* reset to first subline if refresh all flag is set */
    if (refresh_mode == WPS_REFRESH_ALL)
    {
        display->set_viewport(&data->viewports[0].vp);
        display->clear_viewport();

        for (i = 0; i <= data->num_lines; i++)
        {
            data->lines[i].curr_subline = SUBLINE_RESET;
        }
    }

#ifdef HAVE_LCD_CHARCELLS
    for (i = 0; i < 8; i++)
    {
       if (data->wps_progress_pat[i] == 0)
           data->wps_progress_pat[i] = display->get_locked_pattern();
    }
#endif

    if (!state->id3)
    {
        display->stop_scroll();
        return false;
    }

    state->ff_rewind_count = ffwd_offset;

    /* disable any viewports which are conditionally displayed */
    for (v = 0; v < data->num_viewports; v++)
    {
        if (data->viewports[v].hidden_flags&VP_DRAW_HIDEABLE)
        {
            if (data->viewports[v].hidden_flags&VP_DRAW_HIDDEN)
                data->viewports[v].hidden_flags |= VP_DRAW_WASHIDDEN;
            else
                data->viewports[v].hidden_flags |= VP_DRAW_HIDDEN;
        }
    }
    for (v = 0; v < data->num_viewports; v++)
    {
        display->set_viewport(&data->viewports[v].vp);
        vp_refresh_mode = refresh_mode;

#ifdef HAVE_LCD_BITMAP
        /* Set images to not to be displayed */
        for (i = 0; i < MAX_IMAGES; i++)
        {
            data->img[i].display = -1;
        }
#endif
        /* dont redraw the viewport if its disabled */
        if ((data->viewports[v].hidden_flags&VP_DRAW_HIDDEN))
        {
            if (!(data->viewports[v].hidden_flags&VP_DRAW_WASHIDDEN))
                display->scroll_stop(&data->viewports[v].vp);
            data->viewports[v].hidden_flags |= VP_DRAW_WASHIDDEN;
            continue;
        }
        else if (((data->viewports[v].hidden_flags&
                   (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE))
                    == (VP_DRAW_WASHIDDEN|VP_DRAW_HIDEABLE)))
        {
            vp_refresh_mode = WPS_REFRESH_ALL;
            data->viewports[v].hidden_flags = VP_DRAW_HIDEABLE;
        }
        if (vp_refresh_mode == WPS_REFRESH_ALL)
        {
            display->clear_viewport();
        }
            
        for (line = data->viewports[v].first_line; 
             line <= data->viewports[v].last_line; line++)
        {
            memset(linebuf, 0, sizeof(linebuf));
            update_line = false;

            /* get current subline for the line */
            new_subline_refresh = update_curr_subline(gwps, line);

            subline_idx = wps_subline_index(data, line,
                                            data->lines[line].curr_subline);
            flags = data->sublines[subline_idx].line_type;

            if (vp_refresh_mode == WPS_REFRESH_ALL || (flags & vp_refresh_mode)
                || new_subline_refresh)
            {
                /* get_line tells us if we need to update the line */
                update_line = get_line(gwps, line, data->lines[line].curr_subline,
                                       &align, linebuf, sizeof(linebuf));
            }
#ifdef HAVE_LCD_BITMAP
            /* peakmeter */
            if (flags & vp_refresh_mode & WPS_REFRESH_PEAK_METER)
            {
                /* the peakmeter should be alone on its line */
                update_line = false;

                int h = font_get(data->viewports[v].vp.font)->height;
                int peak_meter_y = (line - data->viewports[v].first_line)* h;

                /* The user might decide to have the peak meter in the last
                    line so that it is only displayed if no status bar is
                    visible. If so we neither want do draw nor enable the
                    peak meter. */
                if (peak_meter_y + h <= display->getheight()) {
                    /* found a line with a peak meter -> remember that we must
                        enable it later */
                    enable_pm = true;
                    peak_meter_enabled = true;
                    peak_meter_screen(gwps->display, 0, peak_meter_y,
                                      MIN(h, display->getheight() - peak_meter_y));
                }
                else
                {
                    peak_meter_enabled = false;
                }
            }

#else /* HAVE_LCD_CHARCELL */

            /* progressbar */
            if (flags & vp_refresh_mode & WPS_REFRESH_PLAYER_PROGRESS)
            {
                if (data->full_line_progressbar)
                    draw_player_fullbar(gwps, linebuf, sizeof(linebuf));
                else
                    draw_player_progress(gwps);
            }
#endif

            if (update_line && 
                /* conditionals clear the line which means if the %Vd is put into the default
                   viewport there will be a blank line.
                   To get around this we dont allow any actual drawing to happen in the
                   deault vp if other vp's are defined */
                ((data->num_viewports>1 && v!=0) || data->num_viewports == 1))
            {
                if (flags & WPS_REFRESH_SCROLL)
                {
                    /* if the line is a scrolling one we don't want to update
                       too often, so that it has the time to scroll */
                    if ((vp_refresh_mode & WPS_REFRESH_SCROLL) || new_subline_refresh)
                        write_line(display, &align, line - data->viewports[v].first_line, true);
                }
                else
                    write_line(display, &align, line - data->viewports[v].first_line, false);
            }
        }

#ifdef HAVE_LCD_BITMAP
        /* progressbar */
        if (vp_refresh_mode & WPS_REFRESH_PLAYER_PROGRESS)
        {
            if (data->viewports[v].pb)
                draw_progressbar(gwps, data->viewports[v].pb);
        }
        /* Now display any images in this viewport */
        wps_display_images(gwps, &data->viewports[v].vp);
#endif
    }

#ifdef HAVE_LCD_BITMAP
    data->peak_meter_enabled = enable_pm;
#endif

    /* Restore the default viewport */
    display->set_viewport(NULL);

    display->update();

#ifdef HAVE_BACKLIGHT
    if (global_settings.caption_backlight && state->id3)
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
    if (global_settings.remote_caption_backlight && state->id3)
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

    return true;
}


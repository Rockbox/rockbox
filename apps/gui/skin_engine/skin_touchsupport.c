/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 - Jonathan Gordon
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

#include "config.h"
#include <stdio.h>
#include "action.h"
#include "skin_engine.h"
#include "wps_internals.h"
#include "misc.h"
#include "option_select.h"
#include "sound.h"
#include "settings_list.h"
#include "wps.h"
#include "lang.h"
#include "splash.h"
#include "playlist.h"
#include "dsp_misc.h"
#include "playback.h"

/** Disarms all touchregions. */
void skin_disarm_touchregions(struct gui_wps *gwps)
{
    gesture_reset(&gwps->data->gesture);
}

/* Get the touched action.
 * egde_offset is a percentage value for the position of the touch
 * inside the bar for regions which arnt WPS_TOUCHREGION_ACTION type.
 */
int skin_get_touchaction(struct gui_wps *gwps, int* edge_offset)
{
    struct wps_data *data = gwps->data;
    char* skin_buffer = get_skin_buffer(data);
    struct touchregion *region = NULL;
    struct touchevent tevent;
    struct gesture_event gevent;

    action_get_touch_event(&tevent);
    gesture_process(&data->gesture, &tevent);

    struct skin_token_list *regions = SKINOFFSETTOPTR(skin_buffer, data->touchregions);
    for (; regions && !region;
         regions = SKINOFFSETTOPTR(skin_buffer, regions->next))
    {
        struct wps_token *token = SKINOFFSETTOPTR(skin_buffer, regions->token);
        struct touchregion *r = SKINOFFSETTOPTR(skin_buffer, token->value.data);
        struct skin_viewport *wvp = SKINOFFSETTOPTR(skin_buffer, r->wvp);

        /* make sure this region's viewport is visible */
        if (wvp->hidden_flags & VP_DRAW_HIDDEN)
            continue;

        /* unless it's allow_while_locked, ignore the region if locked
         * (this is a special skin engine lock, different from softlock) */
        if (data->touchscreen_locked &&
            (r->action != ACTION_TOUCH_SOFTLOCK && !r->allow_while_locked))
            continue;

        /* check for a gesture inside the region's parent viewport */
        if (!gesture_get_event_in_vp(&data->gesture, &gevent, &wvp->vp))
            continue;

        /* project touches inside the padding box to the region edge */
        if (r->x - r->wpad <= gevent.ox && gevent.ox < r->x)
            gevent.ox = r->x;
        else if (r->x + r->width <= gevent.ox && gevent.ox < r->x + r->width + r->wpad)
            gevent.ox = r->x + r->width - 1;

        if (r->y - r->hpad <= gevent.y && gevent.y < r->y)
            gevent.oy = r->y;
        else if (r->y + r->height <= gevent.oy && gevent.oy < r->y + r->height + r->hpad)
            gevent.oy = r->y + r->height - 1;

        /* ignore anything outside of the region */
        if (gevent.ox < r->x || gevent.ox >= r->x + r->width)
            continue;
        if (gevent.oy < r->y || gevent.oy >= r->y + r->height)
            continue;

        /* convert coordinates to region-relative and clamp */
        gevent.x -= r->x;
        gevent.y -= r->y;

        switch (r->action)
        {
        case ACTION_TOUCH_SCROLLBAR:
        case ACTION_TOUCH_VOLUME:
        case ACTION_TOUCH_SETTING:
            /* accept taps and drags, ignore the rest */
            if (!(gevent.id == GESTURE_TAP ||
                  gevent.id == GESTURE_HOLD ||
                  gevent.id == GESTURE_DRAGSTART ||
                  gevent.id == GESTURE_DRAG ||
                  gevent.id == GESTURE_RELEASE))
                break;

            if (edge_offset)
            {
                struct progressbar *bar = SKINOFFSETTOPTR(skin_buffer, r->bar);
                bool reverse = r->reverse_bar || (bar && bar->invert_fill_direction);
                int pos, lim;

                if (r->width > r->height) {
                    /* left to right by default */
                    pos = MIN(MAX(0, gevent.x), r->width - 1);
                    lim = r->width;
                } else {
                    /* bottom up by default, so we need to add a reversal */
                    pos = MIN(MAX(0, gevent.y), r->height - 1);
                    lim = r->height;
                    reverse = !reverse;
                }

                if (lim > 1)
                    *edge_offset = pos * 1000 / (lim - 1);
                else
                    *edge_offset = 0;

                if (reverse)
                    *edge_offset = 1000 - *edge_offset;
            }

            region = r;
            break;

        default:
            if (r->press_length == PRESS)
            {
                if (gevent.id != GESTURE_TAP)
                    break;
            }
            else if (r->press_length == LONG_PRESS)
            {
                if (gevent.id != GESTURE_LONG_PRESS)
                    break;
            }
            else /* REPEAT */
            {
                /* for repeat regions we allow dragging inside the region */
                if (gevent.id != GESTURE_HOLD &&
                    gevent.id != GESTURE_DRAGSTART &&
                    gevent.id != GESTURE_DRAG)
                    break;

                if (gevent.x < 0 || gevent.x >= r->width ||
                    gevent.y < 0 || gevent.y >= r->height)
                    break;
            }

            /* gesture is OK */
            region = r;
            break;
        }
    }

    /* no region - pass the event upward */
    if (!region)
        return ACTION_TOUCHSCREEN;

    int action = region->action;
    region->last_press = tevent.tick;

    if (global_settings.party_mode)
    {
        switch (action)
        {
        case ACTION_WPS_PLAY:
        case ACTION_WPS_SKIPPREV:
        case ACTION_WPS_SKIPNEXT:
        case ACTION_WPS_STOP:
            action = ACTION_NONE;
            break;
        default:
            break;
        }
    }

    switch (action)
    {
    case ACTION_TOUCH_SCROLLBAR:
    {
        action = ACTION_TOUCHSCREEN;

        struct wps_state* gwps = get_wps_state();
        if (!gwps->id3)
            break;

        if (gevent.id == GESTURE_HOLD ||
            gevent.id == GESTURE_DRAGSTART ||
            gevent.id == GESTURE_DRAG)
        {
            audio_pre_ff_rewind();
            gwps->id3->elapsed = gwps->id3->length * (*edge_offset) / 1000;
        }
        else if (gevent.id == GESTURE_RELEASE)
        {
            audio_ff_rewind(gwps->id3->elapsed);
        }
        else
        {
            gwps->id3->elapsed = gwps->id3->length * (*edge_offset) / 1000;
            audio_pre_ff_rewind();
            audio_ff_rewind(gwps->id3->elapsed);
        }

    } break;

    case ACTION_TOUCH_VOLUME:
    {
        const int min_vol = sound_min(SOUND_VOLUME);
        const int max_vol = sound_max(SOUND_VOLUME);
        const int step_vol = sound_steps(SOUND_VOLUME);

        global_status.volume = from_normalized_volume(*edge_offset, min_vol, max_vol, 1000);
        global_status.volume -= (global_status.volume % step_vol);
        setvol();

        action = ACTION_TOUCHSCREEN;
    } break;

    case ACTION_TOUCH_SOFTLOCK:
        data->touchscreen_locked = !data->touchscreen_locked;
        action = ACTION_NONE;
        break;

    case ACTION_WPS_PLAY:
        if (!audio_status())
        {
            if ( global_status.resume_index != -1 )
            {
                if (playlist_resume() != -1)
                {
                    playlist_start(global_status.resume_index,
                                   global_status.resume_elapsed,
                                   global_status.resume_offset);
                }
            }
            else
            {
                splash(HZ*2, ID2P(LANG_NOTHING_TO_RESUME));
            }
        }
        else
        {
            wps_do_playpause(false);
        }

        action = ACTION_REDRAW;
        break;

    case ACTION_WPS_SKIPPREV:
        audio_prev();
        action = ACTION_REDRAW;
        break;

    case ACTION_WPS_SKIPNEXT:
        audio_next();
        action = ACTION_REDRAW;
        break;

    case ACTION_WPS_STOP:
        audio_stop();
        action = ACTION_REDRAW;
        break;

    case ACTION_SETTINGS_INC:
    case ACTION_SETTINGS_DEC:
    {
        const struct settings_list *setting = region->setting_data.setting;
        bool decrement = (action == ACTION_SETTINGS_DEC);
        option_select_next_val(setting, decrement, true);
        action = ACTION_REDRAW;
    } break;

    case ACTION_SETTINGS_SET:
    {
        struct touchsetting *data = &region->setting_data;
        const struct settings_list *s = data->setting;
        void (*f)(int) = NULL;
        switch (s->flags & F_T_MASK)
        {
        case F_T_CUSTOM:
            s->custom_setting
                ->load_from_cfg(s->setting, SKINOFFSETTOPTR(skin_buffer, data->value.text));
            break;
        case F_T_INT:
        case F_T_UINT:
            *(int*)s->setting = data->value.number;
            if ((s->flags & F_T_SOUND) == F_T_SOUND)
                sound_set(s->sound_setting->setting, data->value.number);
            else if (s->flags&F_CHOICE_SETTING)
                f = s->choice_setting->option_callback;
            else if (s->flags&F_TABLE_SETTING)
                f = s->table_setting->option_callback;
            else
                f = s->int_setting->option_callback;

            if (f)
                f(data->value.number);
            break;
        case F_T_BOOL:
            *(bool*)s->setting = data->value.number ? true : false;
            if (s->bool_setting->option_callback)
                s->bool_setting
                    ->option_callback(data->value.number ? true : false);
            break;
        }

        action = ACTION_REDRAW;
    } break;

    case ACTION_TOUCH_MUTE:
    {
        const int min_vol = sound_min(SOUND_VOLUME);
        if (global_status.volume == min_vol)
            global_status.volume = region->value;
        else
        {
            region->value = global_status.volume;
            global_status.volume = min_vol;
        }

        setvol();
        action = ACTION_REDRAW;
    } break;

    case ACTION_TOUCH_SHUFFLE:
        global_settings.playlist_shuffle = !global_settings.playlist_shuffle;
        replaygain_update();
        if (global_settings.playlist_shuffle)
            playlist_randomise(NULL, current_tick, true);
        else
            playlist_sort(NULL, true);

        action = ACTION_REDRAW;
        break;

    case ACTION_TOUCH_REPMODE:
    {
        const struct settings_list *rep_setting =
            find_setting(&global_settings.repeat_mode);
        option_select_next_val(rep_setting, false, true);
        audio_flush_and_reload_tracks();
        action = ACTION_REDRAW;
    } break;

    case ACTION_TOUCH_SETTING:
    {
        struct progressbar *bar = SKINOFFSETTOPTR(skin_buffer, region->bar);
        if (bar && edge_offset)
        {
            int val, count;
            get_setting_info_for_bar(bar->setting, 0, &count, &val);
            val = *edge_offset * count / 1000;
            update_setting_value_from_touch(bar->setting, 0, val);
        }

        action = ACTION_NONE;
    } break;
    }

    return action;
}

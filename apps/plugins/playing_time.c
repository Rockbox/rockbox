/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
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

#include "plugin.h"

#define rb_talk_ids(enqueue, ids...) rb->talk_idarray(TALK_IDARRAY(ids), enqueue)

/* units used with output_dyn_value */
const unsigned char * const byte_units[] =
{
    ID2P(LANG_BYTE),
    ID2P(LANG_KIBIBYTE),
    ID2P(LANG_MEBIBYTE),
    ID2P(LANG_GIBIBYTE)
};

const int menu_items[] = {
        LANG_PLAYTIME_ELAPSED,
        LANG_PLAYTIME_REMAINING,
        LANG_PLAYTIME_TRK_ELAPSED,
        LANG_PLAYTIME_TRK_REMAINING,
        LANG_PLAYTIME_TRACK,
        LANG_PLAYTIME_STORAGE,
        LANG_PLAYTIME_AVG_TRACK_SIZE,
        LANG_PLAYTIME_AVG_BITRATE,
};

const unsigned char * const * const kibyte_units = &byte_units[1];

enum ePT_SECS {
    ePT_SECS_TTL = 0,
    ePT_SECS_BEF,
    ePT_SECS_AFT,
    ePT_SECS_COUNT
};

enum ePT_KBS {
    /* Note: Order matters (voicing order of LANG_PLAYTIME_STORAGE) */
    ePT_KBS_TTL = 0,
    ePT_KBS_BEF,
    ePT_KBS_AFT,
    ePT_KBS_COUNT
};

/* playing_time screen context */
struct playing_time_info {
    int curr_index;         /* index of currently playing track in playlist */
    int curr_display_index; /* display index of currently playing track in playlist */
    int nb_tracks;          /* how many tracks in playlist */

    /* seconds total, before, and after current position.  Datatype
       allows for values up to 68years.  If I had kept it in ms
       though, it would have overflowed at 24days, which takes
       something like 8.5GB at 32kbps, and so we could conceivably
       have playlists lasting longer than that. */
    long secs[ePT_SECS_COUNT];
    long trk_secs[ePT_SECS_COUNT];

    /* kilobytes played total, before, and after current pos.
       Kilobytes because bytes would overflow. Data type range is up
       to 2TB. */
    long kbs[ePT_KBS_COUNT];
};

static char* get_percent_str(long percents)
{
    static char val[10];
    rb->snprintf(val, 10, rb->str(LANG_PERCENT_FORMAT), percents);
    return val;
}

static inline void prepare_time_string(char *buf, size_t buffer_len, long elapsed_pct, const char *timestr1, const char *timestr2)
{
    if (rb->lang_is_rtl())
    {
        rb->snprintf(buf, buffer_len, "%s %s / %s",
                get_percent_str(elapsed_pct), timestr2, timestr1);
    }
    else
    {
        rb->snprintf(buf, buffer_len, "%s / %s %s",
                timestr1, timestr2, get_percent_str(elapsed_pct));
    }
}

/* list callback for playing_time screen */
static const char * playing_time_get_or_speak_info(int selected_item, void * data,
                                                   char *buf, size_t buffer_len,
                                                   bool say_it)
{
    long elapsed_pct; /* percentage of duration elapsed */
    struct playing_time_info *pti = (struct playing_time_info *)data;
    int info_no = selected_item/2;
    const int menu_name_id = menu_items[info_no];

    if (!(selected_item%2))
    {/* header */
        return rb->str(menu_name_id);
    }

    switch(info_no) {
    case 0: { /* elapsed and total time */
        char timestr1[25], timestr2[25];
        rb->format_time_auto(timestr1, sizeof(timestr1),
                         pti->secs[ePT_SECS_BEF], UNIT_SEC, true);

        rb->format_time_auto(timestr2, sizeof(timestr2),
                         pti->secs[ePT_SECS_TTL], UNIT_SEC, true);

        if (pti->secs[ePT_SECS_TTL] == 0)
            elapsed_pct = 0;
        else if (pti->secs[ePT_SECS_TTL] <= 0xFFFFFF)
        {
            elapsed_pct = (pti->secs[ePT_SECS_BEF] * 100
                            / pti->secs[ePT_SECS_TTL]);
        }
        else /* sacrifice some precision to avoid overflow */
        {
            elapsed_pct = (pti->secs[ePT_SECS_BEF] >> 7) * 100
                           / (pti->secs[ePT_SECS_TTL] >> 7);
        }
        prepare_time_string(buf, buffer_len, elapsed_pct, timestr1, timestr2);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                     TALK_ID(pti->secs[ePT_SECS_BEF], UNIT_TIME),
                     VOICE_OF,
                     TALK_ID(pti->secs[ePT_SECS_TTL], UNIT_TIME),
                     VOICE_PAUSE,
                     TALK_ID(elapsed_pct, UNIT_PERCENT));
        break;
    }
    case 1: { /* playlist remaining time */
        char timestr[25];
        rb->format_time_auto(timestr, sizeof(timestr), pti->secs[ePT_SECS_AFT],
            UNIT_SEC, false);
        rb->snprintf(buf, buffer_len, "%s", timestr);

        if (say_it)
          rb_talk_ids(false, menu_name_id,
                     TALK_ID(pti->secs[ePT_SECS_AFT], UNIT_TIME));
        break;
    }
    case 2: { /* track elapsed and duration */
        char timestr1[25], timestr2[25];

        rb->format_time_auto(timestr1, sizeof(timestr1), pti->trk_secs[ePT_SECS_BEF],
            UNIT_SEC, true);
        rb->format_time_auto(timestr2, sizeof(timestr2), pti->trk_secs[ePT_SECS_TTL],
            UNIT_SEC, true);

        if (pti->trk_secs[ePT_SECS_TTL] == 0)
            elapsed_pct = 0;
        else if (pti->trk_secs[ePT_SECS_TTL] <= 0xFFFFFF)
        {
            elapsed_pct = (pti->trk_secs[ePT_SECS_BEF] * 100
                           / pti->trk_secs[ePT_SECS_TTL]);
        }
        else /* sacrifice some precision to avoid overflow */
        {
            elapsed_pct = (pti->trk_secs[ePT_SECS_BEF] >> 7) * 100
                            / (pti->trk_secs[ePT_SECS_TTL] >> 7);
        }
        prepare_time_string(buf, buffer_len, elapsed_pct, timestr1, timestr2);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                     TALK_ID(pti->trk_secs[ePT_SECS_BEF], UNIT_TIME),
                     VOICE_OF,
                     TALK_ID(pti->trk_secs[ePT_SECS_TTL], UNIT_TIME),
                     VOICE_PAUSE,
                     TALK_ID(elapsed_pct, UNIT_PERCENT));
        break;
    }
    case 3: { /* track remaining time */
        char timestr[25];
        rb->format_time_auto(timestr, sizeof(timestr), pti->trk_secs[ePT_SECS_AFT],
            UNIT_SEC, false);
        rb->snprintf(buf, buffer_len, "%s", timestr);

        if (say_it)
          rb_talk_ids(false, menu_name_id,
                     TALK_ID(pti->trk_secs[ePT_SECS_AFT], UNIT_TIME));
        break;
    }
    case 4: { /* track index */
        int track_pct = pti->curr_display_index * 100 / pti->nb_tracks;

        if (rb->lang_is_rtl())
        {
            rb->snprintf(buf, buffer_len, "%s %d / %d",
                    get_percent_str(track_pct), pti->nb_tracks, pti->curr_display_index);
        }
        else
        {
            rb->snprintf(buf, buffer_len, "%d / %d %s",
                    pti->curr_display_index, pti->nb_tracks, get_percent_str(track_pct));
        }

        if (say_it)
          rb_talk_ids(false, menu_name_id,
                     TALK_ID(pti->curr_display_index, UNIT_INT),
                     VOICE_OF,
                     TALK_ID(pti->nb_tracks, UNIT_INT),
                     VOICE_PAUSE,
                     TALK_ID(track_pct, UNIT_PERCENT));
        break;
    }
    case 5: { /* storage size */
        int i;
        char kbstr[ePT_KBS_COUNT][20];

        for (i = 0; i < ePT_KBS_COUNT; i++) {
            rb->output_dyn_value(kbstr[i], sizeof(kbstr[i]),
                             pti->kbs[i], kibyte_units, 3, true);
        }
        rb->snprintf(buf, buffer_len, "%s (%s / %s)",
                 kbstr[ePT_KBS_TTL], kbstr[ePT_KBS_BEF],kbstr[ePT_KBS_AFT]);

        if (say_it) {
            int32_t voice_ids[ePT_KBS_COUNT];
            voice_ids[ePT_KBS_TTL] = menu_name_id;
            voice_ids[ePT_KBS_BEF] = VOICE_PLAYTIME_DONE;
            voice_ids[ePT_KBS_AFT] = LANG_PLAYTIME_REMAINING;

            for (i = 0; i < ePT_KBS_COUNT; i++) {
                rb_talk_ids(i > 0, VOICE_PAUSE, voice_ids[i]);
                rb->output_dyn_value(NULL, 0, pti->kbs[i], kibyte_units, 3, true);
            }
        }
        break;
    }
    case 6: { /* Average track file size */
        char str[20];
        long avg_track_size = pti->kbs[ePT_KBS_TTL] / pti->nb_tracks;
        rb->output_dyn_value(str, sizeof(str), avg_track_size, kibyte_units, 3, true);
        rb->snprintf(buf, buffer_len, "%s", str);

        if (say_it) {
            rb->talk_id(menu_name_id, false);
            rb->output_dyn_value(NULL, 0, avg_track_size, kibyte_units, 3, true);
        }
        break;
    }
    case 7: { /* Average bitrate */
        /* Convert power of 2 kilobytes to power of 10 kilobits */
        long avg_bitrate = (pti->kbs[ePT_KBS_TTL] / pti->secs[ePT_SECS_TTL]
                            * 1024 * 8 / 1000);
        rb->snprintf(buf, buffer_len, "%ld  kbps", avg_bitrate);

        if (say_it)
          rb_talk_ids(false, menu_name_id,
                   TALK_ID(avg_bitrate, UNIT_KBIT));
        break;
    }
    }
    return buf;
}

static const char * playing_time_get_info(int selected_item, void * data,
                                          char *buffer, size_t buffer_len)
{
    return playing_time_get_or_speak_info(selected_item, data,
                                          buffer, buffer_len, false);
}

static int playing_time_speak_info(int selected_item, void * data)
{
    static char buffer[MAX_PATH];
    playing_time_get_or_speak_info(selected_item, data,
                                   buffer, MAX_PATH, true);
    return 0;
}

/* playing time screen: shows total and elapsed playlist duration and
   other stats */
static bool playing_time(void)
{
    int error_count = 0;
    unsigned long talked_tick = *rb->current_tick;
    struct playing_time_info pti;
    struct playlist_track_info pltrack;
    struct mp3entry id3;
    int i, index;

    pti.nb_tracks = rb->playlist_amount();
    rb->playlist_get_resume_info(&pti.curr_index);
    struct mp3entry *curr_id3 = rb->audio_current_track();
    if (pti.curr_index == -1 || !curr_id3)
        return false;
    pti.curr_display_index = rb->playlist_get_display_index();

    pti.secs[ePT_SECS_BEF] = pti.trk_secs[ePT_SECS_BEF] = curr_id3->elapsed / 1000;
    pti.secs[ePT_SECS_AFT] = pti.trk_secs[ePT_SECS_AFT]
        = (curr_id3->length -curr_id3->elapsed) / 1000;
    pti.kbs[ePT_KBS_BEF] = curr_id3->offset / 1024;
    pti.kbs[ePT_KBS_AFT] = (curr_id3->filesize -curr_id3->offset) / 1024;

    rb->splash_progress_set_delay(HZ/2);

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif
    /* Go through each file in the playlist and get its stats. For
       huge playlists this can take a while... The reference position
       is the position at the moment this function was invoked,
       although playback continues forward. */
    index = rb->playlist_get_first_index(NULL);
    for (i = 0; i < pti.nb_tracks; i++, index++) {

        if (index == pti.nb_tracks)
            index = 0;

        /* Show a splash while we are loading. */
        rb->splash_progress(i, pti.nb_tracks, "%s (%s)",
                            rb->str(LANG_WAIT), rb->str(LANG_OFF_ABORT));

        /* Voice equivalent */
        if (TIME_AFTER(*rb->current_tick, talked_tick + 5 * HZ)) {
            talked_tick = *rb->current_tick;
            rb_talk_ids(false, LANG_LOADING_PERCENT,
                     TALK_ID(i * 100 / pti.nb_tracks, UNIT_PERCENT));
        }
        if (rb->action_userabort(TIMEOUT_NOBLOCK))
        {
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
            goto exit;
        }

        if (index == pti.curr_index)
            continue;

        if (rb->playlist_get_track_info(NULL, index, &pltrack) < 0
            || !rb->get_metadata(&id3, -1, pltrack.filename))
        {
            error_count++;
            continue;
        }

        if (pltrack.display_index < pti.curr_display_index) /* preceding tracks */
        {
            pti.secs[ePT_SECS_BEF] += id3.length / 1000;
            pti.kbs[ePT_KBS_BEF] += id3.filesize / 1024;
        }
        else
        {
            pti.secs[ePT_SECS_AFT] += id3.length / 1000;
            pti.kbs[ePT_KBS_AFT] += id3.filesize / 1024;
        }
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif

    if (error_count > 0)
    {
        rb->splash(HZ, ID2P(LANG_PLAYTIME_ERROR));
    }

    pti.nb_tracks -= error_count;
    pti.secs[ePT_SECS_TTL] = pti.secs[ePT_SECS_BEF] + pti.secs[ePT_SECS_AFT];
    pti.trk_secs[ePT_SECS_TTL] = pti.trk_secs[ePT_SECS_BEF] + pti.trk_secs[ePT_SECS_AFT];
    pti.kbs[ePT_KBS_TTL] = pti.kbs[ePT_KBS_BEF] + pti.kbs[ePT_KBS_AFT];

    struct gui_synclist pt_lists;
    int key;

    rb->gui_synclist_init(&pt_lists, &playing_time_get_info, &pti, true, 2, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&pt_lists, playing_time_speak_info);
    rb->gui_synclist_set_nb_items(&pt_lists, 16);
    rb->gui_synclist_set_title(&pt_lists, rb->str(LANG_PLAYING_TIME), NOICON);
    rb->gui_synclist_draw(&pt_lists);
    rb->gui_synclist_speak_item(&pt_lists);
    while (true) {
        key = rb->get_action(CONTEXT_LIST, HZ/2);
        if (rb->gui_synclist_do_button(&pt_lists, &key) == 0
           && key!=ACTION_NONE && key!=ACTION_UNKNOWN)
        {
            bool usb = rb->default_event_handler(key) == SYS_USB_CONNECTED;

            if (!usb && IS_SYSEVENT(key))
                continue;

            rb->talk_force_shutup();
            return usb;
        }

    }

 exit:
    return false;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    enum plugin_status status = PLUGIN_OK;

    (void)parameter;

    if (!rb->audio_status())
    {
        rb->splash(HZ*2, "Nothing Playing");
        return status;
    }

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_enable(i, true, NULL);

    if (playing_time())
        status = PLUGIN_USB_CONNECTED;

    FOR_NB_SCREENS(i)
        rb->viewportmanager_theme_undo(i, false);

    return status;
}

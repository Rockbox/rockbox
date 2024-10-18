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
        LANG_REMAINING,
        LANG_ELAPSED,
        LANG_PLAYTIME_TRK_REMAINING,
        LANG_PLAYTIME_TRK_ELAPSED,
        LANG_PLAYTIME_TRACK,
        LANG_PLAYTIME_STORAGE,
        LANG_PLAYTIME_AVG_TRACK_SIZE,
        LANG_PLAYTIME_AVG_BITRATE,
};

const unsigned char * const * const kibyte_units = &byte_units[1];

enum ePT_SUM {
    /* Note: Order matters (voicing order of LANG_PLAYTIME_STORAGE) */
    ePT_TOTAL = 0,
    ePT_ELAPSED,
    ePT_REMAINING,

    ePT_COUNT
};

struct playing_time_info {
    char single_mode_tag[MAX_PATH]; /* Relevant tag when single mode enabled */
    unsigned long long size[ePT_COUNT]; /*               File size of tracks */
    unsigned long long length[ePT_COUNT]; /*                Length of tracks */
    unsigned long curr_track_length[ePT_COUNT]; /*      Current track length */
    int curr_track_index;   /*  Index of currently playing track in playlist */
    int curr_display_index; /*      Display index of currently playing track */
    int actual_index; /*            Display index in actually counted tracks */
    int counted; /*                        Number of tracks already added up */
    int nb_tracks; /*                           Number of tracks in playlist */
    int error_count; /*    Number of tracks whose data couldn't be retrieved */
    bool remaining_only; /*                 Whether to ignore elapsed tracks */
};

static int32_t single_mode_lang(void)
{
    switch (rb->global_settings->single_mode)
    {
        case SINGLE_MODE_ALBUM:
            return LANG_ID3_ALBUM;
        case SINGLE_MODE_ALBUM_ARTIST:
            return LANG_ID3_ALBUMARTIST;
        case SINGLE_MODE_ARTIST:
            return LANG_ID3_ARTIST;
        case SINGLE_MODE_COMPOSER:
            return LANG_ID3_COMPOSER;
        case SINGLE_MODE_GROUPING:
            return LANG_ID3_GROUPING;
        case SINGLE_MODE_GENRE:
            return LANG_ID3_GENRE;
        case SINGLE_MODE_TRACK:
            return LANG_TRACK;
    }
    return LANG_OFF;
}

static char* single_mode_id3_tag(struct mp3entry *id3)
{
    switch (rb->global_settings->single_mode)
    {
        case SINGLE_MODE_ALBUM:
            return id3->album;
        case SINGLE_MODE_ALBUM_ARTIST:
            return id3->albumartist;
        case SINGLE_MODE_ARTIST:
            return id3->artist;
        case SINGLE_MODE_COMPOSER:
            return id3->composer;
        case SINGLE_MODE_GROUPING:
            return id3->grouping;
        case SINGLE_MODE_GENRE:
            return id3->genre_string;
    }
    return NULL;
}

static char* get_percent_str(long percents)
{
    static char val[10];
    rb->snprintf(val, sizeof(val), rb->str(LANG_PERCENT_FORMAT), percents);
    return val;
}

static inline void prepare_time_string(char *buf, size_t buffer_len,
                                       long elapsed_pct, const char *timestr1,
                                       const char *timestr2)
{
    if (rb->lang_is_rtl())
        rb->snprintf(buf, buffer_len, "%s %s / %s",
                     get_percent_str(elapsed_pct), timestr2, timestr1);
    else
        rb->snprintf(buf, buffer_len, "%s / %s %s",
                     timestr1, timestr2, get_percent_str(elapsed_pct));
}

/* list callback for playing_time screen */
static const char * pt_get_or_speak_info(int selected_item, void * data,
                                         char *buf, size_t buffer_len,
                                         bool say_it)
{
    long elapsed_pct; /* percentage of duration elapsed */
    struct playing_time_info *pti = (struct playing_time_info *)data;
    int info_no = selected_item/2;
    const int menu_name_id = menu_items[info_no];

    /* header */
    if (!say_it && !(selected_item % 2))
        return rb->str(menu_name_id);

    /* data */
    switch(info_no) {
    case 0: { /* playlist remaining time */
        char timestr[25];
        rb->format_time_auto(timestr, sizeof(timestr),
                             pti->length[ePT_REMAINING], UNIT_SEC, false);
        rb->snprintf(buf, buffer_len, "%s", timestr);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                        TALK_ID(pti->length[ePT_REMAINING], UNIT_TIME));
        break;
    }
    case 1: { /* elapsed and total time */
        char timestr1[25], timestr2[25];
        rb->format_time_auto(timestr1, sizeof(timestr1),
                             pti->length[ePT_ELAPSED], UNIT_SEC, true);

        rb->format_time_auto(timestr2, sizeof(timestr2),
                             pti->length[ePT_TOTAL], UNIT_SEC, true);

        if (pti->length[ePT_TOTAL] == 0)
            elapsed_pct = 0;
        else if (pti->length[ePT_TOTAL] <= 0xFFFFFF)
        {
            elapsed_pct = (pti->length[ePT_ELAPSED] * 100
                           / pti->length[ePT_TOTAL]);
        }
        else /* sacrifice some precision to avoid overflow */
        {
            elapsed_pct = (pti->length[ePT_ELAPSED] >> 7) * 100
                          / (pti->length[ePT_TOTAL] >> 7);
        }
        prepare_time_string(buf, buffer_len, elapsed_pct, timestr1, timestr2);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                        TALK_ID(pti->length[ePT_ELAPSED], UNIT_TIME),
                        VOICE_OF,
                        TALK_ID(pti->length[ePT_TOTAL], UNIT_TIME),
                        VOICE_PAUSE,
                        TALK_ID(elapsed_pct, UNIT_PERCENT));
        break;
    }
    case 2: { /* track remaining time */
        char timestr[25];
        rb->format_time_auto(timestr, sizeof(timestr),
                             pti->curr_track_length[ePT_REMAINING], UNIT_SEC, false);
        rb->snprintf(buf, buffer_len, "%s", timestr);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                        TALK_ID(pti->curr_track_length[ePT_REMAINING], UNIT_TIME));
        break;
    }
    case 3: { /* track elapsed and duration */
        char timestr1[25], timestr2[25];

        rb->format_time_auto(timestr1, sizeof(timestr1),
                             pti->curr_track_length[ePT_ELAPSED], UNIT_SEC, true);
        rb->format_time_auto(timestr2, sizeof(timestr2),
                             pti->curr_track_length[ePT_TOTAL], UNIT_SEC, true);

        if (pti->curr_track_length[ePT_TOTAL] == 0)
            elapsed_pct = 0;
        else if (pti->curr_track_length[ePT_TOTAL] <= 0xFFFFFF)
        {
            elapsed_pct = (pti->curr_track_length[ePT_ELAPSED] * 100
                           / pti->curr_track_length[ePT_TOTAL]);
        }
        else /* sacrifice some precision to avoid overflow */
        {
            elapsed_pct = (pti->curr_track_length[ePT_ELAPSED] >> 7) * 100
                          / (pti->curr_track_length[ePT_TOTAL] >> 7);
        }
        prepare_time_string(buf, buffer_len, elapsed_pct, timestr1, timestr2);

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                        TALK_ID(pti->curr_track_length[ePT_ELAPSED], UNIT_TIME),
                        VOICE_OF,
                        TALK_ID(pti->curr_track_length[ePT_TOTAL], UNIT_TIME),
                        VOICE_PAUSE,
                        TALK_ID(elapsed_pct, UNIT_PERCENT));
        break;
    }
    case 4: { /* track index */
        int track_pct = pti->actual_index * 100 / pti->counted;

        if (rb->lang_is_rtl())
            rb->snprintf(buf, buffer_len, "%s %d / %d", get_percent_str(track_pct),
                         pti->counted, pti->actual_index);
        else
            rb->snprintf(buf, buffer_len, "%d / %d %s", pti->actual_index,
                         pti->counted, get_percent_str(track_pct));

        if (say_it)
            rb_talk_ids(false, menu_name_id,
                        TALK_ID(pti->actual_index, UNIT_INT),
                        VOICE_OF,
                        TALK_ID(pti->counted, UNIT_INT),
                        VOICE_PAUSE,
                        TALK_ID(track_pct, UNIT_PERCENT));
        break;
    }
    case 5: { /* storage size */
        int i;
        char kbstr[ePT_COUNT][20];

        for (i = 0; i < ePT_COUNT; i++) {
            rb->output_dyn_value(kbstr[i], sizeof(kbstr[i]),
                                 pti->size[i], kibyte_units, 3, true);
        }
        rb->snprintf(buf, buffer_len, "%s (%s / %s)", kbstr[ePT_TOTAL],
                     kbstr[ePT_ELAPSED], kbstr[ePT_REMAINING]);

        if (say_it) {
            int32_t voice_ids[ePT_COUNT];
            voice_ids[ePT_TOTAL] = menu_name_id;
            voice_ids[ePT_ELAPSED] = VOICE_PLAYTIME_DONE;
            voice_ids[ePT_REMAINING] = LANG_REMAINING;

            for (i = 0; i < ePT_COUNT; i++)
            {
                rb_talk_ids(i > 0, VOICE_PAUSE, voice_ids[i]);
                rb->output_dyn_value(NULL, 0, pti->size[i], kibyte_units, 3, true);
            }
        }
        break;
    }
    case 6: { /* Average track file size */
        char str[20];
        long avg_track_size = pti->size[ePT_TOTAL] / pti->counted;
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
        long avg_bitrate = (pti->size[ePT_TOTAL] / pti->length[ePT_TOTAL]
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

static const char * pt_get_info(int selected_item, void * data,
                                char *buffer, size_t buffer_len)
{
    return pt_get_or_speak_info(selected_item, data,
                                buffer, buffer_len, false);
}

static int pt_speak_info(int selected_item, void * data)
{
    static char buffer[MAX_PATH];
    pt_get_or_speak_info(selected_item, data, buffer, sizeof(buffer), true);
    return 0;
}

static bool pt_display_stats(struct playing_time_info *pti)
{
    struct gui_synclist pt_lists;
    rb->gui_synclist_init(&pt_lists, &pt_get_info, pti, true, 2, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&pt_lists, pt_speak_info);
    rb->gui_synclist_set_nb_items(&pt_lists, pti->remaining_only ? 2 : 8*2);
    rb->gui_synclist_set_title(&pt_lists, *pti->single_mode_tag ?
                                          rb->str(single_mode_lang()) :
                                          rb->str(LANG_PLAYLIST), NOICON);
    rb->gui_synclist_draw(&pt_lists);
    rb->gui_synclist_speak_item(&pt_lists);
    while (true)
    {
        int action = rb->get_action(CONTEXT_LIST, HZ/2);
        if (rb->gui_synclist_do_button(&pt_lists, &action) == 0
            && action != ACTION_NONE && action != ACTION_UNKNOWN)
        {
            bool usb = rb->default_event_handler(action) == SYS_USB_CONNECTED;

            if (!usb && IS_SYSEVENT(action))
                continue;

            rb->talk_force_shutup();
            return usb;
        }
    }
    return false;
}

static const char *pt_options_name(int selected_item, void * data,
                                   char *buf, size_t buf_size)
{
    (void) data;
    (void) buf;
    (void) buf_size;
    return selected_item == 0 ? rb->str(LANG_ALL) :
           selected_item == 1 ? rb->str(LANG_REMAINING) :
           rb->str(single_mode_lang());
}

static int pt_options_speak(int selected_item, void * data)
{
    (void) data;
    rb->talk_id(selected_item == 0 ? LANG_ALL :
                selected_item == 1 ? LANG_REMAINING :
                single_mode_lang(), false);
    return 0;
}

static int pt_options(struct playing_time_info *pti)
{
    struct gui_synclist pt_options;
    rb->gui_synclist_init(&pt_options, &pt_options_name, NULL, true, 1, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&pt_options, pt_options_speak);
    rb->gui_synclist_set_nb_items(&pt_options, *pti->single_mode_tag ? 3 : 2);
    rb->gui_synclist_set_title(&pt_options, rb->str(LANG_PLAYING_TIME), NOICON);
    rb->gui_synclist_draw(&pt_options);
    rb->gui_synclist_speak_item(&pt_options);

    while(true)
    {
        int button = rb->get_action(CONTEXT_LIST, HZ);
        if (rb->gui_synclist_do_button(&pt_options, &button))
            continue;
        switch(button)
        {
            case ACTION_STD_OK:
            {
                int sel = rb->gui_synclist_get_sel_pos(&pt_options);
                if (sel < 2)
                    *pti->single_mode_tag = 0;
                if (sel == 1)
                    pti->remaining_only = true;
                return -1;
            }
            case ACTION_STD_CANCEL:
                return 0;
            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
        }
    }
}

static void pt_store_converted_totals(struct playing_time_info *pti)
{
    /* convert units from ms to s */
    pti->length[ePT_ELAPSED] /= 1000;
    pti->length[ePT_REMAINING] /= 1000;
    pti->curr_track_length[ePT_ELAPSED] /= 1000;
    pti->curr_track_length[ePT_REMAINING] /= 1000;
    /* convert units from Bytes to KiB  */
    pti->size[ePT_ELAPSED] >>= 10;
    pti->size[ePT_REMAINING] >>= 10;

    pti->length[ePT_TOTAL] = pti->length[ePT_ELAPSED] + pti->length[ePT_REMAINING];
    pti->curr_track_length[ePT_TOTAL] = pti->curr_track_length[ePT_ELAPSED]
                                       + pti->curr_track_length[ePT_REMAINING];
    pti->size[ePT_TOTAL] = pti->size[ePT_ELAPSED] + pti->size[ePT_REMAINING];
}

static int pt_add_track(int i, enum ePT_SUM section, struct playing_time_info *pti)
{
    struct mp3entry id3;
    struct playlist_track_info pl_track;
    static unsigned long talked_tick;
    int progress_total = pti->remaining_only ?
                         (pti->nb_tracks - pti->curr_display_index) + 1 :
                         pti->nb_tracks;

    rb->splash_progress(pti->counted, progress_total, "%s (%s)",
                        rb->str(LANG_WAIT), rb->str(LANG_OFF_ABORT));

    if (TIME_AFTER(*rb->current_tick, talked_tick + HZ*5))
    {
        talked_tick = *rb->current_tick;
        rb_talk_ids(false, LANG_LOADING_PERCENT,
                    TALK_ID(pti->counted * 100 / progress_total,
                    UNIT_PERCENT));
    }

    if (rb->action_userabort(TIMEOUT_NOBLOCK))
        return -1;
    else if (rb->playlist_get_track_info(NULL, i, &pl_track) < 0
             || !rb->get_metadata(&id3, -1, pl_track.filename))
    {
        pti->error_count++;
        return -2;
    }
    else if(*pti->single_mode_tag &&    /* single mode tag doesn't match */
            rb->strcmp(pti->single_mode_tag, single_mode_id3_tag(&id3) ?: ""))
        return 1;

    pti->length[section] += id3.length;
    pti->size[section] += id3.filesize;
    pti->counted++;
    return 0;
}

static bool pt_add_remaining(struct playing_time_info *pti)
{
    int display_index = pti->curr_display_index + 1;
    for (int i = pti->curr_track_index + 1; display_index <= pti->nb_tracks; i++, display_index++)
    {
        if (i == pti->nb_tracks)
            i = 0;

        int ret = pt_add_track(i, ePT_REMAINING, pti);
        if (ret == 1)
            break;
        else if (ret == -1)
            return false;
    }
    return true;
}

static bool pt_add_elapsed(struct playing_time_info *pti)
{
    int display_index = pti->curr_display_index - 1;
    for (int i = pti->curr_track_index - 1; display_index > 0; i--, display_index--)
    {
        if (i < 0)
            i = pti->nb_tracks - 1;

        int ret = pt_add_track(i, ePT_ELAPSED, pti);
        if (ret == 1)
            break;
        else if (ret == -1)
            return false;
        else if (ret == 0)
            pti->actual_index++;
    }
    return true;
}

static bool pt_add_curr_track(struct playing_time_info *pti)
{
    struct mp3entry *curr_id3 = rb->audio_current_track();
    rb->playlist_get_resume_info(&pti->curr_track_index);

    if (pti->curr_track_index == -1 || !curr_id3)
        return false;

    pti->curr_display_index = rb->playlist_get_display_index();
    pti->length[ePT_ELAPSED] = pti->curr_track_length[ePT_ELAPSED]
                             = curr_id3->elapsed;
    pti->length[ePT_REMAINING] = pti->curr_track_length[ePT_REMAINING]
                               = curr_id3->length - curr_id3->elapsed;
    pti->size[ePT_ELAPSED] = curr_id3->offset;
    pti->size[ePT_REMAINING] = curr_id3->filesize - curr_id3->offset;
    pti->actual_index = pti->counted = 1;
    rb->strlcpy(pti->single_mode_tag, single_mode_id3_tag(curr_id3) ?: "",
                sizeof(pti->single_mode_tag));
    return true;
}

/* playing time screen: shows total and elapsed playlist duration and
   other stats */
static bool playing_time(void)
{
    struct playing_time_info pti;
    rb->memset(&pti, 0, sizeof(struct playing_time_info));

    if (!pt_add_curr_track(&pti))
        return false;

    int opt = pt_options(&pti);
    if (opt > -1)
        return opt;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->splash_progress_set_delay(HZ/2);
    pti.nb_tracks = rb->playlist_amount();
    int success = (pti.remaining_only || pt_add_elapsed(&pti)) && pt_add_remaining(&pti);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    if (!success)
        return false;
    if (pti.error_count > 0)
        rb->splash(HZ, ID2P(LANG_PLAYTIME_ERROR));

    pt_store_converted_totals(&pti);
    return pt_display_stats(&pti);
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
    enum plugin_status status = PLUGIN_OK;

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

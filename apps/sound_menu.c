/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "mp3_playback.h"
#include "settings.h"
#include "statusbar.h"
#include "screens.h"
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "font.h"
#include "scrollbar.h"
#endif
#include "lang.h"
#include "sprintf.h"
#include "talk.h"
#include "misc.h"
#include "sound.h"
#ifdef HAVE_RECORDING
#include "audio.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#endif
#ifdef HAVE_RECORDING
#include "peakmeter.h"
#include "mas.h"
#endif
#include "splash.h"
#if CONFIG_CODEC == SWCODEC
#include "dsp.h"
#include "menus/eq_menu.h"
#include "pcmbuf.h"
#ifdef HAVE_RECORDING
#include "enc_config.h"
#endif
#include "general.h"
#endif
#include "action.h"

#ifdef HAVE_RECORDING
static bool recsource(void)
{
    int n_opts = AUDIO_NUM_SOURCES;

    static const struct opt_items names[AUDIO_NUM_SOURCES] = {
        [AUDIO_SRC_MIC]     = { STR(LANG_RECORDING_SRC_MIC) },
        [AUDIO_SRC_LINEIN]  = { STR(LANG_RECORDING_SRC_LINE) },
#ifdef HAVE_SPDIF_IN
        [AUDIO_SRC_SPDIF]   = { STR(LANG_RECORDING_SRC_DIGITAL) },
#endif
#ifdef HAVE_FMRADIO_IN
        [AUDIO_SRC_FMRADIO] = { STR(LANG_FM_RADIO) }
#endif
    };

    /* caveat: assumes it's the last item! */
#ifdef HAVE_FMRADIO_IN
    if (!radio_hardware_present())
        n_opts--;
#endif

    return set_option(str(LANG_RECORDING_SOURCE),
                      &global_settings.rec_source, INT, names,
                      n_opts, NULL );
}

#if CONFIG_CODEC == MAS3587F
static bool recquality(void)
{
    return set_int(str(LANG_RECORDING_QUALITY), "", UNIT_INT,
                   &global_settings.rec_quality, 
                   NULL, 1, 0, 7, NULL );
}

static bool receditable(void)
{
    return set_bool(str(LANG_RECORDING_EDITABLE),
                    &global_settings.rec_editable);
}
#endif /* CONFIG_CODEC == MAS3587F */

#if CONFIG_CODEC == SWCODEC
/* Makes an options list from a source list of options and indexes */
static void make_options_from_indexes(const struct opt_items *src_names,
                                      const long *src_indexes,
                                      int n_indexes,
                                      struct opt_items *dst_names)
{
    while (--n_indexes >= 0)
        dst_names[n_indexes] = src_names[src_indexes[n_indexes]];
} /* make_options_from_indexes */

static bool recformat(void)
{
    static const struct opt_items names[REC_NUM_FORMATS] = {
        [REC_FORMAT_AIFF]    = { STR(LANG_AFMT_AIFF)    },
        [REC_FORMAT_MPA_L3]  = { STR(LANG_AFMT_MPA_L3)  },
        [REC_FORMAT_WAVPACK] = { STR(LANG_AFMT_WAVPACK) },
        [REC_FORMAT_PCM_WAV] = { STR(LANG_AFMT_PCM_WAV) },
    };

    int rec_format = global_settings.rec_format;
    bool res = set_option(str(LANG_RECORDING_FORMAT), &rec_format, INT,
                          names, REC_NUM_FORMATS, NULL );

    if (rec_format != global_settings.rec_format)
    {
        global_settings.rec_format = rec_format;
        enc_global_settings_apply();
    }

    return res;
} /* recformat */

#endif /* CONFIG_CODEC == SWCODEC */

static bool recfrequency(void)
{
#if CONFIG_CODEC == MAS3587F
    static const struct opt_items names[6] = {
        { "44.1kHz", TALK_ID(44, UNIT_KHZ) },
        { "48kHz", TALK_ID(48, UNIT_KHZ) },
        { "32kHz", TALK_ID(32, UNIT_KHZ) },
        { "22.05kHz", TALK_ID(22, UNIT_KHZ) },
        { "24kHz", TALK_ID(24, UNIT_KHZ) },
        { "16kHz", TALK_ID(16, UNIT_KHZ) }
    };
    return set_option(str(LANG_RECORDING_FREQUENCY),
                      &global_settings.rec_frequency, INT,
                      names, 6, NULL );
#endif /* CONFIG_CODEC == MAS3587F */

#if CONFIG_CODEC == SWCODEC
    static const struct opt_items names[REC_NUM_FREQ] = {
        REC_HAVE_96_([REC_FREQ_96] = { "96kHz",     TALK_ID(96, UNIT_KHZ) },)
        REC_HAVE_88_([REC_FREQ_88] = { "88.2kHz",   TALK_ID(88, UNIT_KHZ) },)
        REC_HAVE_64_([REC_FREQ_64] = { "64kHz",     TALK_ID(64, UNIT_KHZ) },)
        REC_HAVE_48_([REC_FREQ_48] = { "48kHz",     TALK_ID(48, UNIT_KHZ) },)
        REC_HAVE_44_([REC_FREQ_44] = { "44.1kHz",   TALK_ID(44, UNIT_KHZ) },)
        REC_HAVE_32_([REC_FREQ_32] = { "32kHz",     TALK_ID(32, UNIT_KHZ) },)
        REC_HAVE_24_([REC_FREQ_24] = { "24kHz",     TALK_ID(24, UNIT_KHZ) },)
        REC_HAVE_22_([REC_FREQ_22] = { "22.05kHz",  TALK_ID(22, UNIT_KHZ) },)
        REC_HAVE_16_([REC_FREQ_16] = { "16kHz",     TALK_ID(16, UNIT_KHZ) },)
        REC_HAVE_12_([REC_FREQ_12] = { "12kHz",     TALK_ID(12, UNIT_KHZ) },)
        REC_HAVE_11_([REC_FREQ_11] = { "11.025kHz", TALK_ID(11, UNIT_KHZ) },)
        REC_HAVE_8_( [REC_FREQ_8 ] = { "8kHz",      TALK_ID( 8, UNIT_KHZ) },)
    };

    struct opt_items opts[REC_NUM_FREQ];
    unsigned long    table[REC_NUM_FREQ];
    int              n_opts;
    int              rec_frequency;
    bool             ret;

#ifdef HAVE_SPDIF_IN
    if (global_settings.rec_source == AUDIO_SRC_SPDIF)
    {
        /* Inform user that frequency follows the source's frequency */
        opts[0].string    = ID2P(LANG_SOURCE_FREQUENCY);
        opts[0].voice_id  = LANG_SOURCE_FREQUENCY;
        n_opts            = 1;
        rec_frequency     = 0;
}
    else
#endif
    {
        struct encoder_caps caps;
        struct encoder_config cfg;

        cfg.rec_format = global_settings.rec_format;
        global_to_encoder_config(&cfg);

        if (!enc_get_caps(&cfg, &caps, true))
            return false;

        /* Construct samplerate menu based upon encoder settings */
        n_opts = make_list_from_caps32(REC_SAMPR_CAPS, NULL,
                                       caps.samplerate_caps, table);

        if (n_opts == 0)
            return false; /* No common flags...?? */

        make_options_from_indexes(names, table, n_opts, opts);

        /* Find closest rate that the potentially restricted list
           comes to */
        make_list_from_caps32(REC_SAMPR_CAPS, rec_freq_sampr,
                              caps.samplerate_caps, table);

        rec_frequency = round_value_to_list32(
                rec_freq_sampr[global_settings.rec_frequency],
                table, n_opts, false);
    }

    ret = set_option(str(LANG_RECORDING_FREQUENCY),
                     &rec_frequency, INT, opts, n_opts, NULL );

    if (!ret
#ifdef HAVE_SPDIF_IN
        && global_settings.rec_source != AUDIO_SRC_SPDIF
#endif
    )
    {
        /* Translate back to full index */
        global_settings.rec_frequency =
                    round_value_to_list32(table[rec_frequency],
                                          rec_freq_sampr,
                                          REC_NUM_FREQ,
                                          false);
    }

    return ret;
#endif /* CONFIG_CODEC == SWCODEC */
} /* recfrequency */

static bool recchannels(void)
{
    static const struct opt_items names[CHN_NUM_MODES] = {
        [CHN_MODE_STEREO] = { STR(LANG_CHANNEL_STEREO) },
        [CHN_MODE_MONO]   = { STR(LANG_CHANNEL_MONO)   }
    };
#if CONFIG_CODEC == MAS3587F
    return set_option(str(LANG_RECORDING_CHANNELS),
                      &global_settings.rec_channels, INT,
                      names, CHN_NUM_MODES, NULL );
#endif /* CONFIG_CODEC == MAS3587F */

#if CONFIG_CODEC == SWCODEC
    struct opt_items    opts[CHN_NUM_MODES];
    long                table[CHN_NUM_MODES];
    struct encoder_caps caps;
    struct encoder_config cfg;
    int                 n_opts;
    int                 rec_channels;
    bool                ret;

    cfg.rec_format = global_settings.rec_format;
    global_to_encoder_config(&cfg);

    if (!enc_get_caps(&cfg, &caps, true))
        return false;

    n_opts = make_list_from_caps32(CHN_CAP_ALL, NULL,
                                   caps.channel_caps, table);

    rec_channels = round_value_to_list32(global_settings.rec_channels,
                                         table, n_opts, false);

    make_options_from_indexes(names, table, n_opts, opts);

    ret = set_option(str(LANG_RECORDING_CHANNELS), &rec_channels,
                     INT, opts, n_opts, NULL );

    if (!ret)
        global_settings.rec_channels = table[rec_channels];

    return ret;
#endif /* CONFIG_CODEC == SWCODEC */
}

static bool rectimesplit(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "00:05" , TALK_ID(5, UNIT_MIN) },
        { "00:10" , TALK_ID(10, UNIT_MIN) },
        { "00:15" , TALK_ID(15, UNIT_MIN) },
        { "00:30" , TALK_ID(30, UNIT_MIN) },
        { "01:00" , TALK_ID(1, UNIT_HOUR) },
        { "01:14" , TALK_ID(74, UNIT_MIN) },
        { "01:20" , TALK_ID(80, UNIT_MIN) },
        { "02:00" , TALK_ID(2, UNIT_HOUR) },
        { "04:00" , TALK_ID(4, UNIT_HOUR) },
        { "06:00" , TALK_ID(6, UNIT_HOUR) },
        { "08:00" , TALK_ID(8, UNIT_HOUR) },
        { "10:00" , TALK_ID(10, UNIT_HOUR) },
        { "12:00" , TALK_ID(12, UNIT_HOUR) },
        { "18:00" , TALK_ID(18, UNIT_HOUR) },
        { "24:00" , TALK_ID(24, UNIT_HOUR) }
    };
    return set_option(str(LANG_SPLIT_TIME),
                      &global_settings.rec_timesplit, INT,
                      names, 16, NULL );
}

static bool recsizesplit(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "5MB" , TALK_ID(5, UNIT_MB) },
        { "10MB" , TALK_ID(10, UNIT_MB) },
        { "15MB" , TALK_ID(15, UNIT_MB) },
        { "32MB" , TALK_ID(32, UNIT_MB) },
        { "64MB" , TALK_ID(64, UNIT_MB) },
        { "75MB" , TALK_ID(75, UNIT_MB) },
        { "100MB" , TALK_ID(100, UNIT_MB) },
        { "128MB" , TALK_ID(128, UNIT_MB) },
        { "256MB" , TALK_ID(256, UNIT_MB) },
        { "512MB" , TALK_ID(512, UNIT_MB) },
        { "650MB" , TALK_ID(650, UNIT_MB) },
        { "700MB" , TALK_ID(700, UNIT_MB) },
        { "1GB" , TALK_ID(1024, UNIT_MB) },
        { "1.5GB" , TALK_ID(1536, UNIT_MB) },
        { "1.75GB" , TALK_ID(1792, UNIT_MB) }
    };
    return set_option(str(LANG_SPLIT_SIZE),
                      &global_settings.rec_sizesplit, INT,
                      names, 16, NULL );
}

static bool splitmethod(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_REC_TIME) },
        { STR(LANG_REC_SIZE) },
    };
    bool ret;
    ret=set_option( str(LANG_SPLIT_MEASURE),
                    &global_settings.rec_split_method, INT, names, 2, NULL);
    return ret;
}

static bool splittype(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_START_NEW_FILE) },
        { STR(LANG_STOP_RECORDING) },
    };
    bool ret;
    ret=set_option( str(LANG_SPLIT_TYPE),
                    &global_settings.rec_split_type, INT, names, 2, NULL);
    return ret;
}

static bool filesplitoptionsmenu(void)
{
    int m;
    bool result;

    static const struct menu_item items[] = {
        { ID2P(LANG_SPLIT_MEASURE), splitmethod },
        { ID2P(LANG_SPLIT_TYPE),  splittype  },
        { ID2P(LANG_SPLIT_TIME),  rectimesplit  },
        { ID2P(LANG_SPLIT_SIZE), recsizesplit   }
    };
     m=menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);
    return result;
}
static bool recprerecord(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { "1s", TALK_ID(1, UNIT_SEC) },
        { "2s", TALK_ID(2, UNIT_SEC) },
        { "3s", TALK_ID(3, UNIT_SEC) },
        { "4s", TALK_ID(4, UNIT_SEC) },
        { "5s", TALK_ID(5, UNIT_SEC) },
        { "6s", TALK_ID(6, UNIT_SEC) },
        { "7s", TALK_ID(7, UNIT_SEC) },
        { "8s", TALK_ID(8, UNIT_SEC) },
        { "9s", TALK_ID(9, UNIT_SEC) },
        { "10s", TALK_ID(10, UNIT_SEC) },
        { "11s", TALK_ID(11, UNIT_SEC) },
        { "12s", TALK_ID(12, UNIT_SEC) },
        { "13s", TALK_ID(13, UNIT_SEC) },
        { "14s", TALK_ID(14, UNIT_SEC) },
        { "15s", TALK_ID(15, UNIT_SEC) },
        { "16s", TALK_ID(16, UNIT_SEC) },
        { "17s", TALK_ID(17, UNIT_SEC) },
        { "18s", TALK_ID(18, UNIT_SEC) },
        { "19s", TALK_ID(19, UNIT_SEC) },
        { "20s", TALK_ID(20, UNIT_SEC) },
        { "21s", TALK_ID(21, UNIT_SEC) },
        { "22s", TALK_ID(22, UNIT_SEC) },
        { "23s", TALK_ID(23, UNIT_SEC) },
        { "24s", TALK_ID(24, UNIT_SEC) },
        { "25s", TALK_ID(25, UNIT_SEC) },
        { "26s", TALK_ID(26, UNIT_SEC) },
        { "27s", TALK_ID(27, UNIT_SEC) },
        { "28s", TALK_ID(28, UNIT_SEC) },
        { "29s", TALK_ID(29, UNIT_SEC) },
        { "30s", TALK_ID(30, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORD_PRERECORD_TIME),
                      &global_settings.rec_prerecord_time, INT,
                      names, 31, NULL );
}

static bool recdirectory(void)
{
    static const struct opt_items names[] = {
        { rec_base_directory, -1 },
        { STR(LANG_RECORD_CURRENT_DIR) }
    };
    return set_option(str(LANG_RECORD_DIRECTORY),
                      &global_settings.rec_directory, INT,
                      names, 2, NULL );
}

static bool reconstartup(void)
{
    return set_bool(str(LANG_RECORD_STARTUP),
                    &global_settings.rec_startup);
}

#if CONFIG_BACKLIGHT
static bool cliplight(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_MAIN_UNIT) }
#ifdef HAVE_REMOTE_LCD
      , { STR(LANG_REMOTE_MAIN) },
        { STR(LANG_REMOTE_UNIT) }
#endif
    };
      
    return set_option( str(LANG_CLIP_LIGHT),
                    &global_settings.cliplight, INT, names,
#ifdef HAVE_REMOTE_LCD
                     4, NULL );
#else
                     2, NULL );
#endif
}
#endif /*CONFIG_BACKLIGHT */

#ifdef HAVE_AGC
static bool agc_preset(void)
{
    static const struct opt_items names[] = {
        { STR(LANG_OFF) },
        { STR(LANG_AGC_SAFETY) },
        { STR(LANG_AGC_LIVE) },
        { STR(LANG_AGC_DJSET) },
        { STR(LANG_AGC_MEDIUM) },
        { STR(LANG_AGC_VOICE) },
    };
    if (global_settings.rec_source)
        return set_option(str(LANG_RECORD_AGC_PRESET),
                          &global_settings.rec_agc_preset_line,
                          INT, names, 6, NULL );
    else
        return set_option(str(LANG_RECORD_AGC_PRESET),
                          &global_settings.rec_agc_preset_mic,
                          INT, names, 6, NULL );
}

static bool agc_cliptime(void)
{
    static const struct opt_items names[] = {
        { "200ms", TALK_ID(200, UNIT_MS) },
        { "400ms", TALK_ID(400, UNIT_MS) },
        { "600ms", TALK_ID(600, UNIT_MS) },
        { "800ms", TALK_ID(800, UNIT_MS) },
        { "1s", TALK_ID(1, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORD_AGC_CLIPTIME),
                      &global_settings.rec_agc_cliptime,
                      INT, names, 5, NULL );
}
#endif /* HAVE_AGC */
#endif /* HAVE_RECORDING */



#ifdef HAVE_RECORDING
enum trigger_menu_option
{
    TRIGGER_MODE,
    TRIGGER_TYPE,
    PRERECORD_TIME,
    START_THRESHOLD,
    START_DURATION,
    STOP_THRESHOLD,
    STOP_POSTREC,
    STOP_GAP,
    TRIG_OPTION_COUNT,
};

#ifdef HAVE_RECORDING
static char* create_thres_str(int threshold)
{
    static char retval[6];
    if (threshold < 0) {
        if (threshold < -88) {
            snprintf (retval, sizeof retval, "%s", str(LANG_DB_INF));
        } else {
            snprintf (retval, sizeof retval, "%ddb", threshold + 1);
        }
    } else {
        snprintf (retval, sizeof retval, "%d%%", threshold);
    }
    return retval;
}

#define INF_DB (-89)
static void change_threshold(int *threshold, int change)
{
    if (global_settings.peak_meter_dbfs) {
        if (*threshold >= 0) {
            int db = (calc_db(*threshold * MAX_PEAK / 100) - 9000) / 100;
            *threshold = db;
        }
        *threshold += change;
        if (*threshold > -1) {
            *threshold = INF_DB;
        } else if (*threshold < INF_DB) {
            *threshold = -1;
        }
    } else {
        if (*threshold < 0) {
            *threshold = peak_meter_db2sample(*threshold * 100) * 100 / MAX_PEAK;
        }
        *threshold += change;
        if (*threshold > 100) {
            *threshold = 0;
        } else if (*threshold < 0) {
            *threshold = 100;
        }
    }
}

/**
 * Displays a menu for editing the trigger settings.
 */
bool rectrigger(void)
{
    int exit_request = false;
    enum trigger_menu_option selected = TRIGGER_MODE;
    bool retval = false;
    int old_x_margin[NB_SCREENS];
    int old_y_margin[NB_SCREENS];

#define TRIGGER_MODE_COUNT 3
    static const unsigned char *trigger_modes[] = {
        ID2P(LANG_OFF),
        ID2P(LANG_RECORD_TRIG_NOREARM),
        ID2P(LANG_RECORD_TRIG_REARM)
    };

#define PRERECORD_TIMES_COUNT 31
    static const unsigned char *prerecord_times[] = {
        ID2P(LANG_OFF),"1s","2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s",
        "10s", "11s", "12s", "13s", "14s", "15s", "16s", "17s", "18s", "19s",
        "20s", "21s", "22s", "23s", "24s", "25s", "26s", "27s", "28s", "29s",
        "30s"
    };

#define TRIGGER_TYPE_COUNT 3
    static const unsigned char *trigger_types[] = {
        ID2P(LANG_RECORD_TRIGGER_STOP),
        ID2P(LANG_RECORD_TRIGGER_PAUSE),
        ID2P(LANG_RECORD_TRIGGER_NEWFILESTP),
    };

    static const unsigned char *option_name[] = {
        [TRIGGER_MODE] =    ID2P(LANG_RECORD_TRIGGER_MODE),
        [TRIGGER_TYPE] =    ID2P(LANG_RECORD_TRIGGER_TYPE),
        [PRERECORD_TIME] =  ID2P(LANG_RECORD_PRERECORD_TIME),
        [START_THRESHOLD] = ID2P(LANG_RECORD_START_THRESHOLD),
        [START_DURATION] =  ID2P(LANG_RECORD_MIN_DURATION),
        [STOP_THRESHOLD] =  ID2P(LANG_RECORD_STOP_THRESHOLD),
        [STOP_POSTREC] =    ID2P(LANG_RECORD_STOP_POSTREC),
        [STOP_GAP] =        ID2P(LANG_RECORD_STOP_GAP)
    };

    int old_start_thres = global_settings.rec_start_thres;
    int old_start_duration = global_settings.rec_start_duration;
    int old_prerecord_time = global_settings.rec_prerecord_time;
    int old_stop_thres = global_settings.rec_stop_thres;
    int old_stop_postrec = global_settings.rec_stop_postrec;
    int old_stop_gap = global_settings.rec_stop_gap;
    int old_trigger_mode = global_settings.rec_trigger_mode;
    int old_trigger_type = global_settings.rec_trigger_type;

    int offset[NB_SCREENS];
    int option_lines[NB_SCREENS];
    int w, h, i;
    int stat_height = global_settings.statusbar ? STATUSBAR_HEIGHT : 0;
    int pm_y[NB_SCREENS];

    int trig_xpos[NB_SCREENS];
    int trig_ypos[NB_SCREENS];
    int trig_width[NB_SCREENS];

    FOR_NB_SCREENS(i)
    {
        offset[i] = 0;
        trig_xpos[i] = 0;
        trig_ypos[i] =  screens[i].height - stat_height - TRIG_HEIGHT;
        pm_y[i] = screens[i].height - stat_height;
        trig_width[i] = screens[i].width;
    }

    /* restart trigger with new values */
    settings_apply_trigger();
    peak_meter_trigger (global_settings.rec_trigger_mode != TRIG_MODE_OFF);

    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();

        old_x_margin[i] = screens[i].getxmargin();
        old_y_margin[i] = screens[i].getymargin();
        if(global_settings.statusbar)
           screens[i].setmargins(0, STATUSBAR_HEIGHT);
      else
          screens[i].setmargins(0, 0);

      screens[i].getstringsize("M", &w, &h);

      // 16 pixels are reserved for peak meter and trigger status
      option_lines[i] = MIN(((screens[i].height) -
                          stat_height - 16)/h,
                           TRIG_OPTION_COUNT);
    }

    while (!exit_request) {
        int button, k;
        const char *str;
        char option_value[TRIG_OPTION_COUNT][9];

        snprintf(
            option_value[TRIGGER_MODE],
            sizeof option_value[TRIGGER_MODE],
            "%s",
            P2STR(trigger_modes[global_settings.rec_trigger_mode]));

        snprintf(
            option_value[TRIGGER_TYPE],
            sizeof option_value[TRIGGER_TYPE],
            "%s",
            P2STR(trigger_types[global_settings.rec_trigger_type]));

        snprintf(
            option_value[TRIGGER_TYPE],
            sizeof option_value[TRIGGER_TYPE],
            "%s",
            P2STR(trigger_types[global_settings.rec_trigger_type]));

        snprintf (
            option_value[PRERECORD_TIME],
            sizeof option_value[PRERECORD_TIME],
            "%s",
            P2STR(prerecord_times[global_settings.rec_prerecord_time]));

        /* due to value range shift (peak_meter_define_trigger) -1 is 0db */
        if (global_settings.rec_start_thres == -1) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_start_thres);
        }
        snprintf(
            option_value[START_THRESHOLD],
            sizeof option_value[START_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[START_DURATION],
            sizeof option_value[START_DURATION],
            "%s",
            trig_durations[global_settings.rec_start_duration]);


        if (global_settings.rec_stop_thres <= INF_DB) {
            str = str(LANG_OFF);
        } else {
            str = create_thres_str(global_settings.rec_stop_thres);
        }
        snprintf(
            option_value[STOP_THRESHOLD],
            sizeof option_value[STOP_THRESHOLD],
            "%s",
            str);

        snprintf(
            option_value[STOP_POSTREC],
            sizeof option_value[STOP_POSTREC],
            "%s",
            trig_durations[global_settings.rec_stop_postrec]);

        snprintf(
            option_value[STOP_GAP],
            sizeof option_value[STOP_GAP],
            "%s",
            trig_durations[global_settings.rec_stop_gap]);

        FOR_NB_SCREENS(i)
        {
            screens[i].set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            screens[i].fillrect(0, stat_height, screens[i].width,
                                   screens[i].height - stat_height);
            screens[i].set_drawmode(DRMODE_SOLID);
        }

        gui_syncstatusbar_draw(&statusbars, true);

        /* reselect FONT_SYSFONT as status_draw has changed the font */
        /*lcd_setfont(FONT_SYSFIXED);*/

        FOR_NB_SCREENS(i)
        {
            for (k = 0; k < option_lines[i]; k++) {
                int x, y;

                str = P2STR(option_name[k + offset[i]]);
                screens[i].putsxy((option_lines[i] < TRIG_OPTION_COUNT) ? 5 : 0,
                                       stat_height + k * h, str);

                str = option_value[k + offset[i]];
                screens[i].getstringsize(str, &w, &h);
                y = stat_height + k * h;
                x = screens[i].width - w;
                screens[i].putsxy(x, y, str);
                if ((int)selected == (k + offset[i])) {
                    screens[i].set_drawmode(DRMODE_COMPLEMENT);
                    screens[i].fillrect(x, y, w, h);
                    screens[i].set_drawmode(DRMODE_SOLID);
                }
            }
            if (option_lines[i] < TRIG_OPTION_COUNT)
                gui_scrollbar_draw(&screens[i], 0, stat_height,
                    4, screens[i].height - 16 - stat_height,
                    TRIG_OPTION_COUNT, offset[i], offset[i] + option_lines[i],
                    VERTICAL);
        }

        peak_meter_draw_trig(trig_xpos, trig_ypos, trig_width, NB_SCREENS);
        button = peak_meter_draw_get_btn(0, pm_y, 8, NB_SCREENS);

        FOR_NB_SCREENS(i)
            screens[i].update();

        switch (button) {
            case ACTION_STD_CANCEL:
                gui_syncsplash(50, true, str(LANG_MENU_SETTING_CANCEL));
                global_settings.rec_start_thres = old_start_thres;
                global_settings.rec_start_duration = old_start_duration;
                global_settings.rec_prerecord_time = old_prerecord_time;
                global_settings.rec_stop_thres = old_stop_thres;
                global_settings.rec_stop_postrec = old_stop_postrec;
                global_settings.rec_stop_gap = old_stop_gap;
                global_settings.rec_trigger_mode = old_trigger_mode;
                global_settings.rec_trigger_type = old_trigger_type;
                exit_request = true;
                break;

            case ACTION_REC_PAUSE:
                exit_request = true;
                break;

            case ACTION_STD_PREV:
                selected += TRIG_OPTION_COUNT - 1;
                selected %= TRIG_OPTION_COUNT;
                FOR_NB_SCREENS(i)
                {
                    offset[i] = MIN(offset[i], (int)selected);
                    offset[i] = MAX(offset[i], (int)selected - option_lines[i] + 1);
                }
                break;

            case ACTION_STD_NEXT:
                selected ++;
                selected %= TRIG_OPTION_COUNT;
                FOR_NB_SCREENS(i)
                {
                    offset[i] = MIN(offset[i], (int)selected);
                    offset[i] = MAX(offset[i], (int)selected - option_lines[i] + 1);
                }
                break;

            case ACTION_SETTINGS_INC:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode ++;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case TRIGGER_TYPE:
                        global_settings.rec_trigger_type ++;
                        global_settings.rec_trigger_type %= TRIGGER_TYPE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time ++;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, 1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration ++;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, 1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec ++;
                        global_settings.rec_stop_postrec %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap ++;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                break;

            case ACTION_SETTINGS_DEC:
                switch (selected) {
                    case TRIGGER_MODE:
                        global_settings.rec_trigger_mode+=TRIGGER_MODE_COUNT-1;
                        global_settings.rec_trigger_mode %= TRIGGER_MODE_COUNT;
                        break;

                    case TRIGGER_TYPE:
                        global_settings.rec_trigger_type+=TRIGGER_TYPE_COUNT-1;
                        global_settings.rec_trigger_type %= TRIGGER_TYPE_COUNT;
                        break;

                    case PRERECORD_TIME:
                        global_settings.rec_prerecord_time += PRERECORD_TIMES_COUNT - 1;
                        global_settings.rec_prerecord_time %= PRERECORD_TIMES_COUNT;
                        break;

                    case START_THRESHOLD:
                        change_threshold(&global_settings.rec_start_thres, -1);
                        break;

                    case START_DURATION:
                        global_settings.rec_start_duration += TRIG_DURATION_COUNT-1;
                        global_settings.rec_start_duration %= TRIG_DURATION_COUNT;
                        break;

                    case STOP_THRESHOLD:
                        change_threshold(&global_settings.rec_stop_thres, -1);
                        break;

                    case STOP_POSTREC:
                        global_settings.rec_stop_postrec +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_postrec %=
                            TRIG_DURATION_COUNT;
                        break;

                    case STOP_GAP:
                        global_settings.rec_stop_gap +=
                            TRIG_DURATION_COUNT - 1;
                        global_settings.rec_stop_gap %= TRIG_DURATION_COUNT;
                        break;

                    case TRIG_OPTION_COUNT:
                        // avoid compiler warnings
                        break;
                }
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                break;

            case ACTION_REC_F2:
                peak_meter_trigger(true);
                break;

            case SYS_USB_CONNECTED:
                if(default_event_handler(button) == SYS_USB_CONNECTED) {
                    retval = true;
                    exit_request = true;
                }
                break;
        }
    }

    peak_meter_trigger(false);
    FOR_NB_SCREENS(i)
    {
        screens[i].setfont(FONT_UI);
        screens[i].setmargins(old_x_margin[i], old_y_margin[i]);
    }
    action_signalscreenchange();
    return retval;
}
#endif /* !defined(SIMULATOR) && CONFIG_CODEC == MAS3587F */

bool recording_menu(bool no_source)
{
    static const struct menu_item static_items[] = {
#if CONFIG_CODEC == MAS3587F
        { ID2P(LANG_RECORDING_QUALITY), recquality },
#endif
#if CONFIG_CODEC == SWCODEC
        { ID2P(LANG_RECORDING_FORMAT), recformat },
        { ID2P(LANG_ENCODER_SETTINGS), enc_global_config_menu },
#endif
        { ID2P(LANG_RECORDING_FREQUENCY), recfrequency },
        { ID2P(LANG_RECORDING_SOURCE), recsource }, /* not shown if no_source */
        { ID2P(LANG_RECORDING_CHANNELS), recchannels },
#if CONFIG_CODEC == MAS3587F
        { ID2P(LANG_RECORDING_EDITABLE), receditable },
#endif
        { ID2P(LANG_RECORD_TIMESPLIT), filesplitoptionsmenu },
        { ID2P(LANG_RECORD_PRERECORD_TIME), recprerecord },
        { ID2P(LANG_RECORD_DIRECTORY), recdirectory },
        { ID2P(LANG_RECORD_STARTUP), reconstartup },
#if CONFIG_BACKLIGHT
        { ID2P(LANG_CLIP_LIGHT), cliplight },
#endif
        { ID2P(LANG_RECORD_TRIGGER), rectrigger },
#ifdef HAVE_AGC
        { ID2P(LANG_RECORD_AGC_PRESET), agc_preset },
        { ID2P(LANG_RECORD_AGC_CLIPTIME), agc_cliptime },
#endif
    };

    struct menu_item items[ARRAYLEN(static_items)];
    int i, n_items;
    int m;

    bool result;

    for (i = 0, n_items = 0; i < (int)ARRAYLEN(items); i++)
    {
        const struct menu_item *mi = &static_items[i];
        if (no_source && mi->function == recsource)
            continue;
        items[n_items++] = *mi;
    }

    m = menu_init(items, n_items, NULL, NULL, NULL, NULL);
    result = menu_run(m);
    menu_exit(m);

    return result;
} /* recording_menu */

#endif /* HAVE_RECORDING */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#include <stdlib.h>
#include <stdbool.h>
#include "string.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "menu.h"
#include "button.h"
#include "mp3_playback.h"
#include "settings.h"
#include "statusbar.h"
#include "screens.h"
#include "icons.h"
#ifdef HAVE_LCD_BITMAP
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
#ifdef HAVE_RECORDING
#include "enc_config.h"
#endif
#include "general.h"
#endif
#include "action.h"
#include "recording.h"
#include "sound_menu.h"
#include "option_select.h"
#include "settings_list.h"
#include "list.h"
#include "viewport.h"

static bool no_source_in_menu = false;
static int recmenu_callback(int action,const struct menu_item_ex *this_item);

static int recsource_func(void)
{
    int n_opts = REC_NUM_SOURCES;

    static const struct opt_items names[AUDIO_NUM_SOURCES] = {
        HAVE_MIC_REC_([AUDIO_SRC_MIC]
            = { STR(LANG_RECORDING_SRC_MIC) },)
        HAVE_LINE_REC_([AUDIO_SRC_LINEIN]
            = { STR(LANG_LINE_IN) },)
        HAVE_SPDIF_REC_([AUDIO_SRC_SPDIF]
            = { STR(LANG_RECORDING_SRC_DIGITAL) },)
        HAVE_FMRADIO_REC_([AUDIO_SRC_FMRADIO]
            = { STR(LANG_FM_RADIO) },)
    };

    /* caveat: assumes it's the last item! */
#ifdef HAVE_FMRADIO_REC
    if (!radio_hardware_present())
        n_opts--;
#endif

    return set_option(str(LANG_RECORDING_SOURCE),
                      &global_settings.rec_source, INT, names,
                      n_opts, NULL );
}
MENUITEM_FUNCTION(recsource, 0, ID2P(LANG_RECORDING_SOURCE), 
                    recsource_func, NULL, recmenu_callback, Icon_Menu_setting);

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


#endif /* CONFIG_CODEC == SWCODEC */

static int recfrequency_func(void)
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

#ifdef HAVE_SPDIF_REC
    if (global_settings.rec_source == REC_SRC_SPDIF)
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
        HAVE_SPDIF_REC_( && global_settings.rec_source != REC_SRC_SPDIF)
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
MENUITEM_FUNCTION(recfrequency, 0, ID2P(LANG_RECORDING_FREQUENCY), 
                    recfrequency_func, NULL, NULL, Icon_Menu_setting);


static int recchannels_func(void)
{
    static const struct opt_items names[CHN_NUM_MODES] = {
        [CHN_MODE_STEREO] = { STR(LANG_CHANNEL_STEREO) },
        [CHN_MODE_MONO]   = { STR(LANG_CHANNEL_MONO)   }
    };
#if CONFIG_CODEC == MAS3587F
    return set_option(str(LANG_CHANNELS),
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

    ret = set_option(str(LANG_CHANNELS), &rec_channels,
                     INT, opts, n_opts, NULL );

    if (!ret)
        global_settings.rec_channels = table[rec_channels];

    return ret;
#endif /* CONFIG_CODEC == SWCODEC */
}
MENUITEM_FUNCTION(recchannels, 0, ID2P(LANG_CHANNELS),
                    recchannels_func, NULL, NULL, Icon_Menu_setting);

#if CONFIG_CODEC == SWCODEC

static int recformat_func(void)
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
MENUITEM_FUNCTION(recformat, 0, ID2P(LANG_RECORDING_FORMAT), 
                    recformat_func, NULL, NULL, Icon_Menu_setting);

MENUITEM_FUNCTION(enc_global_config_menu_item, 0, ID2P(LANG_ENCODER_SETTINGS),
                    (int(*)(void))enc_global_config_menu,
                     NULL, NULL, Icon_Submenu);

#endif /* CONFIG_CODEC == SWCODEC */


static int recmenu_callback(int action,const struct menu_item_ex *this_item)
{
    switch (action)
    {
        case ACTION_REQUEST_MENUITEM:
            if (this_item == &recsource && no_source_in_menu)
                return ACTION_EXIT_MENUITEM;
            break;
    }
    return action;
}
#if CONFIG_CODEC == MAS3587F
MENUITEM_SETTING(rec_quality, &global_settings.rec_quality, NULL);
MENUITEM_SETTING(rec_editable, &global_settings.rec_editable, NULL);
#endif

MENUITEM_SETTING(rec_split_type, &global_settings.rec_split_type, NULL);
MENUITEM_SETTING(rec_split_method, &global_settings.rec_split_method, NULL);
MENUITEM_SETTING(rec_timesplit, &global_settings.rec_timesplit, NULL);
MENUITEM_SETTING(rec_sizesplit, &global_settings.rec_sizesplit, NULL);
MAKE_MENU(filesplitoptionsmenu, ID2P(LANG_RECORD_TIMESPLIT), NULL, Icon_NOICON,
            &rec_split_method, &rec_split_type, &rec_timesplit, &rec_sizesplit);


MENUITEM_SETTING(rec_prerecord_time, &global_settings.rec_prerecord_time, NULL);

static int clear_rec_directory(void)
{
    strcpy(global_settings.rec_directory, REC_BASE_DIR);
    gui_syncsplash(HZ, ID2P(LANG_RESET_DONE_CLEAR));
    return false;
}
MENUITEM_FUNCTION(clear_rec_directory_item, 0, ID2P(LANG_CLEAR_REC_DIR), 
                  clear_rec_directory, NULL, NULL, Icon_Folder);
                  
MENUITEM_SETTING(cliplight, &global_settings.cliplight, NULL);

#ifdef HAVE_AGC
static int agc_preset_func(void)
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
        return set_option(str(LANG_RECORDING_AGC_PRESET),
                          &global_settings.rec_agc_preset_line,
                          INT, names, 6, NULL );
    else
        return set_option(str(LANG_RECORDING_AGC_PRESET),
                          &global_settings.rec_agc_preset_mic,
                          INT, names, 6, NULL );
}

static int agc_cliptime_func(void)
{
    static const struct opt_items names[] = {
        { "200ms", TALK_ID(200, UNIT_MS) },
        { "400ms", TALK_ID(400, UNIT_MS) },
        { "600ms", TALK_ID(600, UNIT_MS) },
        { "800ms", TALK_ID(800, UNIT_MS) },
        { "1s", TALK_ID(1, UNIT_SEC) }
    };
    return set_option(str(LANG_RECORDING_AGC_CLIPTIME),
                      &global_settings.rec_agc_cliptime,
                      INT, names, 5, NULL );
}
MENUITEM_FUNCTION(agc_preset, 0, ID2P(LANG_RECORDING_AGC_PRESET), 
                    agc_preset_func, NULL, NULL, Icon_Menu_setting);
MENUITEM_FUNCTION(agc_cliptime, 0, ID2P(LANG_RECORDING_AGC_CLIPTIME), 
                    agc_cliptime_func, NULL, NULL, Icon_Menu_setting);
#endif /* HAVE_AGC */

/** Rec trigger **/
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

static enum themable_icons trigger_get_icon(int selected_item, void * data)
{
    (void)data;
    if ((selected_item % 2) == 0) /* header */
          return Icon_Menu_setting;
    return Icon_NOICON;
}

static char * trigger_get_name(int selected_item, void * data,
                        char * buffer, size_t buffer_len)
{
    const struct settings_list **settings = 
            (const struct settings_list **)data;
    const struct settings_list *s = settings[selected_item / 2];
    if ((selected_item % 2) == 0) /* header */
        return P2STR(ID2P(s->lang_id));
    else
    {
        int temp;
        temp = option_value_as_int(s);
        if ((selected_item / 2 == START_THRESHOLD ||
             selected_item / 2 == STOP_THRESHOLD) &&
             temp == 0)
        {
            return str(LANG_OFF);
        }
        return option_get_valuestring(s, buffer, buffer_len, temp);
    }
}
static void trigger_speak_item(const struct settings_list **settings,
                               int selected_setting, bool title)
{
    const struct settings_list *s = settings[selected_setting];
    int temp;
    if (!global_settings.talk_menu)
        return;
    if (title)
        talk_id(s->lang_id, false);
    temp = option_value_as_int(s);
    if ((selected_setting == START_THRESHOLD ||
         selected_setting == STOP_THRESHOLD) &&
        temp == 0)
    {
        talk_id(LANG_OFF, title?true:false);
    } else {
        option_talk_value(s, temp, title?true:false);
    }
}
int rectrigger(void)
{
    struct viewport vp[NB_SCREENS];
    struct gui_synclist lists;
    int i, action = ACTION_REDRAW;
    bool done = false, changed = true;
    const struct settings_list *settings[TRIG_OPTION_COUNT];

    int pm_x[NB_SCREENS];
    int pm_y[NB_SCREENS];
    int pm_h[NB_SCREENS];
    int trig_xpos[NB_SCREENS];
    int trig_ypos[NB_SCREENS];
    int trig_width[NB_SCREENS];

    int old_start_thres_db = global_settings.rec_start_thres_db;
    int old_start_thres_linear = global_settings.rec_start_thres_linear;
    int old_start_duration = global_settings.rec_start_duration;
    int old_prerecord_time = global_settings.rec_prerecord_time;
    int old_stop_thres_db = global_settings.rec_stop_thres_db;
    int old_stop_thres_linear = global_settings.rec_stop_thres_linear;
    int old_stop_postrec = global_settings.rec_stop_postrec;
    int old_stop_gap = global_settings.rec_stop_gap;
    int old_trigger_mode = global_settings.rec_trigger_mode;
    int old_trigger_type = global_settings.rec_trigger_type;

    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();
        screens[i].update();
        viewport_set_defaults(&vp[i], i);
        vp[i].height -= SYSFONT_HEIGHT*2;
        trig_xpos[i] = 0;
        trig_ypos[i] =  vp[i].y + vp[i].height;
        pm_x[i] = 0;
        pm_y[i] = screens[i].getheight() - SYSFONT_HEIGHT;
        pm_h[i] = SYSFONT_HEIGHT;
        trig_width[i] = screens[i].getwidth();
    }
    /* TODO: what to do if there is < 4 lines on the screen? */

    settings[TRIGGER_MODE] =
            find_setting(&global_settings.rec_trigger_mode, NULL);
    settings[TRIGGER_TYPE] =
            find_setting(&global_settings.rec_trigger_type, NULL);
    settings[PRERECORD_TIME] =
            find_setting(&global_settings.rec_prerecord_time, NULL);
    settings[START_DURATION] =
            find_setting(&global_settings.rec_start_duration, NULL);
    settings[STOP_POSTREC] =
            find_setting(&global_settings.rec_stop_postrec, NULL);
    settings[STOP_GAP] =
            find_setting(&global_settings.rec_stop_gap, NULL);
    if (global_settings.peak_meter_dbfs) /* show the dB settings */
    {
        settings[START_THRESHOLD] =
                find_setting(&global_settings.rec_start_thres_db, NULL);
        settings[STOP_THRESHOLD] =
                find_setting(&global_settings.rec_stop_thres_db, NULL);
    }
    else
    {
        settings[START_THRESHOLD] =
                find_setting(&global_settings.rec_start_thres_linear, NULL);
        settings[STOP_THRESHOLD] =
                find_setting(&global_settings.rec_stop_thres_linear, NULL);
    }
    gui_synclist_init(&lists, trigger_get_name, settings, false, 2, vp);
    gui_synclist_set_nb_items(&lists, TRIG_OPTION_COUNT*2);
    gui_synclist_set_icon_callback(&lists, trigger_get_icon);
    /* restart trigger with new values */
    settings_apply_trigger();
    peak_meter_trigger (global_settings.rec_trigger_mode != TRIG_MODE_OFF);
    
    trigger_speak_item(settings, 0, true);

    while (!done)
    {
        if (changed)
        {
            gui_synclist_draw(&lists);
            gui_syncstatusbar_draw(&statusbars, true);
            peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
            settings_apply_trigger();
            changed = false;
        }
        
        peak_meter_draw_trig(trig_xpos, trig_ypos, trig_width, NB_SCREENS);
        action = peak_meter_draw_get_btn(CONTEXT_SETTINGS_RECTRIGGER,
                                         pm_x, pm_y, pm_h, NB_SCREENS);
        FOR_NB_SCREENS(i)
            screens[i].update();
        i = gui_synclist_get_sel_pos(&lists);
        switch (action)
        {
            case ACTION_STD_CANCEL:
                gui_syncsplash(HZ/2, ID2P(LANG_CANCEL));
                global_settings.rec_start_thres_db = old_start_thres_db;
                global_settings.rec_start_thres_linear = old_start_thres_linear;
                global_settings.rec_start_duration = old_start_duration;
                global_settings.rec_prerecord_time = old_prerecord_time;
                global_settings.rec_stop_thres_db = old_stop_thres_db;
                global_settings.rec_stop_thres_linear = old_stop_thres_linear;
                global_settings.rec_stop_postrec = old_stop_postrec;
                global_settings.rec_stop_gap = old_stop_gap;
                global_settings.rec_trigger_mode = old_trigger_mode;
                global_settings.rec_trigger_type = old_trigger_type;
                peak_meter_trigger(global_settings.rec_trigger_mode!=TRIG_OFF);
                settings_apply_trigger();
                done = true;
                break;
            case ACTION_STD_OK:
                done = true;
                break;
            case ACTION_SETTINGS_DEC:
            case ACTION_SETTINGS_DECREPEAT:
                option_select_next_val(settings[i/2], true, false);
                trigger_speak_item(settings, i/2, false);
                changed = true;
                break;
            case ACTION_SETTINGS_INC:
            case ACTION_SETTINGS_INCREPEAT:
                option_select_next_val(settings[i/2], false, false);
                trigger_speak_item(settings, i/2, false);
                changed = true;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                i -= 2;
                if (i<0)
                    i = (TRIG_OPTION_COUNT*2) - 2;
                gui_synclist_select_item(&lists, i);
                i = gui_synclist_get_sel_pos(&lists);
                trigger_speak_item(settings, i/2, true);
                changed = true;
                break;
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                gui_synclist_select_item(&lists, (i+2) % (TRIG_OPTION_COUNT*2));
                i = gui_synclist_get_sel_pos(&lists);
                trigger_speak_item(settings, i/2, true);
                changed = true;
                break;
        }
    }
    peak_meter_trigger(false);
    settings_save();
    return 0;
}
MENUITEM_FUNCTION(rectrigger_item, 0, ID2P(LANG_RECORD_TRIGGER), 
                  rectrigger, NULL, NULL, Icon_Menu_setting);



/* from main_menu.c */
struct browse_folder_info {
    const char* dir;
    int show_options;
};
static struct browse_folder_info rec_config_browse = {RECPRESETS_DIR, SHOW_CFG};
int browse_folder(void *param);
MENUITEM_FUNCTION(browse_recconfigs, MENU_FUNC_USEPARAM, ID2P(LANG_CUSTOM_CFG), 
                  browse_folder, (void*)&rec_config_browse, NULL, Icon_Config);
static int write_settings_file(void)
{
    return settings_save_config(SETTINGS_SAVE_RECPRESETS);
}
MENUITEM_FUNCTION(save_recpresets_item, 0, ID2P(LANG_SAVE_SETTINGS), 
                  write_settings_file, NULL, NULL, Icon_Config);

MAKE_MENU(recording_settings_menu, ID2P(LANG_RECORDING_SETTINGS),
            NULL, Icon_Recording,
#if CONFIG_CODEC == MAS3587F
            &rec_quality,
#endif
#if CONFIG_CODEC == SWCODEC
            &recformat, &enc_global_config_menu_item,
#endif
            &recfrequency, &recsource, /* recsource not shown if no_source */
            &recchannels,
#if CONFIG_CODEC == MAS3587F
            &rec_editable,
#endif
            &filesplitoptionsmenu,
            &rec_prerecord_time,
            &clear_rec_directory_item,
#ifdef HAVE_BACKLIGHT
            &cliplight,
#endif
            &rectrigger_item,
#ifdef HAVE_AGC
            &agc_preset, &agc_cliptime,
#endif
            &browse_recconfigs, &save_recpresets_item
);

bool recording_menu(bool no_source)
{
    bool retval;
    no_source_in_menu = no_source;
    retval = do_menu(&recording_settings_menu, NULL, NULL, false) == MENU_ATTACHED_USB;
    no_source_in_menu = false; /* always fall back to the default */
    return retval;
};

MENUITEM_FUNCTION(recording_settings, MENU_FUNC_USEPARAM, ID2P(LANG_RECORDING_SETTINGS),
                          (int (*)(void*))recording_menu, 0, NULL, Icon_Recording);

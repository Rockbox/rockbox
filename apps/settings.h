/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Stuart Martin
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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include <stddef.h>
#include "inttypes.h"
#include "config.h"
#include "audiohw.h" /* for the AUDIOHW_* defines */
#include "statusbar.h" /* for the statusbar values */
#include "quickscreen.h"
#include "button.h"
#if CONFIG_CODEC == SWCODEC
#include "audio.h"
#include "dsp_proc_settings.h"
#endif
#include "rbpaths.h"

struct opt_items {
    unsigned const char* string;
    int32_t voice_id;
};

/** Setting values defines **/
#define MAX_FILENAME 32
#define MAX_PATHNAME 80


enum {
    BOOKMARK_NO = 0,
    BOOKMARK_YES,
    BOOKMARK_ASK,
    BOOKMARK_UNIQUE_ONLY = 2,
    BOOKMARK_RECENT_ONLY_YES,
    BOOKMARK_RECENT_ONLY_ASK,
};

enum
{
    TRIG_MODE_OFF = 0,
    TRIG_MODE_NOREARM,
    TRIG_MODE_REARM
};

enum
{
    TRIG_TYPE_STOP = 0,
    TRIG_TYPE_PAUSE,
    TRIG_TYPE_NEW_FILE
};

#ifdef HAVE_CROSSFADE
enum {
    CROSSFADE_ENABLE_OFF = 0,
    CROSSFADE_ENABLE_AUTOSKIP,
    CROSSFADE_ENABLE_MANSKIP,
    CROSSFADE_ENABLE_SHUFFLE,
    CROSSFADE_ENABLE_SHUFFLE_OR_MANSKIP,
    CROSSFADE_ENABLE_ALWAYS,
};
#endif

enum {
    FOLDER_ADVANCE_OFF = 0,
    FOLDER_ADVANCE_NEXT,
    FOLDER_ADVANCE_RANDOM,
};

/* repeat mode options */
enum
{
    REPEAT_OFF = 0,
    REPEAT_ALL,
    REPEAT_ONE,
    REPEAT_SHUFFLE,
#ifdef AB_REPEAT_ENABLE
    REPEAT_AB,
#endif
    NUM_REPEAT_MODES
};


/* dir filter options */
/* Note: Any new filter modes need to be added before NUM_FILTER_MODES.
 *       Any new rockbox browse filter modes (accessible through the menu)
 *       must be added after NUM_FILTER_MODES. */
enum { SHOW_ALL, SHOW_SUPPORTED, SHOW_MUSIC, SHOW_PLAYLIST, SHOW_ID3DB,
       NUM_FILTER_MODES,
       SHOW_WPS, SHOW_RWPS, SHOW_FMS, SHOW_RFMS, SHOW_SBS, SHOW_RSBS, SHOW_FMR, SHOW_CFG,
       SHOW_LNG, SHOW_MOD, SHOW_FONT, SHOW_PLUGINS, SHOW_M3U};

/* file and dir sort options */
enum { SORT_ALPHA, SORT_DATE, SORT_DATE_REVERSED, SORT_TYPE, /* available as settings */
       SORT_ALPHA_REVERSED, SORT_TYPE_REVERSED };            /* internal use only */
enum { SORT_INTERPRET_AS_DIGIT, SORT_INTERPRET_AS_NUMBER };

/* recursive dir insert options */
enum { RECURSE_OFF, RECURSE_ON, RECURSE_ASK };

/* show path types */
enum { SHOW_PATH_OFF = 0, SHOW_PATH_CURRENT, SHOW_PATH_FULL };

/* scrollbar visibility/position */
enum { SCROLLBAR_OFF = 0, SCROLLBAR_LEFT, SCROLLBAR_RIGHT };

/* autoresume settings */
enum { AUTORESUME_NEXTTRACK_NEVER = 0, AUTORESUME_NEXTTRACK_ALWAYS,
       AUTORESUME_NEXTTRACK_CUSTOM};

/* Alarm settings */
#ifdef HAVE_RTC_ALARM
enum {  ALARM_START_WPS = 0,
#if CONFIG_TUNER
        ALARM_START_FM,
#endif
#ifdef HAVE_RECORDING
        ALARM_START_REC,
#endif
        ALARM_START_COUNT
    };
#if CONFIG_TUNER && defined(HAVE_RECORDING)
#define ALARM_SETTING_TEXT "wps,fm,rec"
#elif CONFIG_TUNER
#define ALARM_SETTING_TEXT "wps,fm"
#elif defined(HAVE_RECORDING)
#define ALARM_SETTING_TEXT "wps,rec"
#endif

#endif /* HAVE_RTC_ALARM */

/* Keyclick stuff */

 /* Not really a setting but several files should stay synced */
#define KEYCLICK_DURATION 2

/** virtual pointer stuff.. move to another .h maybe? **/
/* These define "virtual pointers", which could either be a literal string,
   or a mean a string ID if the pointer is in a certain range.
   This helps to save space for menus and options. */

#define VIRT_SIZE 0xFFFF /* more than enough for our string ID range */
#if defined(CPU_S5L870X)
/* the S5L870X has IRAM at 0, so we use 0xffff bytes right after that */
#define VIRT_PTR ((unsigned char*)0x40000)
#elif CONFIG_CPU==DM320
/* the DM320 has IRAM at 0, so we use 0xffff bytes right after that */
#define VIRT_PTR ((unsigned char*)0x4000)
#else
/* a location where we won't store strings, 0 is the fastest */
#define VIRT_PTR ((unsigned char*)0)
#endif

/* form a "virtual pointer" out of a language ID */
#define ID2P(id) (VIRT_PTR + id)

/* resolve a pointer which could be a virtualized ID or a literal */
#define P2STR(p) (char *)((p>=VIRT_PTR && p<VIRT_PTR+VIRT_SIZE) ? str(p-VIRT_PTR) : p)

/* get the string ID from a virtual pointer, -1 if not virtual */
#define P2ID(p) ((p>=VIRT_PTR && p<VIRT_PTR+VIRT_SIZE) ? p-VIRT_PTR : -1)

/* !defined(HAVE_LCD_COLOR) implies HAVE_LCD_CONTRAST with default 40.
   Explicitly define HAVE_LCD_CONTRAST in config file for newer ports for
   simplicity. */



/** function prototypes **/

/* argument bits for settings_load() */
#define SETTINGS_RTC (BIT_N(0)) /* only the settings from the RTC nonvolatile RAM */
#define SETTINGS_HD  (BIT_N(1)) /* only the settings from the disk sector */
#define SETTINGS_ALL (SETTINGS_RTC|SETTINGS_HD) /* both */
void settings_load(int which) INIT_ATTR;
bool settings_load_config(const char* file, bool apply);

void status_save(void);
int settings_save(void);
void reset_runtime(void);
/* defines for the options paramater */
enum {
    SETTINGS_SAVE_CHANGED = 0,
    SETTINGS_SAVE_ALL,
    SETTINGS_SAVE_THEME,
    SETTINGS_SAVE_SOUND,
#ifdef HAVE_RECORDING
    SETTINGS_SAVE_RECPRESETS,
#endif
#if CONFIG_CODEC == SWCODEC
    SETTINGS_SAVE_EQPRESET,
#endif
};
bool settings_save_config(int options);

struct settings_list;
void reset_setting(const struct settings_list *setting, void *var);
void settings_reset(void);
void sound_settings_apply(void);

/* call this after loading a .wps/.rwps or other skin files, so that the
 * skin buffer is reset properly
 */
void settings_apply_skins(void);

void settings_apply(bool read_disk);
void settings_apply_pm_range(void);
void settings_display(void);

enum optiontype { INT, BOOL };

const struct settings_list* find_setting(const void* variable, int *id);
const struct settings_list* find_setting_by_cfgname(const char* name, int *id);
bool cfg_int_to_string(int setting_id, int val, char* buf, int buf_len);
bool cfg_string_to_int(int setting_id, int* out, const char* str);
bool cfg_to_string(int setting_id, char* buf, int buf_len);
bool set_bool_options(const char* string, const bool* variable,
                      const char* yes_str, int yes_voice,
                      const char* no_str, int no_voice,
                      void (*function)(bool));

bool set_bool(const char* string, const bool* variable);
bool set_int(const unsigned char* string, const char* unit, int voice_unit,
             const int* variable,
             void (*function)(int), int step, int min, int max,
             const char* (*formatter)(char*, size_t, int, const char*) );

/* use this one if you need to create a lang from the value (i.e with TALK_ID()) */
bool set_int_ex(const unsigned char* string, const char* unit, int voice_unit,
             const int* variable,
             void (*function)(int), int step, int min, int max,
             const char* (*formatter)(char*, size_t, int, const char*),
             int32_t (*get_talk_id)(int, int));

void set_file(const char* filename, char* setting, const int maxlen);

bool set_option(const char* string, const void* variable, enum optiontype type,
                const struct opt_items* options, int numoptions, void (*function)(int));



/** global_settings and global_status struct definitions **/

struct system_status
{
    int resume_index;  /* index in playlist (-1 for no active resume) */
    uint32_t resume_crc32; /* crc32 of the name of the file */
    uint32_t resume_elapsed; /* elapsed time in last file */
    uint32_t resume_offset; /* byte offset in mp3 file */
    int runtime;       /* current runtime since last charge */
    int topruntime;    /* top known runtime */
#ifdef HAVE_DIRCACHE
    int dircache_size;      /* directory cache structure last size, 22 bits */
#endif
#if CONFIG_TUNER
    int last_frequency;  /* Last frequency for resuming, in FREQ_STEP units,
                            relative to MIN_FREQ */
#endif
    signed char last_screen;
    int  viewer_icon_count;
    int last_volume_change; /* tick the last volume change happened. skins use this */
#ifdef HAVE_LCD_BITMAP
    int font_id[NB_SCREENS]; /* font id of the settings font for each screen */
#endif

};

struct user_settings
{
    /* audio settings */

    int volume;     /* audio output volume in decibels range depends on the dac */
    int balance;    /* stereo balance: -100 - +100 -100=left  0=bal +100=right  */
    int bass;       /* bass boost/cut in decibels                               */
    int treble;     /* treble boost/cut in decibels                             */
    int channel_config; /* Stereo, Mono, Custom, Mono left, Mono right, Karaoke */
    int stereo_width; /* 0-255% */

#if CONFIG_CODEC != SWCODEC
    int  loudness;      /* loudness eq:          0-100 0=off   100=max         */
    int  avc;           /* auto volume correct:  0=off, 1=20ms, 2=2s 3=4s 4=8s */
    int  mdb_strength;  /* 0-127dB */
    int  mdb_harmonics; /* 0-100% */
    int  mdb_center;    /* 20-300Hz */
    int  mdb_shape;     /* 50-300Hz */
    bool mdb_enable;    /* true/false */
    bool superbass;     /* true/false */
#endif

#ifdef AUDIOHW_HAVE_BASS_CUTOFF
    int bass_cutoff;
#endif
#ifdef AUDIOHW_HAVE_TREBLE_CUTOFF
    int treble_cutoff;
#endif

#if CONFIG_CODEC == SWCODEC
#ifdef HAVE_CROSSFADE
    /* Crossfade */
    int crossfade;     /* Enable crossfade (0=off, 1=shuffle, 2=trackskip,
                                            3=shuff&trackskip, 4=always) */
    int crossfade_fade_in_delay;      /* Fade in delay (0-15s)             */
    int crossfade_fade_out_delay;     /* Fade out delay (0-15s)            */
    int crossfade_fade_in_duration;   /* Fade in duration (0-15s)          */
    int crossfade_fade_out_duration;  /* Fade out duration (0-15s)         */
    int crossfade_fade_out_mixmode;   /* Fade out mode (0=crossfade,1=mix) */
#endif

    /* Replaygain */
    struct replaygain_settings replaygain_settings;

    /* Crossfeed */
    int crossfeed;                              /* crossfeed type */
    unsigned int crossfeed_direct_gain;         /* dB x 10 */
    unsigned int crossfeed_cross_gain;          /* dB x 10 */
    unsigned int crossfeed_hf_attenuation;      /* dB x 10 */
    unsigned int crossfeed_hf_cutoff;           /* Frequency in Hz */

    /* EQ */
    bool eq_enabled;            /* Enable equalizer */
    unsigned int eq_precut;     /* dB */
    struct eq_band_setting eq_band_settings[EQ_NUM_BANDS]; /* for each band */

    /* Misc. swcodec */
    int  beep;              /* system beep volume when changing tracks etc. */
    int  keyclick;          /* keyclick volume */
    int  keyclick_repeats;  /* keyclick on repeats */
    bool dithering_enabled;
#ifdef HAVE_PITCHCONTROL
    bool timestretch_enabled;
#endif
#endif /* CONFIG_CODEC == SWCODEC */

#ifdef HAVE_RECORDING
#if CONFIG_CODEC == SWCODEC
    int rec_format;    /* record format index */
    int rec_mono_mode; /* how to create mono: L, R, L+R */

    /* Encoder Settings Start - keep these together */
    struct mp3_enc_config     mp3_enc_config;
#if 0 /* These currently contain no members but their places in line
         should be held */
    struct aiff_enc_config    aiff_enc_config;
    struct wav_enc_config     wav_enc_config;
    struct wavpack_enc_config wavpack_enc_config;
#endif
    /* Encoder Settings End */

#else
    int rec_quality;   /* 0-7 */
#endif  /* CONFIG_CODEC == SWCODEC */
    int rec_source;    /* 0=mic, 1=line, 2=S/PDIF, 2 or 3=FM Radio */
    int rec_frequency; /* 0 = 44.1kHz (depends on target)
                          1 = 48kHz
                          2 = 32kHz
                          3 = 22.05kHz
                          4 = 24kHz
                          5 = 16kHz */
    int rec_channels;  /* 0=Stereo, 1=Mono */

    int rec_mic_gain;   /* depends on target */
    int rec_left_gain;  /* depends on target */
    int rec_right_gain; /* depends on target */
    bool peak_meter_clipcounter; /* clipping count indicator */
    bool rec_editable; /* true means that the bit reservoir is off */

    /* note: timesplit setting is not saved */
    int rec_timesplit; /* 0 = off,
                          1 = 00:05, 2 = 00:10, 3 = 00:15, 4 = 00:30
                          5 = 01:00, 6 = 02:00, 7 = 04:00, 8 = 06:00
                          9 = 08:00, 10= 10:00, 11= 12:00, 12= 18:00,
                          13= 24:00 */
    int rec_sizesplit; /* 0 = off,
                          1 = 5MB, 2 = 10MB, 3 = 15MB, 4 = 32MB
                          5 = 64MB, 6 = 75MB, 7 = 100MB, 8 = 128MB
                          9 = 256MB, 10= 512MB, 11= 650MB, 12= 700MB,
                          13= 1GB, 14 = 1.5GB 15 = 1.75MB*/
    int rec_split_type; /* split/stop */
    int rec_split_method; /* time/filesize */

    int rec_prerecord_time; /* In seconds, 0-30, 0 means OFF */
    char rec_directory[MAX_PATHNAME+1];
    int cliplight; /* 0 = off
                      1 = main lcd
                      2 = main and remote lcd
                      3 = remote lcd */

    int rec_start_thres_db;
    int rec_start_thres_linear;
    int rec_start_duration; /* index of trig_durations */
    int rec_stop_thres_db;
    int rec_stop_thres_linear;
    int rec_stop_postrec;
    int rec_stop_gap;       /* index of trig_durations */
    int rec_trigger_mode;   /* see TRIG_MODE_XXX constants */
    int rec_trigger_type;   /* what to do when trigger released */
#ifdef HAVE_HISTOGRAM
    int histogram_interval; /* recording peakmeter histogram */
#endif

#ifdef HAVE_AGC
    int rec_agc_preset_mic; /* AGC mic preset modes:
                             0 = Off
                             1 = Safety (clip)
                             2 = Live (slow)
                             3 = DJ-Set (slow)
                             4 = Medium
                             5 = Voice (fast) */
    int rec_agc_preset_line; /* AGC line-in preset modes:
                              0 = Off
                              1 = Safety (clip)
                              2 = Live (slow)
                              3 = DJ-Set (slow)
                              4 = Medium
                              5 = Voice (fast) */
    int rec_agc_maxgain_mic;  /* AGC maximum mic gain */
    int rec_agc_maxgain_line; /* AGC maximum line-in gain */
    int rec_agc_cliptime;     /* 0.2, 0.4, 0.6, 0.8, 1s */
#endif
#endif /* HAVE_RECORDING */

#if CONFIG_TUNER
    int fm_region;
    bool fm_force_mono;  /* Forces Mono mode if true */
    unsigned char fmr_file[MAX_FILENAME+1]; /* last fmr preset */
    unsigned char fms_file[MAX_FILENAME+1];  /* last fms */
#ifdef HAVE_REMOTE_LCD
    unsigned char rfms_file[MAX_FILENAME+1];  /* last remote-fms */
#endif
#endif /* CONFIG_TUNER */

    /* misc options */
#ifndef HAVE_WHEEL_ACCELERATION
    int list_accel_start_delay; /* ms before we start increaseing step size */
    int list_accel_wait; /* ms between increases */
#endif

#ifdef HAVE_TOUCHPAD_SENSITIVITY_SETTING
    int touchpad_sensitivity;
#endif

#ifdef HAVE_TOUCHPAD_DEADZONE
    int touchpad_deadzone;
#endif

    int  pause_rewind; /* time in s to rewind when pausing */
#ifdef HAVE_HEADPHONE_DETECTION
    int  unplug_mode; /* pause on headphone unplug */
    bool unplug_autoresume; /* disable auto-resume if no phones */
#endif

#ifdef HAVE_QUICKSCREEN
    int qs_items[QUICKSCREEN_ITEM_COUNT];
#endif

#if CONFIG_RTC
    int timeformat;    /* time format: 0=24 hour clock, 1=12 hour clock */
#endif

#ifdef HAVE_DISK_STORAGE
    int disk_spindown; /* time until disk spindown, in seconds (0=off) */
    int buffer_margin; /* audio buffer watermark margin, in seconds */
#endif

    int dirfilter;     /* 0=display all, 1=only supported, 2=only music,
                          3=dirs+playlists, 4=ID3 database */
    int show_filename_ext; /* show filename extensions in file browser?
                              0 = no, 1 = yes, 2 = only unknown 0 */
    int default_codepage;   /* set default codepage for tag conversion */
    bool hold_lr_for_scroll_in_list; /* hold L/R scrolls the list left/right */
    bool play_selected; /* Plays selected file even in shuffle mode */
    bool party_mode;    /* party mode - unstoppable music */
    bool audioscrobbler; /* Audioscrobbler logging  */
    bool cuesheet;
    bool car_adapter_mode; /* 0=off 1=on */
    int start_in_screen;
#if defined(HAVE_RTC_ALARM) && \
    (defined(HAVE_RECORDING) || CONFIG_TUNER)
    int alarm_wake_up_screen;
#endif
    int ff_rewind_min_step; /* FF/Rewind minimum step size */
    int ff_rewind_accel; /* FF/Rewind acceleration (in seconds per doubling) */

    int peak_meter_release;   /* units per read out */
    int peak_meter_hold;      /* hold time for peak meter in 1/100 s */
    int peak_meter_clip_hold; /* hold time for clips */
    bool peak_meter_dbfs;     /* show linear or dbfs values */
    int peak_meter_min; /* range minimum */
    int peak_meter_max; /* range maximum */

    unsigned char wps_file[MAX_FILENAME+1];  /* last wps */
#ifdef HAVE_LCD_BITMAP
    unsigned char sbs_file[MAX_FILENAME+1];  /* last statusbar skin */
#endif
#ifdef HAVE_REMOTE_LCD
    unsigned char rwps_file[MAX_FILENAME+1];  /* last remote-wps */
    unsigned char rsbs_file[MAX_FILENAME+1];  /* last remote statusbar skin */
#endif
    unsigned char lang_file[MAX_FILENAME+1]; /* last language */
    unsigned char playlist_catalog_dir[MAX_PATHNAME+1];
    int skip_length; /* skip length */
    int max_files_in_dir; /* Max entries in directory (file browser) */
    int max_files_in_playlist; /* Max entries in playlist */
    int volume_type;   /* how volume is displayed: 0=graphic, 1=percent */
    int battery_display; /* how battery is displayed: 0=graphic, 1=percent */
    bool show_icons;   /* 0=hide 1=show */
    int statusbar;    /* STATUSBAR_* enum values */
#ifdef HAVE_REMOTE_LCD
    int remote_statusbar;
#endif

#if CONFIG_KEYPAD == RECORDER_PAD
    bool buttonbar;    /* 0=hide, 1=show */
#endif

#ifdef HAVE_LCD_BITMAP
    int scrollbar;    /* SCROLLBAR_* enum values */
    int scrollbar_width;

#ifdef HAVE_TOUCHSCREEN
    int list_line_padding;
#endif
#if LCD_DEPTH > 1
    int list_separator_height; /* -1=auto (== 1 currently), 0=disabled, X=height in pixels */
    int list_separator_color;
#endif
#endif
    /* goto current song when exiting WPS */
    bool browse_current; /* 1=goto current song,
                            0=goto previous location */
    bool scroll_paginated; /* 0=dont 1=do */
    int  scroll_speed;     /* long texts scrolling speed: 1-30 */
    int  bidir_limit;      /* bidir scroll length limit */
    int  scroll_delay;     /* delay (in 1/10s) before starting scroll */
    int  scroll_step;      /* pixels to advance per update */

    /* auto bookmark settings */
    int autoloadbookmark;   /* auto load option: 0=off, 1=ask, 2=on */
    int autocreatebookmark; /* auto create option: 0=off, 1=ask, 2=on */
    bool autoupdatebookmark;/* auto update option */
    int usemrb;             /* use MRB list: 0=No, 1=Yes, 2=One per playlist */

#ifdef HAVE_DIRCACHE
    bool dircache;          /* enable directory cache */
#endif
#ifdef HAVE_TAGCACHE
#ifdef HAVE_TC_RAMCACHE
    bool tagcache_ram;        /* load tagcache to ram? */
#endif
    bool tagcache_autoupdate; /* automatically keep tagcache in sync? */
    bool autoresume_enable;   /* enable auto-resume feature? */
    int autoresume_automatic; /* resume next track? 0=never, 1=always,
                                 2=custom */
    unsigned char autoresume_paths[MAX_PATHNAME+1]; /* colon-separated list */
    bool runtimedb;           /* runtime database active? */
    unsigned char tagcache_scan_paths[MAX_PATHNAME+1];
#endif /* HAVE_TAGCACHE */

#if LCD_DEPTH > 1
    unsigned char backdrop_file[MAX_PATHNAME+1];  /* backdrop bitmap file */
#endif

#ifdef HAVE_LCD_COLOR
    int bg_color; /* background color native format */
    int fg_color; /* foreground color native format */
    int lss_color; /* background color for the selector or start color for the gradient */
    int lse_color; /* end color for the selector gradient */
    int lst_color; /* color of the text for the selector */
    unsigned char colors_file[MAX_FILENAME+1];
#endif

    /* playlist/playback settings */
    int  repeat_mode; /* 0=off 1=repeat all 2=repeat one 3=shuffle 4=ab */
    int  next_folder; /* move to next folder */
    bool constrain_next_folder; /* whether next_folder is constrained to
                                   directories within start_directory */
    int  recursive_dir_insert; /* should directories be inserted recursively */
    bool fade_on_stop; /* fade on pause/unpause/stop */
    bool playlist_shuffle;
    bool warnon_erase_dynplaylist; /* warn when erasing dynamic playlist */

    /* playlist viewer settings */
    bool playlist_viewer_icons; /* display icons on viewer */
    bool playlist_viewer_indices; /* display playlist indices on viewer */
    int playlist_viewer_track_display; /* how to display tracks in viewer */

    /* voice UI settings */
    bool talk_menu; /* enable voice UI */
    int talk_dir; /* voiced directories mode: 0=off 1=number 2=spell */
    bool talk_dir_clip; /* use directory .talk clips */
    int talk_file; /* voice file mode: 0=off, 1=number, 2=spell */
    bool talk_file_clip; /* use file .talk clips */
    bool talk_filetype; /* say file type */
    bool talk_battery_level;

    /* file browser sorting */
    bool sort_case; /* dir sort order: 0=case insensitive, 1=sensitive */
    int sort_dir;   /* 0=alpha, 1=date (old first), 2=date (new first) */
    int sort_file;  /* 0=alpha, 1=date, 2=date (new first), 3=type */
    int interpret_numbers; /* true=strnatcmp, false=strcmp */

    /* power settings */
    int poweroff;   /* idle power off timer */
#if BATTERY_CAPACITY_DEFAULT > 0
    int battery_capacity; /* in mAh */
#endif

#if BATTERY_TYPES_COUNT > 1
    int battery_type;  /* for units which can take multiple types (Ondio). */
#endif
#ifdef HAVE_SPDIF_POWER
    bool spdif_enable; /* S/PDIF power on/off */
#endif
#ifdef HAVE_USB_CHARGING_ENABLE
    int usb_charging;
#endif
#ifdef HAVE_USB_POWER
    bool usb_charge_only;
#endif    
    /* device settings */
#ifdef HAVE_LCD_CONTRAST
    int contrast;   /* lcd contrast */
#endif

#ifdef HAVE_LCD_BITMAP
#ifdef HAVE_LCD_INVERT
    bool invert;    /* invert display */
#endif
#ifdef HAVE_LCD_FLIP
    bool flip_display; /* turn display (and button layout) by 180 degrees */
#endif
    int  cursor_style; /* style of the selection cursor */
    int  screen_scroll_step;
    int  show_path_in_browser; /* 0=off, 1=current directory, 2=full path */
    bool offset_out_of_view;
    unsigned char icon_file[MAX_FILENAME+1];
    unsigned char viewers_icon_file[MAX_FILENAME+1];
    unsigned char font_file[MAX_FILENAME+1]; /* last font */
    int glyphs_to_cache; /* default font allocation size in glyphs */
#ifdef HAVE_REMOTE_LCD
    unsigned char remote_font_file[MAX_FILENAME+1]; /* last font */
#endif
    unsigned char kbd_file[MAX_FILENAME+1];  /* last keyboard */
#endif /* HAVE_LCD_BITMAP */
    int  backlight_timeout;  /* backlight off timeout:  0-18 0=never,
                               1=always,
                               then according to timeout_values[] */
    bool caption_backlight; /* turn on backlight at end and start of track */
    bool bl_filter_first_keypress;   /* filter first keypress when dark? */
#if CONFIG_CHARGING
    int backlight_timeout_plugged;
#endif
#ifndef HAS_BUTTON_HOLD
    bool bt_selective_softlock_actions;
    int bt_selective_softlock_actions_mask;
#endif
#ifdef HAVE_BACKLIGHT
    bool bl_selective_actions; /* backlight disable on some actions */
    int  bl_selective_actions_mask;/* mask of actions that will not enable backlight */
#ifdef HAS_BUTTON_HOLD
    int backlight_on_button_hold; /* what to do with backlight when hold
                                     switch is on */
#endif
#ifdef HAVE_LCD_SLEEP_SETTING
    int lcd_sleep_after_backlight_off; /* when to put lcd to sleep after backlight
                                          has turned off */
#endif
#endif /* HAVE_BACKLIGHT */

#if defined(HAVE_BACKLIGHT_FADING_INT_SETTING)
    int backlight_fade_in;  /* backlight fade in timing: 0..3 */
    int backlight_fade_out; /* backlight fade in timing: 0..7 */
#elif defined(HAVE_BACKLIGHT_FADING_BOOL_SETTING)
    bool backlight_fade_in;
    bool backlight_fade_out;
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS
    int brightness;
#endif

#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    int  remote_contrast;          /* lcd contrast:       0-63 0=low 63=high */
    int  remote_backlight_timeout; /* backlight off timeout:  0-18 0=never,
                                   1=always, then according to timeout_values[] */
    int  remote_backlight_timeout_plugged;
    int  remote_scroll_speed;      /* long texts scrolling speed: 1-30 */
    int  remote_scroll_delay;      /* delay (in 1/10s) before starting scroll */
    int  remote_scroll_step;       /* pixels to advance per update */
    int  remote_bidir_limit;       /* bidir scroll length limit */
    bool remote_invert;            /* invert display */
    bool remote_flip_display;      /* turn display (and button layout) by 180 degrees */
    bool remote_caption_backlight; /* turn on backlight at end and start of track */
    bool remote_bl_filter_first_keypress; /* filter first remote keypress when remote dark? */
    unsigned char remote_icon_file[MAX_FILENAME+1];
    unsigned char remote_viewers_icon_file[MAX_FILENAME+1];
#ifdef HAS_REMOTE_BUTTON_HOLD
    int remote_backlight_on_button_hold; /* what to do with remote backlight when hold
                                            switch is on */
#endif
#ifdef HAVE_REMOTE_LCD_TICKING
    bool remote_reduce_ticking; /* 0=normal operation,
                                   1=EMI reduce on with cost more CPU. */
#endif
#endif /* HAVE_REMOTE_LCD */

#if CONFIG_CODEC == MAS3507D
    bool line_in;       /* false=off, true=active */
#endif

#ifdef HAVE_BUTTON_LIGHT
    int buttonlight_timeout;
#endif
#ifdef HAVE_BUTTONLIGHT_BRIGHTNESS
    int buttonlight_brightness;
#endif

#ifdef IPOD_ACCESSORY_PROTOCOL
    int serial_bitrate; /* 0=auto 1=9600 2=19200 3=38400 4=57600 */
#endif
#ifdef HAVE_ACCESSORY_SUPPLY
    bool accessory_supply; /* 0=off 1=on, accessory power supply for iPod */
#endif
#ifdef HAVE_LINEOUT_POWEROFF
    bool lineout_active;
#endif

#ifdef HAVE_SPEAKER
    int speaker_mode; /* 0: off, 1: on, 2: auto (only if headphone detection) */
#endif /* HAVE_SPEAKER */
    bool prevent_skip;

#ifdef HAVE_TOUCHSCREEN
    int touch_mode;
    struct touchscreen_parameter ts_calibration_data;
#endif

#ifdef HAVE_PITCHCONTROL
    /* pitch screen settings */
    bool pitch_mode_semitone;
#if CONFIG_CODEC == SWCODEC
    bool pitch_mode_timestretch;
#endif
#endif
    /* If values are just added to the end, no need to bump plugin API
       version. */
    /* new stuff to be added at the end */

#ifdef USB_ENABLE_HID
    bool usb_hid;
    int usb_keypad_mode;
#endif

#if defined(USB_ENABLE_STORAGE) && defined(HAVE_MULTIDRIVE)
    bool usb_skip_first_drive;
#endif

#ifdef HAVE_LCD_BITMAP
    unsigned char ui_vp_config[64]; /* viewport string for the lists */
#ifdef HAVE_REMOTE_LCD
    unsigned char remote_ui_vp_config[64]; /* viewport string for the remote lists */
#endif
#endif

#if CONFIG_CODEC == SWCODEC
    struct compressor_settings compressor_settings;
#endif

    int sleeptimer_duration; /* In minutes; 0=off */
    bool sleeptimer_on_startup;
    bool keypress_restarts_sleeptimer;

#ifdef HAVE_MORSE_INPUT
    bool morse_input; /* text input method setting */
#endif

#ifdef HAVE_HOTKEY
    /* hotkey assignments - acceptable values are in
       hotkey_action enum in onplay.h */
    int hotkey_wps;
    int hotkey_tree;
#endif

#if CONFIG_CODEC == SWCODEC
    /* When resuming playback (after a stop), rewind this number of seconds */
    int resume_rewind;
#endif

#ifdef AUDIOHW_HAVE_DEPTH_3D
    int depth_3d;
#endif

#ifdef AUDIOHW_HAVE_FILTER_ROLL_OFF
    int roll_off;
#endif

#ifdef AUDIOHW_HAVE_FUNCTIONAL_MODE
    int func_mode;
#endif

#ifdef AUDIOHW_HAVE_EQ
    /** Hardware EQ tone controls **/
    struct hw_eq_band
    {
        /* Maintain the order of members or sound_menu has to be changed */
        int gain;
#ifdef AUDIOHW_HAVE_EQ_FREQUENCY
        int frequency;
#endif
#ifdef AUDIOHW_HAVE_EQ_WIDTH
        int width;
#endif
    } hw_eq_bands[AUDIOHW_EQ_BAND_NUM];
#endif /* AUDIOHW_HAVE_EQ */

#ifdef HAVE_HARDWARE_CLICK
#if CONFIG_CODEC == SWCODEC
    bool keyclick_hardware; /* hardware piezo keyclick */
#endif
#endif

    char start_directory[MAX_PATHNAME+1];
    /* Has the root been customized from the .cfg file? false = no, true = loaded from cfg */
    bool root_menu_customized;
#ifdef HAVE_QUICKSCREEN
    bool shortcuts_replaces_qs;
#endif

#ifdef HAVE_PLAY_FREQ
    int play_frequency; /* core audio output frequency selection */
#endif
    int volume_limit; /* maximum volume limit */

    int surround_enabled;
    int surround_balance;
    int surround_fx1;
    int surround_fx2;
    bool surround_method2;
    int surround_mix;

    int pbe;
    int pbe_precut;

    int afr_enabled;

#if defined(DX50) || defined(DX90)
    int governor;
    int usb_mode;
#endif
};

/** global variables **/
extern long lasttime;
/* global settings */
extern struct user_settings global_settings;
/* global status */
extern struct system_status global_status;

#endif /* __SETTINGS_H__ */

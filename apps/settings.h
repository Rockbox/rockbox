/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdbool.h>
#include "config.h"
#include "file.h"
#include "dircache.h"
#include "timefuncs.h"

#ifdef HAVE_BACKLIGHT_BRIGHTNESS
#include "backlight.h" /* for [MIN|MAX]_BRIGHTNESS_SETTING */
#endif

#define ROCKBOX_DIR "/.rockbox"
#define ROCKBOX_DIR_LEN 9
#define FONT_DIR "/fonts"
#define LANG_DIR "/langs"
#define WPS_DIR ROCKBOX_DIR "/wps"
#define THEME_DIR ROCKBOX_DIR "/themes"
#define PLUGIN_DIR ROCKBOX_DIR"/rocks"
#define BACKDROP_DIR ROCKBOX_DIR"/backdrops"
#define REC_BASE_DIR "/recordings"

#define MAX_FILENAME 20

/* button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
  (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SETTINGS_INC     BUTTON_UP
#define SETTINGS_DEC     BUTTON_DOWN
#define SETTINGS_OK      BUTTON_SELECT
#define SETTINGS_OK2     BUTTON_LEFT
#define SETTINGS_CANCEL  BUTTON_OFF
#define SETTINGS_CANCEL2 BUTTON_MODE
#define SETTINGS_PREV    BUTTON_LEFT
#define SETTINGS_NEXT    BUTTON_RIGHT
#define SETTINGS_ACCEPT  BUTTON_ON

#elif CONFIG_KEYPAD == RECORDER_PAD
#define SETTINGS_INC     BUTTON_UP
#define SETTINGS_DEC     BUTTON_DOWN
#define SETTINGS_OK      BUTTON_PLAY
#define SETTINGS_OK2     BUTTON_LEFT
#define SETTINGS_CANCEL  BUTTON_OFF
#define SETTINGS_CANCEL2 BUTTON_F1
#define SETTINGS_PREV    BUTTON_LEFT
#define SETTINGS_NEXT    BUTTON_RIGHT
#define SETTINGS_ACCEPT  BUTTON_ON

#elif CONFIG_KEYPAD == PLAYER_PAD
#define SETTINGS_INC     BUTTON_RIGHT
#define SETTINGS_DEC     BUTTON_LEFT
#define SETTINGS_OK      BUTTON_PLAY
#define SETTINGS_CANCEL  BUTTON_STOP
#define SETTINGS_CANCEL2 BUTTON_MENU

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SETTINGS_INC     BUTTON_UP
#define SETTINGS_DEC     BUTTON_DOWN
#define SETTINGS_OK      BUTTON_RIGHT
#define SETTINGS_OK2     BUTTON_LEFT
#define SETTINGS_CANCEL  BUTTON_MENU
#define SETTINGS_CANCEL2 BUTTON_OFF

#elif CONFIG_KEYPAD == GMINI100_PAD
#define SETTINGS_INC     BUTTON_UP
#define SETTINGS_DEC     BUTTON_DOWN
#define SETTINGS_OK      BUTTON_PLAY
#define SETTINGS_OK2     BUTTON_LEFT
#define SETTINGS_CANCEL  BUTTON_OFF
#define SETTINGS_CANCEL2 BUTTON_MENU
#define SETTINGS_PREV    BUTTON_LEFT
#define SETTINGS_NEXT    BUTTON_RIGHT
#define SETTINGS_ACCEPT  BUTTON_MENU

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define SETTINGS_INC     BUTTON_SCROLL_FWD
#define SETTINGS_DEC     BUTTON_SCROLL_BACK
#define SETTINGS_OK      BUTTON_SELECT
#define SETTINGS_CANCEL  BUTTON_MENU
#define SETTINGS_PREV    BUTTON_LEFT
#define SETTINGS_NEXT    BUTTON_RIGHT
#define SETTINGS_ACCEPT  BUTTON_PLAY

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define SETTINGS_INC     BUTTON_RIGHT
#define SETTINGS_DEC     BUTTON_LEFT
#define SETTINGS_OK      BUTTON_SELECT
#define SETTINGS_CANCEL  BUTTON_PLAY

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define SETTINGS_INC     BUTTON_UP
#define SETTINGS_DEC     BUTTON_DOWN
#define SETTINGS_OK      BUTTON_MENU
#define SETTINGS_CANCEL  BUTTON_REC
#define SETTINGS_PREV    BUTTON_LEFT
#define SETTINGS_NEXT    BUTTON_RIGHT
#define SETTINGS_ACCEPT  BUTTON_PLAY

#endif

/* data structures */

#define BOOKMARK_NO  0
#define BOOKMARK_YES 1
#define BOOKMARK_ASK 2
#define BOOKMARK_UNIQUE_ONLY 2
#define BOOKMARK_RECENT_ONLY_YES 3
#define BOOKMARK_RECENT_ONLY_ASK 4
#define FF_REWIND_1000   0
#define FF_REWIND_2000   1
#define FF_REWIND_3000   2
#define FF_REWIND_4000   3
#define FF_REWIND_5000   4
#define FF_REWIND_6000   5
#define FF_REWIND_8000   6
#define FF_REWIND_10000  7
#define FF_REWIND_15000  8
#define FF_REWIND_20000  9
#define FF_REWIND_25000 10
#define FF_REWIND_30000 11
#define FF_REWIND_45000 12
#define FF_REWIND_60000 13

#define TRIG_MODE_OFF 0
#define TRIG_MODE_NOREARM 1
#define TRIG_MODE_REARM 2

#define TRIG_DURATION_COUNT 13
extern const char * const trig_durations[TRIG_DURATION_COUNT];

#define CROSSFADE_ENABLE_SHUFFLE  1
#define CROSSFADE_ENABLE_ALWAYS   2

/* These define "virtual pointers", which could either be a literal string,
   or a mean a string ID if the pointer is in a certain range.
   This helps to save space for menus and options. */

#define VIRT_SIZE 0xFFFF /* more than enough for our string ID range */
#ifdef SIMULATOR
/* a space which is defined in stubs.c */
extern unsigned char vp_dummy[VIRT_SIZE];
#define VIRT_PTR vp_dummy
#else
/* a location where we won't store strings, 0 is the fastest */
#define VIRT_PTR ((unsigned char*)0)
#endif

/* form a "virtual pointer" out of a language ID */
#define ID2P(id) (VIRT_PTR + id)

/* resolve a pointer which could be a virtualized ID or a literal */
#define P2STR(p) (char *)((p>=VIRT_PTR && p<=VIRT_PTR+VIRT_SIZE) ? str(p-VIRT_PTR) : p)

/* get the string ID from a virtual pointer, -1 if not virtual */
#define P2ID(p) ((p>=VIRT_PTR && p<=VIRT_PTR+VIRT_SIZE) ? p-VIRT_PTR : -1)


struct user_settings
{
    /* audio settings */

    int volume;     /* audio output volume:  0-100 0=off   100=max           */
    int balance;    /* stereo balance:       0-100 0=left  50=bal 100=right  */
    int bass;       /* bass eq:              0-100 0=off   100=max           */
    int treble;     /* treble eq:            0-100 0=low   100=high          */
    int loudness;   /* loudness eq:          0-100 0=off   100=max           */
    int avc;        /* auto volume correct:  0=off, 1=20ms, 2=2s 3=4s 4=8s   */
    int channel_config; /* Stereo, Mono, Custom, Mono left, Mono right, Karaoke */
    int stereo_width; /* 0-255% */
    int mdb_strength; /* 0-127dB */
    int mdb_harmonics; /* 0-100% */
    int mdb_center; /* 20-300Hz */
    int mdb_shape; /* 50-300Hz */
    bool mdb_enable; /* true/false */
    bool superbass; /* true/false */

#if CONFIG_CODEC == SWCODEC
    int crossfade;     /* Enable crossfade (0=off,1=shuffle,2=always)      */
    int crossfade_fade_in_delay;      /* Fade in delay (0-15s)             */
    int crossfade_fade_out_delay;     /* Fade out delay (0-15s)            */
    int crossfade_fade_in_duration;   /* Fade in duration (0-15s)          */
    int crossfade_fade_out_duration;  /* Fade out duration (0-15s)         */
    int crossfade_fade_out_mixmode;   /* Fade out mode (0=crossfade,1=mix) */
#endif

    int rec_quality;   /* 0-7 */
    int rec_source;    /* 0=mic, 1=line, 2=S/PDIF */
    int rec_frequency; /* 0 = 44.1kHz
                          1 = 48kHz
                          2 = 32kHz
                          3 = 22.05kHz
                          4 = 24kHz
                          5 = 16kHz */
    int rec_channels;  /* 0=Stereo, 1=Mono */
    int rec_mic_gain; /* 0-15 */
    int rec_left_gain; /* 0-15 */
    int rec_right_gain; /* 0-15 */
#ifdef HAVE_UDA1380
    int rec_adc_left_gain; /* -128 .. 48 */
    int rec_adc_right_gain; /* -128 .. 48 */
#endif    
    bool rec_editable; /* true means that the bit reservoir is off */

    /* note: timesplit setting is not saved */
    int rec_timesplit; /* 0 = off,
                          1 = 00:05, 2 = 00:10, 3 = 00:15, 4 = 00:30
                          5 = 01:00, 6 = 02:00, 7 = 04:00, 8 = 06:00
                          9 = 08:00, 10= 10:00, 11= 12:00, 12= 18:00,
                          13= 24:00 */

    int rec_prerecord_time; /* In seconds, 0-30, 0 means OFF */
    int rec_directory; /* 0=base dir, 1=current dir */
    bool rec_startup; /* true means start Rockbox in recording screen */
    
    int rec_start_thres;    /* negative: db, positive: % range -87 .. 100 */
    int rec_start_duration; /* index of trig_durations */
    int rec_stop_thres;     /* negative: db, positive: % */
    int rec_stop_postrec;   /* negative: db, positive: % range -87 .. 100 */
    int rec_stop_gap;       /* index of trig_durations */
    int rec_trigger_mode;   /* see TRIG_MODE_XXX constants */

    /* device settings */

    int contrast;   /* lcd contrast:          0-63 0=low 63=high            */
    bool invert;    /* invert display */
    bool invert_cursor; /* invert the current file in dir browser and menu
                           instead of using the default cursor */
    bool flip_display; /* turn display (and button layout) by 180 degrees */
    int poweroff;   /* power off timer */
    int backlight_timeout;  /* backlight off timeout:  0-18 0=never,
                               1=always,
                               then according to timeout_values[] */
    int backlight_timeout_plugged;
#if CONFIG_BACKLIGHT == BL_IRIVER_H100
    int backlight_fade_in;  /* backlight fade in timing: 0..3 */
    int backlight_fade_out; /* backlight fade in timing: 0..7 */
#endif
    int battery_capacity; /* in mAh */
    int battery_type;  /* for units which can take multiple types (Ondio). */

#ifdef HAVE_SPDIF_POWER
    bool spdif_enable; /* S/PDIF power on/off */
#endif

    /* resume settings */

    bool resume;        /* resume option: 0=off, 1=on */
    int resume_index;  /* index in playlist (-1 for no active resume) */
    int resume_first_index;  /* index of first track in playlist */
    unsigned long resume_offset; /* byte offset in mp3 file */
    int resume_seed;   /* shuffle seed (-1=no resume shuffle 0=sorted
                          >0=shuffled) */

    unsigned char font_file[MAX_FILENAME+1]; /* last font */
    unsigned char wps_file[MAX_FILENAME+1];  /* last wps */
    unsigned char lang_file[MAX_FILENAME+1]; /* last language */

    /* misc options */

    int repeat_mode;   /* 0=off 1=repeat all 2=repeat one 3=shuffle 4=ab */
    int dirfilter;     /* 0=display all, 1=only supported, 2=only music,
                          3=dirs+playlists, 4=ID3 database */
    bool sort_case;    /* dir sort order: 0=case insensitive, 1=sensitive */
    int volume_type;   /* how volume is displayed: 0=graphic, 1=percent */
    int battery_display; /* how battery is displayed: 0=graphic, 1=percent */
    int timeformat;    /* time format: 0=24 hour clock, 1=12 hour clock */
    int scroll_speed;  /* long texts scrolling speed: 1-30 */
    bool playlist_shuffle;
    bool play_selected; /* Plays selected file even in shuffle mode */
    int ff_rewind_min_step; /* FF/Rewind minimum step size */
    int ff_rewind_accel; /* FF/Rewind acceleration (in seconds per doubling) */
    int disk_spindown; /* time until disk spindown, in seconds (0=off) */
    bool disk_poweroff; /* whether to cut disk power after spindown or not */
    int buffer_margin; /* MP3 buffer watermark margin, in seconds */

    int peak_meter_release;   /* units per read out */
    int peak_meter_hold;      /* hold time for peak meter in 1/100 s */
    int peak_meter_clip_hold; /* hold time for clips */
    bool peak_meter_dbfs;     /* show linear or dbfs values */
    int peak_meter_min; /* range minimum */
    int peak_meter_max; /* range maximum */
    bool car_adapter_mode; /* 0=off 1=on */

    /* show status bar */
    bool statusbar;    /* 0=hide, 1=show */

    /* show button bar */
    bool buttonbar;    /* 0=hide, 1=show */

    /* show scroll bar */
    bool scrollbar;    /* 0=hide, 1=show */

    /* goto current song when exiting WPS */
    bool browse_current; /* 1=goto current song,
                            0=goto previous location */

    int runtime;       /* current runtime since last charge */
    int topruntime;    /* top known runtime */

    int bidir_limit;   /* bidir scroll length limit */
    int scroll_delay;  /* delay (in 1/10s) before starting scroll */
    int scroll_step;   /* pixels to advance per update */
#ifdef HAVE_LCD_BITMAP 
    bool offset_out_of_view;
    int screen_scroll_step;
#endif

    /* auto bookmark settings */
    int autoloadbookmark;   /* auto load option: 0=off, 1=ask, 2=on */
    int autocreatebookmark; /* auto create option: 0=off, 1=ask, 2=on */
    int usemrb;                 /* use MRB list: 0=No, 1=Yes*/
#ifdef HAVE_LCD_CHARCELLS
    int jump_scroll;   /* Fast jump when scrolling */
    int jump_scroll_delay; /* Delay between jump scroll screens */
#endif
    bool fade_on_stop; /* fade on pause/unpause/stop */
    bool caption_backlight; /* turn on backlight at end and start of track */

#ifdef CONFIG_TUNER
    int fm_freq_step;    /* Frequency step for manual tuning, in kHz */
    bool fm_force_mono;  /* Forces Mono mode if true */
    bool fm_full_range;  /* Enables full 10MHz-160MHz range if true, else
                            only 88MHz-108MHz */
    int last_frequency;  /* Last frequency for resuming, in FREQ_STEP units,
                            relative to MIN_FREQ */
#endif

    int max_files_in_dir; /* Max entries in directory (file browser) */
    int max_files_in_playlist; /* Max entries in playlist */
    bool show_icons;   /* 0=hide 1=show */
    int recursive_dir_insert; /* should directories be inserted recursively */

    bool   line_in;       /* false=off, true=active */

    bool id3_v1_first;    /* true = ID3V1 has prio over ID3V2 tag */

    /* playlist viewer settings */
    bool playlist_viewer_icons; /* display icons on viewer */
    bool playlist_viewer_indices; /* display playlist indices on viewer */
    int playlist_viewer_track_display; /* how to display tracks in viewer */

    /* voice UI settings */
    bool talk_menu; /* enable voice UI */
    int talk_dir; /* talkbox mode: 0=off 1=number 2=clip@enter 3=clip@hover */
    int talk_file; /* voice filename mode: 0=off, 1=number, other t.b.d. */ 

    /* file browser sorting */
    int sort_file; /* 0=alpha, 1=date, 2=date (new first), 3=type */
    int sort_dir; /* 0=alpha, 1=date (old first), 2=date (new first) */
    
#ifdef HAVE_REMOTE_LCD
    /* remote lcd */
    int remote_contrast;   /* lcd contrast:          0-63 0=low 63=high            */
    bool remote_invert;    /* invert display */
    bool remote_flip_display; /* turn display (and button layout) by 180 degrees */
    int remote_backlight_timeout;  /* backlight off timeout:  0-18 0=never,
                               1=always,
                               then according to timeout_values[] */
    int remote_backlight_timeout_plugged;
    bool remote_caption_backlight; /* turn on backlight at end and start of track */
#ifdef HAVE_REMOTE_LCD_TICKING
    bool remote_reduce_ticking; /* 0=normal operation,
                                   1=EMI reduce on with cost more CPU. */
#endif
#endif
    
    bool next_folder; /* move to next folder */
    bool runtimedb;   /* runtime database active? */

#if CONFIG_CODEC == SWCODEC
    bool replaygain;        /* enable replaygain */
    bool replaygain_noclip; /* scale to prevent clips */
    int  replaygain_type;   /* 0=track gain, 1=album gain, 2=track gain if
                               shuffle is on, album gain otherwise */
    int  replaygain_preamp; /* scale replaygained tracks by this */
    int  beep;              /* system beep volume when changing tracks etc. */
    bool crossfeed;         /* enable crossfeed */
#endif
#ifdef HAVE_DIRCACHE
    bool dircache;          /* enable directory cache */
    int dircache_size;      /* directory cache structure last size, 22 bits */
#endif
    int default_codepage;   /* set default codepage for tag conversion */
#ifdef HAVE_REMOTE_LCD
    unsigned char rwps_file[MAX_FILENAME+1];  /* last remote-wps */
#endif
#ifdef HAVE_BACKLIGHT_BRIGHTNESS 
    int brightness;         /* iriver h300: backlight PWM value: 2..15
                                (0 and 1 are black) */
#endif
#ifdef HAVE_LCD_COLOR
    unsigned char backdrop_file[MAX_FILENAME+1];  /* backdrop bitmap file */
#endif

    bool warnon_erase_dynplaylist; /* warn when erasing dynamic playlist */
};

enum optiontype { INT, BOOL };

struct opt_items {
    unsigned const char* string;
    long voice_id;
};

/* prototypes */

void settings_calc_config_sector(void);
int settings_save(void);
void settings_load(int which);
void settings_reset(void);
void sound_settings_apply(void);
void settings_apply(void);
void settings_apply_pm_range(void);
void settings_display(void);

bool settings_load_config(const char* file);
bool settings_save_config(void);
bool set_bool_options(const char* string, bool* variable,
                      const char* yes_str, int yes_voice,
                      const char* no_str, int no_voice,
                      void (*function)(bool));

bool set_bool(const char* string, bool* variable );
bool set_option(const char* string, void* variable, enum optiontype type,
                const struct opt_items* options, int numoptions, void (*function)(int));
bool set_int(const unsigned char* string, const char* unit, int voice_unit,
             int* variable,
             void (*function)(int), int step, int min, int max, 
             void (*formatter)(char*, int, int, const char*) );
bool set_time_screen(const char* string, struct tm *tm);
int read_line(int fd, char* buffer, int buffer_size);
void set_file(char* filename, char* setting, int maxlen);

unsigned int rec_timesplit_seconds(void);
void settings_apply_trigger(void);

/* global settings */
extern struct user_settings global_settings;
/* name of directory where configuration, fonts and other data
 * files are stored */
extern long lasttime;

/* Recording base directory */
extern const char rec_base_directory[];

/* system defines */

#ifdef HAVE_LCD_CHARCELLS
#define MAX_CONTRAST_SETTING        31
#define DEFAULT_CONTRAST_SETTING    30
#else
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    38
#endif
#define MIN_CONTRAST_SETTING        5


/* argument bits for settings_load() */
#define SETTINGS_RTC 1 /* only the settings from the RTC nonvolatile RAM */
#define SETTINGS_HD  2 /* only the settings fron the disk sector */
#define SETTINGS_ALL 3 /* both */

/* repeat mode options */
enum
{
    REPEAT_OFF,
    REPEAT_ALL,
    REPEAT_ONE,
    REPEAT_SHUFFLE,
#if (AB_REPEAT_ENABLE == 1)
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
       SHOW_WPS, SHOW_RWPS, SHOW_CFG, SHOW_LNG, SHOW_MOD, SHOW_FONT, SHOW_PLUGINS};

/* recursive dir insert options */
enum { RECURSE_OFF, RECURSE_ON, RECURSE_ASK };

/* replaygain types */
enum { REPLAYGAIN_TRACK = 0, REPLAYGAIN_ALBUM, REPLAYGAIN_SHUFFLE };

#endif /* __SETTINGS_H__ */

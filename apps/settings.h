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
#include "file.h"

#define ROCKBOX_DIR "/.rockbox"

#define MAX_FILENAME 20

/* data structures */

#define RESUME_OFF 0
#define RESUME_ASK 1
#define RESUME_ASK_ONCE 2
#define RESUME_ON  3

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

struct user_settings
{
    /* audio settings */

    int volume;     /* audio output volume:  0-100 0=off   100=max           */
    int balance;    /* stereo balance:       0-100 0=left  50=bal 100=right  */
    int bass;       /* bass eq:              0-100 0=off   100=max           */
    int treble;     /* treble eq:            0-100 0=low   100=high          */
    int loudness;   /* loudness eq:          0-100 0=off   100=max           */
    int bass_boost; /* bass boost eq:        0-100 0=off   100=max           */
    int avc;        /* auto volume correct:  0=disable, 1=2s 2=4s 3=8s       */
    int channel_config;  /* Stereo, Mono, Mono left, Mono right */

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
    bool rec_editable; /* true means that the bit reservoir is off */
    
    /* device settings */

    int contrast;   /* lcd contrast:          0-63 0=low 63=high            */
    bool invert;    /* invert display */
    bool invert_cursor; /* invert the current file in dir browser and menu
                           instead of using the default cursor */
    int poweroff;   /* power off timer */
    int backlight_timeout;  /* backlight off timeout:  0-18 0=never,
                               1=always,
                               then according to timeout_values[] */
    bool backlight_on_when_charging;
    bool discharge; /* maintain charge of at least: false = 85%, true = 10%  */
    bool trickle_charge; /* do trickle charging: 0=off, 1=on */
    int battery_capacity; /* in mAh */

    /* resume settings */

    int resume;        /* resume option: 0=off, 1=ask, 2=on */
    int resume_index;  /* index in playlist (-1 for no active resume) */
    int resume_offset; /* byte offset in mp3 file */
    int resume_seed;   /* random seed for playlist shuffle */
    int resume_first_index; /* first index of playlist */

    bool save_queue_resume; /* save queued songs for resume */
    int queue_resume; /* resume queue file?: 0 = no
                                             1 = resume at queue index
                                             2 = resume at playlist index */
    int queue_resume_index; /* queue index (seek point in queue file) */

    unsigned char resume_file[MAX_PATH+1]; /* playlist name (or dir) */
    unsigned char font_file[MAX_FILENAME+1]; /* last font */
    unsigned char wps_file[MAX_FILENAME+1];  /* last wps */
    unsigned char lang_file[MAX_FILENAME+1]; /* last language */

    /* misc options */

    int repeat_mode;   /* 0=off 1=repeat all 2=repeat one  */
    int dirfilter;     /* 0=display all, 1=only supported, 2=only music */
    bool sort_case;    /* dir sort order: 0=case insensitive, 1=sensitive */
    int volume_type;   /* how volume is displayed: 0=graphic, 1=percent */
    int battery_type;  /* how battery is displayed: 0=graphic, 1=percent */
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
    bool peak_meter_performance;  /* true: high performance, else save energy*/
    int peak_meter_min; /* range minimum */
    int peak_meter_max; /* range maximum */

    /* show status bar */
    bool statusbar;    /* 0=hide, 1=show */

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

    bool fade_on_stop; /* fade on pause/unpause/stop */
    bool caption_backlight; /* turn on backlight at end and start of track */

#ifdef HAVE_FMRADIO
    int fm_freq_step;    /* Frequency step for manual tuning, in kHz */
    bool fm_force_mono;  /* Forces Mono mode if true */
    bool fm_full_range;  /* Enables full 10MHz-160MHz range if true, else
                            only 88MHz-108MHz */
#endif

    int max_files_in_dir; /* Max entries in directory (file browser) */
    int max_files_in_playlist; /* Max entries in playlist */
};

/* prototypes */

int settings_save(void);
void settings_load(void);
void settings_reset(void);
void settings_apply(void);
void settings_apply_pm_range(void);
void settings_display(void);

bool settings_load_config(char* file);
bool settings_save_config(void);
bool set_bool_options(char* string, bool* variable, 
                      char* yes_str, char* no_str );

bool set_bool(char* string, bool* variable );
bool set_option(char* string, int* variable, char* options[],
                int numoptions, void (*function)(int));
bool set_int(char* string, char* unit, int* variable,
             void (*function)(int), int step, int min, int max );
bool set_time(char* string, int timedate[]);
void set_file(char* filename, char* setting, int maxlen);

/* global settings */
extern struct user_settings global_settings;
/* name of directory where configuration, fonts and other data
 * files are stored */
extern char rockboxdir[];

/* system defines */

#ifdef HAVE_LCD_CHARCELLS
#define MAX_CONTRAST_SETTING        31
#define DEFAULT_CONTRAST_SETTING    30
#else
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    38
#endif
#define MIN_CONTRAST_SETTING        5
#define DEFAULT_INVERT_SETTING    false
#define DEFAULT_INVERT_CURSOR_SETTING false
#define DEFAULT_POWEROFF_SETTING    0
#define DEFAULT_BACKLIGHT_TIMEOUT_SETTING   5
#define DEFAULT_BACKLIGHT_ON_WHEN_CHARGING_SETTING   0
#define DEFAULT_FF_REWIND_MIN_STEP  FF_REWIND_1000
#define DEFAULT_FF_REWIND_ACCEL_SETTING 3

/* repeat mode options */
enum { REPEAT_OFF, REPEAT_ALL, REPEAT_ONE, NUM_REPEAT_MODES };

/* dir filter options */
enum { SHOW_ALL, SHOW_SUPPORTED, SHOW_MUSIC, SHOW_PLAYLIST, NUM_FILTER_MODES };

#endif /* __SETTINGS_H__ */

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

/* data structures */

#define RESUME_OFF 0
#define RESUME_ASK 1
#define RESUME_ON  2

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
    
    /* device settings */

    int contrast;   /* lcd contrast:         0-100 0=low 100=high            */
    int poweroff;   /* power off timer:      0-100 0=never:each 1% = 60 secs */
    int backlight;  /* backlight off timer:  0-100 0=never:each 1% = 10 secs */
    bool discharge; /* maintain charge of at least: false = 90%, true = 10%  */

    /* resume settings */

    int resume;        /* resume option: 0=off, 1=ask, 2=on */
    int resume_index;  /* index in playlist (-1 for no active resume) */
    int resume_offset; /* byte offset in mp3 file */
    int resume_seed;   /* random seed for playlist shuffle */
    unsigned char resume_file[MAX_PATH+1]; /* playlist name (or dir) */

    /* misc options */

    int loop_playlist; /* do we return to top of playlist at end?  */
    bool mp3filter;    /* only display mp3/m3u files and dirs in directory? */
    bool sort_case;    /* dir sort order: 0=case insensitive, 1=sensitive */
    int scroll_speed;  /* long texts scrolling speed: 1-30 */
    bool playlist_shuffle;
    bool play_selected; /* Plays selected file even in shuffle mode */
    int ff_rewind_min_step; /* FF/Rewind minimum step size */
    int ff_rewind_accel; /* FF/Rewind acceleration (in seconds per doubling) */
    int disk_spindown; /* time until disk spindown, in seconds (0=off) */

    /* show status bar */
    bool statusbar;    /* 0=hide, 1=show */

    /* show scroll bar */
    bool scrollbar;    /* 0=hide, 1=show */

    /* Hidden and dotfile settings */
    bool show_hidden_files; /* 1=show dotfiles/hidden,
                               0=hide dotfiles/hidden */
    
    /* goto current song when exiting WPS */
    bool browse_current; /* 1=goto current song,
                            0=goto previous location */

    /* geeky persistent statistics */
    unsigned int total_uptime; /* total uptime since rockbox was first booted */
};

/* prototypes */

int settings_save(void);
void settings_load(void);
void settings_reset(void);
void settings_display(void);

bool settings_load_eq(char* file);
void set_bool_options(char* string, bool* variable, char* yes_str, char* no_str );

void set_bool(char* string, bool* variable );
void set_option(char* string, int* variable, char* options[],
                int numoptions, void (*function)(int));
void set_int(char* string, 
             char* unit,
             int* variable,
             void (*function)(int),
             int step,
             int min,
             int max );
void set_time(char* string, int timedate[]);

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
#define DEFAULT_POWEROFF_SETTING    0
#define DEFAULT_BACKLIGHT_SETTING   5
#define DEFAULT_FF_REWIND_MIN_STEP  FF_REWIND_1000
#define DEFAULT_FF_REWIND_ACCEL_SETTING 3

#endif /* __SETTINGS_H__ */

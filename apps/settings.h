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

/* data structures */

#define RESUME_NONE     0
#define RESUME_SONG     1 /* resume song at startup     */
#define RESUME_PLAYLIST 2 /* resume playlist at startup */

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
    bool muted;     /* Is the device muted?                                  */
    
    /* device settings */

    int contrast;   /* lcd contrast:         0-100 0=low 100=high            */
    int poweroff;   /* power off timer:      0-100 0=never:each 1% = 60 secs */
    int backlight;  /* backlight off timer:  0-100 0=never:each 1% = 10 secs */
    bool discharge; /* maintain charge of at least: false = 90%, true = 10%  */

    /* resume settings */

    int resume;     /* power-on song resume: 0=no. 1=yes song. 2=yes pl   */
    int track_time; /* number of seconds into the track to resume         */

    /* misc options */

    int loop_playlist; /* do we return to top of playlist at end?  */
    bool mp3filter;    /* only display mp3/m3u files and dirs in directory? */
    bool sort_case;    /* dir sort order: 0=case insensitive, 1=sensitive */
    int scroll_speed;  /* long texts scrolling speed: 1-20 */
    bool playlist_shuffle;

    /* while playing screen settings  */
    int wps_display;   /* 0=id3, 1=file, 2=parse */

    /* show status bar */
    bool statusbar;    /* 0=hide, 1=show */
    
    /* geeky persistent statistics */
    unsigned int total_uptime; /* total uptime since rockbox was first booted */
};

/* prototypes */

int settings_save(void);
void settings_load(void);
void settings_reset(void);
void settings_display(void);

void set_bool(char* string, bool* variable );
void set_option(char* string, int* variable, char* options[], int numoptions );
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

/* system defines */

#define DEFAULT_VOLUME_SETTING     70/2
#define DEFAULT_BALANCE_SETTING    50
#define DEFAULT_BASS_SETTING       50/2
#define DEFAULT_TREBLE_SETTING     50/2
#define DEFAULT_LOUDNESS_SETTING    0
#define DEFAULT_BASS_BOOST_SETTING  0
#define DEFAULT_AVC_SETTING         0
#ifdef HAVE_LCD_CHARCELLS
#define MAX_CONTRAST_SETTING    31
#define DEFAULT_CONTRAST_SETTING    30
#else
#define MAX_CONTRAST_SETTING    63
#define DEFAULT_CONTRAST_SETTING    32
#endif
#define DEFAULT_POWEROFF_SETTING    0
#define DEFAULT_BACKLIGHT_SETTING   5
#define DEFAULT_WPS_DISPLAY         0 

#endif /* __SETTINGS_H__ */

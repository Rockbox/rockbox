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

/* data structures */

typedef enum 
{
    RESUME_NONE,       /* do not resume              */
    RESUME_SONG,       /* resume song at startup     */
    RESUME_PLAYLIST    /* resume playlist at startup */
} resume_t;

typedef struct
{
    /* audio settings */

    int volume;     /* audio output volume:  0-100 0=off   100=max           */
    int balance;    /* stereo balance:       0-100 0=left  50=bal 100=right  */
    int bass;       /* bass eq:              0-100 0=off   100=max           */
    int treble;     /* treble eq:            0-100 0=low   100=high          */
    int loudness;   /* loudness eq:          0-100 0=off   100=max           */
    int bass_boost; /* bass boost eq:        0-100 0=off   100=max           */

    /* device settings */

    int contrast;   /* lcd contrast:         0-100 0=low 100=high            */
    int poweroff;   /* power off timer:      0-100 0=never:each 1% = 60 secs */ 
    int backlight;  /* backlight off timer:  0-100 0=never:each 1% = 10 secs */

    /* resume settings */

    resume_t resume;   /* power-on song resume: 0=no. 1=yes song. 2=yes pl   */
    int track_time;    /* number of seconds into the track to resume         */

    /* misc options */

    int loop_playlist; /* do we return to top of playlist at end?            */

} user_settings_t;

/* prototypes */

int persist_all_settings( void );
void reload_all_settings( user_settings_t *settings );
void reset_settings( user_settings_t *settings );
void display_current_settings( user_settings_t *settings );

/* system defines */

#define DEFAULT_VOLUME_SETTING     70
#define DEFAULT_BALANCE_SETTING    50
#define DEFAULT_BASS_SETTING       50
#define DEFAULT_TREBLE_SETTING     50
#define DEFAULT_LOUDNESS_SETTING    0
#define DEFAULT_BASS_BOOST_SETTING  0
#define DEFAULT_CONTRAST_SETTING    0
#define DEFAULT_POWEROFF_SETTING    0
#define DEFAULT_BACKLIGHT_SETTING   1


#endif /* __SETTINGS_H__ */




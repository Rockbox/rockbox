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

#include <stdio.h>
#include "config.h"
#include "settings.h"
#include "disk.h"
#include "panic.h"
#include "debug.h"

/*
 * persist all runtime user settings to disk
 */
int persist_all_settings( void )
{
    return 1;
}

/*
 * persist all the playlist information to disk
 */
int persist_all_playlist_info( void )
{
    return 1;
}

/*
 * load settings from disk
 */
void reload_all_settings( user_settings_t *settings )
{
    DEBUGF( "reload_all_settings()\n" );

    /* this is a TEMP stub version */
    
    /* populate settings with default values */
    
    reset_settings( settings );
}

/*
 * reset all settings to their default value 
 */
void reset_settings( user_settings_t *settings ) {
        
    DEBUGF( "reset_settings()\n" );

    settings->volume     = DEFAULT_VOLUME_SETTING;
    settings->balance    = DEFAULT_BALANCE_SETTING;
    settings->bass       = DEFAULT_BASS_SETTING;
    settings->treble     = DEFAULT_TREBLE_SETTING;
    settings->loudness   = DEFAULT_LOUDNESS_SETTING;
    settings->bass_boost = DEFAULT_BASS_BOOST_SETTING;
    settings->contrast   = DEFAULT_CONTRAST_SETTING;
    settings->poweroff   = DEFAULT_POWEROFF_SETTING;
    settings->backlight  = DEFAULT_BACKLIGHT_SETTING;
}

/*
 * dump the list of current settings
 */
void display_current_settings( user_settings_t *settings )
{
#ifdef DEBUG
    DEBUGF( "\ndisplay_current_settings()\n" );

    DEBUGF( "\nvolume:\t\t%d\nbalance:\t%d\nbass:\t\t%d\ntreble:\t\t%d\nloudness:\t%d\nbass boost:\t%d\n",
            settings->volume,
            settings->balance,
            settings->bass,
            settings->treble,
            settings->loudness,
            settings->bass_boost );

    DEBUGF( "contrast:\t%d\npoweroff:\t%d\nbacklight:\t%d\n",
            settings->contrast,
            settings->poweroff,
            settings->backlight );
#else
    /* Get rid of warning */
    settings = settings;
#endif
}






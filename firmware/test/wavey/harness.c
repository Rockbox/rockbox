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
#include <common/track.h>
#include "settings.h"
#include "playlist.h"
#include "panic.h"
#include "disk.h"
#include "debug.h"
#include "config.h"
#include "harness.h"

/* global string for panic display */

char panic_message[128];

/*
 * entrypoint
 */
int main( int argc, char **args )
{
    /* allocate runtime data structures */
    
    user_settings_t settings;
    playlist_info_t playlist;

    debug( "\nrockbox test harness started.\n" );

    /* populate runtime data structures */
    
    initialise( &settings, &playlist );

    /* begin tests */
    
    start( &settings, &playlist );

    return 1;
}

/*
 * populate runtime data structures 
 */
void initialise( user_settings_t *settings, playlist_info_t *playlist )
{
    debug( "init()\n" );

    reload_all_settings( settings );
    reload_playlist_info( playlist );
}

/*
 * start tests 
 */
void start( user_settings_t *settings, playlist_info_t *playlist )
{
    track_t track;

    debug( "start()\n" );

    /* show current values */
    
    display_current_settings( settings );
    display_current_playlist( playlist );

    /* wipe playlist contents */
    
    empty_playlist( playlist );
    display_current_playlist( playlist );
    
    /* user selects a new playlist */
    
    load_playlist( playlist, "\\playlists\\2.m3u" );
    display_current_playlist( playlist );

    /* randomise playlist */
    
    randomise_playlist( playlist );
    display_current_playlist( playlist );

    /* get next track in playlist */

    track = next_playlist_track( playlist );
    display_playlist_track( &track );

    /* get next track in playlist */

    track = next_playlist_track( playlist );
    display_playlist_track( &track );
}

#ifdef SIMULATOR
void app_main ()
{
    main (0, NULL);
}
#endif // SIMULATOR

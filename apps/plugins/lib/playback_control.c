/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

struct plugin_api* api = 0;

bool prevtrack(void)
{
    api->audio_prev();
    return false;
}

bool play(void)
{
    int audio_status = api->audio_status();
    if (!audio_status && api->global_status->resume_index != -1)
    {
        if (api->playlist_resume() != -1)
        {
            api->playlist_start(api->global_status->resume_index,
                api->global_status->resume_offset);
        }
    }
    else if (audio_status & AUDIO_STATUS_PAUSE)
        api->audio_resume();
    else
        api->audio_pause();
    return false;
}

bool stop(void)
{
    api->audio_stop();
    return false;
}

bool nexttrack(void)
{
    api->audio_next();
    return false;
}

static bool volume(void)
{
    return api->set_sound("Volume", &api->global_settings->volume,
                          SOUND_VOLUME);
}

static bool shuffle(void)
{
    struct opt_items names[] = {
        { "No", -1 },
        { "Yes", -1 }
    };
    return api->set_option("Shuffle", &api->global_settings->playlist_shuffle,
                           BOOL, names, 2,NULL);
}

static bool repeat_mode(void)
{
    bool result;
    static const struct opt_items names[] = {
        { "Off", -1 },
        { "Repeat All", -1 },
        { "Repeat One", -1 },
        { "Repeat Shuffle", -1 },
#ifdef AB_REPEAT_ENABLE
        { "Repeat A-B", -1 }
#endif
    };
    
    int old_repeat = api->global_settings->repeat_mode;

    result = api->set_option( "Repeat Mode",
                              &api->global_settings->repeat_mode,
                              INT, names, NUM_REPEAT_MODES, NULL );

    if (old_repeat != api->global_settings->repeat_mode &&
        (api->audio_status() & AUDIO_STATUS_PLAY))
        api->audio_flush_and_reload_tracks();

    return result;
}

bool playback_control(struct plugin_api* newapi)
{
    int m;
    api = newapi;
    bool result;
    
    static struct menu_item items[] = {
        { "Previous Track", prevtrack },
        { "Pause / Play", play },
        { "Stop Playback", stop },
        { "Next Track", nexttrack },
        { "Change Volume", volume },
        { "Enable/Disable Shuffle", shuffle },
        { "Change Repeat Mode", repeat_mode },
    };

    m=api->menu_init( items, sizeof(items) / sizeof(*items), NULL,
                 NULL, NULL, NULL);
    result = api->menu_run(m);
    api->menu_exit(m);
    return result;
}

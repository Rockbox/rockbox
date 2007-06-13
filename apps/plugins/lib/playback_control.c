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
    struct opt_items names[2];
        names[0].string = "No";
        names[0].voice_id = -1;
        names[1].string = "Yes";
        names[1].voice_id = -1;

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
MENUITEM_FUNCTION(prevtrack_item, 0, "Previous Track",
                  prevtrack, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(playpause_item, 0, "Pause / Play",
                  play, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(stop_item, 0, "Stop Playback",
                  stop, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(nexttrack_item, 0, "Next Track",
                  nexttrack, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(volume_item, 0, "Change Volume",
                  volume, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(shuffle_item, 0, "Enable/Disable Shuffle",
                  shuffle, NULL, NULL, Icon_NOICON);
MENUITEM_FUNCTION(repeat_mode_item, 0, "Change Repeat Mode",
                  repeat_mode, NULL, NULL, Icon_NOICON);
MAKE_MENU(playback_control_menu, "Playback Control", NULL, Icon_NOICON,
            &prevtrack_item, &playpause_item, &stop_item, &nexttrack_item,
            &volume_item, &shuffle_item, &repeat_mode_item);

void playback_control_init(struct plugin_api* newapi)
{
    api = newapi;
}

bool playback_control(struct plugin_api* newapi)
{
    api = newapi;
    return api->do_menu(&playback_control_menu, NULL) == MENU_ATTACHED_USB;
}

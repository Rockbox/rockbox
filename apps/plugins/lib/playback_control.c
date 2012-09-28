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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"
#include "playback_control.h"

struct viewport *parentvp = NULL;

static bool prevtrack(void)
{
    audio_prev();
    return false;
}

static bool play(void)
{
    int status = audio_status();
    if (!status && global_status.resume_index != -1)
    {
        if (playlist_resume() != -1)
        {
            playlist_start(global_status.resume_index,
                global_status.resume_offset);
        }
    }
    else if (status & AUDIO_STATUS_PAUSE)
        audio_resume();
    else
        audio_pause();
    return false;
}

static bool stop(void)
{
    audio_stop();
    return false;
}

static bool nexttrack(void)
{
    audio_next();
    return false;
}

static bool volume(void)
{
    const struct settings_list* vol = 
        find_setting(&global_settings.volume, NULL);
    return option_screen((struct settings_list*)vol, parentvp, false, "Volume");
}

static bool shuffle(void)
{
    const struct settings_list* shuffle = 
        find_setting(&global_settings.playlist_shuffle, NULL);
    return option_screen((struct settings_list*)shuffle, parentvp, false, "Shuffle");
}

static bool repeat_mode(void)
{
    const struct settings_list* repeat = 
        find_setting(&global_settings.repeat_mode, NULL);
    int old_repeat = global_settings.repeat_mode;
  
    option_screen((struct settings_list*)repeat, parentvp, false, "Repeat");
  
    if (old_repeat != global_settings.repeat_mode &&
        (audio_status() & AUDIO_STATUS_PLAY))
        audio_flush_and_reload_tracks();
  
    return false;
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

void playback_control_init(struct viewport parent[NB_SCREENS])
{
    parentvp = parent;
}

bool playback_control(struct viewport parent[NB_SCREENS])
{
    parentvp = parent;
    return do_menu(&playback_control_menu, NULL, parent, false) == MENU_ATTACHED_USB;
}

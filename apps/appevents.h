/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: events.h 17847 2008-06-28 18:10:04Z bagder $
 *
 * Copyright (C) 2008 by Jonathan Gordon
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

#ifndef _APPEVENTS_H
#define _APPEVENTS_H

#include <stdbool.h>
#include "events.h"

/** Only app/ level events should be defined here.
 *  firmware/ level events and CLASS's are defined in firmware/export/events.h
 */

/** Playback events **/
enum {
    PLAYBACK_EVENT_TRACK_BUFFER = (EVENT_CLASS_PLAYBACK|1),
    PLAYBACK_EVENT_TRACK_FINISH,
    PLAYBACK_EVENT_TRACK_CHANGE,
};

/** Buffering events **/
enum {
    BUFFER_EVENT_BUFFER_LOW = (EVENT_CLASS_BUFFERING|1),
    BUFFER_EVENT_REBUFFER,
    BUFFER_EVENT_CLOSED,
    BUFFER_EVENT_MOVED,
    BUFFER_EVENT_FINISHED,
};

/** Generic GUI class events **/
enum {
    GUI_EVENT_THEME_CHANGED = (EVENT_CLASS_GUI|1),
    GUI_EVENT_STATUSBAR_TOGGLE,
};

#endif



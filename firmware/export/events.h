/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Miika Pekkarinen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _EVENTS_H
#define _EVENTS_H

#include <stdbool.h>

/**
 * High byte = Event class definition
 * Low byte  = Event ID
 */

#define EVENT_CLASS_DISK       0x0100
#define EVENT_CLASS_PLAYBACK   0x0200
#define EVENT_CLASS_BUFFERING  0x0400

/**
 * Because same playback events are used in mpeg.c and playback.c, define
 * them here to prevent cluttering and ifdefs.
 */
enum {
    PLAYBACK_EVENT_TRACK_BUFFER = (EVENT_CLASS_PLAYBACK|1),
    PLAYBACK_EVENT_TRACK_FINISH,
    PLAYBACK_EVENT_TRACK_CHANGE,
};


bool add_event(unsigned short id, bool oneshot, void (*handler));
void remove_event(unsigned short id, void (*handler));
void send_event(unsigned short id, void *data);

#endif


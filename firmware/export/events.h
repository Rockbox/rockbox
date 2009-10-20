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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _EVENTS_H
#define _EVENTS_H

#include <stdbool.h>

/** Only CLASS defines and firmware/ level events should be defined here.
 *  apps/ level events are defined in apps/appevents.h 
 */

/**
 * High byte = Event class definition
 * Low byte  = Event ID
 */

#define EVENT_CLASS_DISK       0x0100
#define EVENT_CLASS_PLAYBACK   0x0200
#define EVENT_CLASS_BUFFERING  0x0400
#define EVENT_CLASS_GUI        0x0800
#define EVENT_CLASS_LCD        0xf000

bool add_event(unsigned short id, bool oneshot, void (*handler)(void *data));
void remove_event(unsigned short id, void (*handler)(void *data));
void send_event(unsigned short id, void *data);

#endif


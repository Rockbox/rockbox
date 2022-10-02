/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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
#ifndef _WPS_H_
#define _WPS_H_

#include <stdbool.h>

long gui_wps_show(void);

/* fade (if enabled) and pause the audio, optionally rewind a little */
void pause_action(bool may_fade, bool updatewps);
void unpause_action(bool may_fade, bool updatewps);
void wps_do_playpause(bool updatewps);

#ifdef IPOD_ACCESSORY_PROTOCOL
/* whether the wps is fading the volume due to pausing/stopping */
bool is_wps_fading(void);
#endif /* IPOD_ACCESSORY_PROTOCOL */

/* in milliseconds */
#define DEFAULT_SKIP_TRESH          3000l

#endif /* _WPS_H_ */

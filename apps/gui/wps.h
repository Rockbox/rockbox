/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: gwps-common.h 20492 2009-03-23 17:19:48Z alle $
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
#include "screen_access.h"
 
long gui_wps_show(void);
 
/* wrapper for the wps to load the skin (.wps/.rwps) files */
void wps_data_load(enum screen_type, const char *, bool);
void wps_data_init(enum screen_type);

void gui_sync_wps_init(void);

/* fades the volume, e.g. on pause or stop */
void fade(bool fade_in, bool updatewps);

bool ffwd_rew(int button);
void display_keylock_text(bool locked);

bool is_wps_fading(void);


#ifdef HAVE_ALBUMART
/*
 * Returns true if at least one of the gui_wps screens has an album art
 * tag in its wps structure and writes the width and height into the passed
 * pointers
 */
bool wps_uses_albumart(int*, int*);
#endif

#ifdef IPOD_ACCESSORY_PROTOCOL
/* return length of the current ff or rewin action, IAP needs this */
int wps_get_ff_rewind_count(void);
#endif /* IPOD_ACCESSORY_PROTOCOL */

#endif /* _WPS_H_ */

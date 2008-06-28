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
#ifndef __PLAYBACK_CONTROL_H__
#define __PLAYBACK_CONTROL_H__

/* Use these if your menu uses the new menu api. 
   REMEMBER to call playback_control_init(rb) before rb->do_menu()... 
   The parent viewport here is needed by the internal functions,
   So, make sure you use the same viewport for the rb->do_menu() call
   that you use in the playback_control_init() call
*/
void playback_control_init(const struct plugin_api* newapi,
                           struct viewport parent[NB_SCREENS]);

/* Use this if your menu still uses the old menu api */
bool playback_control(const struct plugin_api* api,
                      struct viewport parent[NB_SCREENS]);

#endif /* __PLAYBACK_CONTROL_H__ */

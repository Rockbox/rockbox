/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _GWPS_COMMON_
#define _GWPS_COMMON_
#include <stdbool.h>
#include <sys/types.h> /* for size_t */

#include "gwps.h"

void fade(bool fade_in);
bool gui_wps_display(void);
bool update_onvol_change(struct gui_wps * gwps);
bool update(struct gui_wps *gwps);
void play_hop(int direction);
bool ffwd_rew(int button);
void display_keylock_text(bool locked);

bool gui_wps_refresh(struct gui_wps *gwps,
                     int ffwd_offset,
                     unsigned char refresh_mode);
#endif

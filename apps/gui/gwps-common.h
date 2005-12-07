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

void gui_wps_format_time(char* buf, int buf_size, long time);
void fade(bool fade_in);
void gui_wps_format(struct wps_data *data);
bool gui_wps_refresh(struct gui_wps *gwps, int ffwd_offset,
                     unsigned char refresh_mode);
bool gui_wps_display(void);
void setvol(void);
bool update_onvol_change(struct gui_wps * gwps);
bool update(struct gui_wps *gwps);
bool ffwd_rew(int button);
bool wps_data_preload_tags(struct wps_data *data, unsigned char *buf,
                            const char *bmpdir, size_t bmpdirlen);
#ifdef WPS_KEYLOCK
void display_keylock_text(bool locked);
void waitfor_nokey(void);
#endif
#endif


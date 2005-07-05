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
#ifndef WPS_DISPLAY
#define WPS_DISPLAY

#include <stdbool.h>
#include "id3.h"

/* constants used in line_type and as refresh_mode for wps_refresh */
#define WPS_REFRESH_STATIC          1    /* line doesn't change over time */
#define WPS_REFRESH_DYNAMIC         2    /* line may change (e.g. time flag) */
#define WPS_REFRESH_SCROLL          4    /* line scrolls */
#define WPS_REFRESH_PLAYER_PROGRESS 8    /* line contains a progress bar */
#define WPS_REFRESH_PEAK_METER      16   /* line contains a peak meter */
#define WPS_REFRESH_ALL             0xff /* to refresh all line types */
/* to refresh only those lines that change over time */
#define WPS_REFRESH_NON_STATIC (WPS_REFRESH_ALL & ~WPS_REFRESH_STATIC & ~WPS_REFRESH_SCROLL)

/* alignments */
#define WPS_ALIGN_RIGHT 32
#define WPS_ALIGN_CENTER 64
#define WPS_ALIGN_LEFT 128


void wps_format_time(char* buf, int buf_size, long time);
bool wps_refresh(struct mp3entry* id3, struct mp3entry* nid3,
                 int ffwd_offset, unsigned char refresh_mode);
bool wps_display(struct mp3entry* id3, struct mp3entry* nid3);
bool wps_load(const char* file, bool display);
void wps_reset(void);

#endif

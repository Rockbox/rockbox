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

bool wps_refresh(struct mp3entry* id3, int ffwd_offset, bool refresh_scroll);
void wps_display(struct mp3entry* id3);
bool wps_load(char* file, bool display);

#ifdef HAVE_LCD_CHARCELLS
bool draw_player_progress(struct mp3entry* id3, int ff_rewind_count);
#endif

#endif

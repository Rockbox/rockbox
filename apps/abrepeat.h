/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $$
 *
 * Copyright (C) 2005 Ray Lambert
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ABREPEAT_H_
#define _ABREPEAT_H_

#include "system.h"
#include <stdbool.h>

#ifdef AB_REPEAT_ENABLE

#define AB_MARKER_NONE 0

void ab_repeat_init(void);
bool ab_repeat_mode_enabled(void);  // test if a/b repeat is enabled
bool ab_A_marker_set(void);
bool ab_B_marker_set(void);
unsigned int ab_get_A_marker(void);
unsigned int ab_get_B_marker(void);
bool ab_reached_B_marker(unsigned int song_position);
bool ab_before_A_marker(unsigned int song_position);
bool ab_after_A_marker(unsigned int song_position);
void ab_jump_to_A_marker(void);
void ab_reset_markers(void);
void ab_set_A_marker(unsigned int song_position);
void ab_set_B_marker(unsigned int song_position);
#ifdef HAVE_LCD_BITMAP
void ab_draw_markers(int capacity, int x, int y, int w, int h);
#endif

#endif

#endif /* _ABREPEAT_H_ */

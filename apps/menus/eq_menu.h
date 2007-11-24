/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: eq_menu.h 10888 2006-09-05 11:48:17Z dan $
 *
 * Copyright (C) 2006 Dan Everton
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _EQ_MENU_H
#define _EQ_MENU_H

#include "menu.h"
#include "config.h"
/* Various user interface limits and sizes */
#define EQ_CUTOFF_MIN        20
#define EQ_CUTOFF_MAX     22040
#define EQ_CUTOFF_STEP       10
#define EQ_CUTOFF_FAST_STEP 100
#define EQ_GAIN_MIN       (-240)
#define EQ_GAIN_MAX         240
#define EQ_GAIN_STEP          5
#define EQ_GAIN_FAST_STEP    10
#define EQ_Q_MIN              5
#define EQ_Q_MAX             64
#define EQ_Q_STEP             1
#define EQ_Q_FAST_STEP       10

#define EQ_USER_DIVISOR      10

bool eq_browse_presets(void);
bool eq_menu_graphical(void);

/* utility functions for settings_list.c */
void eq_gain_format(char* buffer, size_t buffer_size, int value, const char* unit);
void eq_q_format(char* buffer, size_t buffer_size, int value, const char* unit);
void eq_precut_format(char* buffer, size_t buffer_size, int value, const char* unit);

#endif

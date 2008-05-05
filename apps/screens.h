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
#ifndef _SCREENS_H_
#define _SCREENS_H_

#include "config.h"
#include "timefuncs.h"

struct screen;

void usb_screen(void);
#if CONFIG_CHARGING && !defined(HAVE_POWEROFF_WHILE_CHARGING) && defined(CPU_SH)
int charging_screen(void);
#endif
#if CONFIG_CHARGING || defined(SIMULATOR)
void charging_splash(void);
#endif

#ifdef HAVE_MMC
int mmc_remove_request(void);
#endif

#ifdef HAVE_PITCHSCREEN
bool pitch_screen(void);
#endif

#if CONFIG_RTC
bool set_time_screen(const char* title, struct tm *tm);
#endif

bool shutdown_screen(void);
bool browse_id3(void);
bool view_runtime(void);

#endif


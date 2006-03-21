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

#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == IRIVER_H100_PAD) ||\
    (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PITCH_UP BUTTON_UP
#define PITCH_DOWN BUTTON_DOWN
#define PITCH_RIGHT BUTTON_RIGHT
#define PITCH_LEFT BUTTON_LEFT
#define PITCH_EXIT BUTTON_OFF
#define PITCH_RESET BUTTON_ON
#elif (CONFIG_KEYPAD == ONDIO_PAD)
#define PITCH_UP BUTTON_UP
#define PITCH_DOWN BUTTON_DOWN
#define PITCH_RIGHT BUTTON_RIGHT
#define PITCH_LEFT BUTTON_LEFT
#define PITCH_EXIT BUTTON_OFF
#define PITCH_RESET BUTTON_MENU
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD)
#define PITCH_UP BUTTON_SCROLL_FWD
#define PITCH_DOWN BUTTON_SCROLL_BACK
#define PITCH_RIGHT BUTTON_RIGHT
#define PITCH_LEFT BUTTON_LEFT
#define PITCH_EXIT BUTTON_SELECT
#define PITCH_RESET BUTTON_MENU
#endif

struct screen;

void usb_display_info(struct screen * display);
void usb_screen(void);
int charging_screen(void);
void charging_splash(void);

#ifdef HAVE_MMC
int mmc_remove_request(void);
#endif

bool pitch_screen(void);

#if CONFIG_KEYPAD == RECORDER_PAD
extern bool quick_screen_f3(int button_enter);
#endif
extern bool quick_screen_quick(int button_enter);

#ifdef CONFIG_RTC
bool set_time_screen(const char* string, struct tm *tm);
#endif

bool shutdown_screen(void);
bool browse_id3(void);
bool set_rating(void);

#endif


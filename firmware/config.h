/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(ARCHOS_PLAYER)
#include "config-player.h"
#elif defined(ARCHOS_PLAYER_OLD)
#include "config-playerold.h"
#elif defined(ARCHOS_RECORDER)
#include "config-recorder.h"
#else
/* no known platform */
#endif

/* system defines */

#define DEFAULT_VOLUME_SETTING     50
#define DEFAULT_BALANCE_SETTING    50
#define DEFAULT_BASS_SETTING       50
#define DEFAULT_TREBLE_SETTING     50
#define DEFAULT_LOUDNESS_SETTING    0
#define DEFAULT_BASS_BOOST_SETTING  0
#define DEFAULT_CONTRAST_SETTING    0
#define DEFAULT_POWEROFF_SETTING    0
#define DEFAULT_BACKLIGHT_SETTING   1

#define CRT_DISPLAY 0 /* see debug.c debug() -> printf() */

#endif

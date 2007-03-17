/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _GUI_SPLASH_H_
#define _GUI_SPLASH_H_
#include <_ansi.h>
#include "screen_access.h"

/*
 * Puts a splash message centered on the given screen for a given period
 *  - screen : the screen to put the splash on
 *  - ticks : how long the splash is displayed (in rb ticks)
 *  - fmt : what to say *printf style
 */
extern void gui_splash(struct screen * screen, int ticks,
                       const char *fmt, ...) ATTRIBUTE_PRINTF(3, 4);

/*
 * Puts a splash message centered on all the screens for a given period
 *  - ticks : how long the splash is displayed (in rb ticks)
 *  - fmt : what to say *printf style
 */
extern void gui_syncsplash(int ticks, const char *fmt, ...)
                           ATTRIBUTE_PRINTF(2, 3);

#endif /* _GUI_ICON_H_ */

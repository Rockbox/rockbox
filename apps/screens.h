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

void usb_display_info(void);
void usb_screen(void);
int charging_screen(void);

#ifdef HAVE_RECORDER_KEYPAD
int on_screen(void);
bool f2_screen(void);
bool f3_screen(void);
#endif

void splash(int ticks,  /* how long */
            int keymask,/* what keymask aborts the waiting (if any) */
            bool center, /* FALSE means left-justified, TRUE means
                            horizontal and vertical center */
            char *fmt,  /* what to say *printf style */
            ...);

#endif


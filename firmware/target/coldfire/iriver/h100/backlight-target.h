/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef BACKLIGHT_TARGET_H
#define BACKLIGHT_TARGET_H

bool _backlight_init(void); /* Returns backlight current state (true=ON). */
void _backlight_hw_on(void);
void _backlight_hw_off(void);

#define _backlight_on_isr() _backlight_hw_on()
#define _backlight_off_isr() _backlight_hw_off()
#define _backlight_on_normal() _backlight_hw_on()
#define _backlight_off_normal() _backlight_hw_off()
#define _BACKLIGHT_FADE_BOOST

void _remote_backlight_on(void);
void _remote_backlight_off(void);

#endif

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Rockbox driver for 2bit vertically interleaved remote LCDs
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd-remote.h"

/* Compile 2 bit vertical interleaved LCD driver for remote LCD */
#define LCDFN(fn) lcd_remote_ ## fn
#define FBFN(fn)  fb_remote_ ## fn
#define LCDM(ma) LCD_REMOTE_ ## ma
#define LCDNAME "lcd_remote_"

#include "lcd-2bit-vi.c"

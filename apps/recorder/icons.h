/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <lcd.h>

/* 
 * Icons of size 6x8 pixels 
 */

#ifdef HAVE_LCD_BITMAP

enum icons_6x8 {
    Box_Filled, Box_Empty, Slider_Horizontal, File, 
    Folder,     Directory, Playlist,          Repeat,
    Selected,   Selector,  Cursor,            LastIcon
};

extern unsigned char bitmap_icons_6x8[LastIcon][6];

extern unsigned char rockbox112x37[];

#endif /* End HAVE_LCD_BITMAP */


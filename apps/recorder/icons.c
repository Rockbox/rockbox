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

#include "icons.h"

#ifdef HAVE_LCD_BITMAP

unsigned char bitmap_icons_6x8[LastIcon][6] =
{
    /* Box_Filled */
    { 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f }, 
    /* Box_Empty */
    { 0x00, 0x7f, 0x41, 0x41, 0x41, 0x7f }, 
    /* Slider_Horizontal */
    { 0x00, 0x3e, 0x7f, 0x63, 0x7f, 0x3e }, 
    /* File */
    { 0x60, 0x7f, 0x03, 0x63, 0x7f, 0x00 },
    /* Folder */
    { 0x00, 0x7e, 0x41, 0x41, 0x42, 0x7e },
    /* Directory */
    { 0x3e, 0x26, 0x26, 0x24, 0x3c, 0x00 },
    /* Playlist */
    { 0x55, 0x00, 0x55, 0x55, 0x55, 0x00 }, 
    /* Repeat */
    { 0x39, 0x43, 0x47, 0x71, 0x61, 0x4e }, 
    /* Selected */
	{ 0x00, 0x1c, 0x3e, 0x3e, 0x3e, 0x1c },
    /* Selector */
    { 0x00, 0x7f, 0x3e, 0x1c, 0x08, 0x00 }, 
};


#endif

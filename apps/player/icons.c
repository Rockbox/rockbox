/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Justin Heiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "icons.h"

#ifdef HAVE_LCD_CHARCELLS

char tree_icons_5x7[LastTreeIcon][8] =
{
    { 0x06, 0x09, 0x09, 0x02, 0x02, 0x00, 0x02, 0x00 }, /* Unknown  */
    { 0x07, 0x04, 0x07, 0x04, 0x1c, 0x1c, 0x08, 0x00 }, /* File     */
    { 0x0c, 0x13, 0x11, 0x11, 0x11, 0x11, 0x1f, 0x00 }, /* Folder   */
    { 0x17, 0x00, 0x17, 0x00, 0x17, 0x00, 0x17, 0x00 }, /* Playlist */
    { 0x01, 0x01, 0x02, 0x02, 0x14, 0x0c, 0x04, 0x00 }, /* WPS      */
    { 0x1f, 0x11, 0x1b, 0x15, 0x1b, 0x11, 0x1f, 0x00 }, /* MOD/AJZ  */
    { 0x00, 0x1f, 0x15, 0x1f, 0x15, 0x1f, 0x00, 0x00 }, /* Language */
    { 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00 }, /* Text file */
    { 0x0b, 0x10, 0x0b, 0x00, 0x1f, 0x00, 0x1f, 0x00 }, /* Config file */
};

#endif

/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Grayscale framework
* gray_set_drawinfo() function
*
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
*
* Copyright (C) 2004 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef SIMULATOR /* not for simulator by now */
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP /* and also not for the Player */
#include "gray.h"

/*---------------------------------------------------------------------------
 Set draw mode, foreground and background shades at once
 ----------------------------------------------------------------------------
 If you hand it -1 (or in fact any other out-of-bounds value) for a
 parameter, that particular setting won't be changed

 Default after initialization: GRAY_DRAW_SOLID, 0, 255
 */
void gray_set_drawinfo(int drawmode, int fg_brightness, int bg_brightness)
{
    gray_set_drawmode(drawmode);
    gray_set_foreground(fg_brightness);
    gray_set_background(bg_brightness);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


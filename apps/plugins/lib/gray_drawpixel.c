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
* gray_drawpixel() function
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
 Set a pixel with the current drawinfo
 ----------------------------------------------------------------------------
 If the drawmode is GRAY_DRAW_INVERSE, the pixel is inverted
 GRAY_DRAW_FG and GRAY_DRAW_SOLID draw the pixel in the foreground shade
 GRAY_DRAW_BG draws the pixel in the background shade
 */
void gray_drawpixel(int x, int y)
{
    unsigned long pattern;

    if ((unsigned) x >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;

    pattern = (_graybuf->drawmode == GRAY_DRAW_BG) ?
               _graybuf->bg_pattern : _graybuf->fg_pattern;

    _gray_pixelfuncs[_graybuf->drawmode](x, y, pattern);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


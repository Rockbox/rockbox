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
* gray_horline() function
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
 Draw a horizontal line from (x1, y) to (x2, y) with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 */
void gray_horline(int x1, int x2, int y)
{
    int x;
    unsigned long pattern;
    void (*pixelfunc)(int x, int y, unsigned long pattern);

    if ((unsigned) x1 >= (unsigned) _graybuf->width
        || (unsigned) x2 >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;

    if (x1 > x2)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    pixelfunc = _gray_pixelfuncs[_graybuf->drawmode];
    pattern = (_graybuf->drawmode == GRAY_DRAW_BG) ?
               _graybuf->bg_pattern : _graybuf->fg_pattern;
              
    for (x = x1; x <= x2; x++)
        pixelfunc(x, y, pattern);

}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


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
* gray_drawrect() function
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
 Draw a (hollow) rectangle with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details
 */
void gray_drawrect(int x, int y, int nx, int ny)
{
    int x2, y2;

    if ((unsigned) x >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;

    if ((unsigned) (y + ny) >= (unsigned) _graybuf->height) /* clip bottom */
        ny = _graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) _graybuf->width) /* clip right */
        nx = _graybuf->width - x;
        
    x2 = x + nx - 1;
    y2 = y + ny - 1;

    gray_horline(x, x2, y);
    gray_horline(x, x2, y2);
    gray_verline(x, y, y2);
    gray_verline(x2, y, y2);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


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
* gray_set_drawmode() function
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
 Set the draw mode for subsequent drawing operations
 ----------------------------------------------------------------------------
   drawmode =
     GRAY_DRAW_INVERSE: Foreground pixels are inverted, background pixels are
                        left untouched
     GRAY_DRAW_FG:      Only foreground pixels are drawn
     GRAY_DRAW_BG:      Only background pixels are drawn
     GRAY_DRAW_SOLID:   Foreground and background pixels are drawn

 Default after initialization: GRAY_DRAW_SOLID
 */
void gray_set_drawmode(int drawmode)
{
    if ((unsigned) drawmode <= GRAY_DRAW_SOLID)
        _graybuf->drawmode = drawmode;
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


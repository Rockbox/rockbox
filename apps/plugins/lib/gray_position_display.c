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
* gray_position_display() function
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
 Set position of the top left corner of the grayscale overlay
 ----------------------------------------------------------------------------
   x  = left margin in pixels
   by = top margin in 8-pixel units

 You may set this in a way that the overlay spills across the right or
 bottom display border. In this case it will simply be clipped by the
 LCD controller. You can even set negative values, this will clip at the
 left or top border. I did not test it, but the limits may be +127 / -128

 If you use this while the grayscale overlay is running, the now-freed area
 will be restored.
 */
void gray_position_display(int x, int by)
{
    _graybuf->x = x;
    _graybuf->by = by;
    
    if (_graybuf->flags & _GRAY_RUNNING)
        _graybuf->flags |= _GRAY_DEFERRED_UPDATE;
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


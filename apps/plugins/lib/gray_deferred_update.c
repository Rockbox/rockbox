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
* gray_deferred_update() function
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
 Do an lcd_update() to show changes done by rb->lcd_xxx() functions (in areas
 of the screen not covered by the grayscale overlay).
 ----------------------------------------------------------------------------
 If the grayscale overlay is running, the update will be done in the next
 call of the interrupt routine, otherwise it will be performed right away.
 See also comment for the gray_show_display() function.
 */
void gray_deferred_update(void)
{
    if (_graybuf->flags & _GRAY_RUNNING)
        _graybuf->flags |= _GRAY_DEFERRED_UPDATE;
    else
        _gray_rb->lcd_update();
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


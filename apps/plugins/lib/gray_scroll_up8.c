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
* gray_scroll_up8() function
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
 Scroll the whole grayscale buffer up by 8 pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the bottom are black
 or white

 Scrolling up/down by 8 pixels is very fast.
 */
void gray_scroll_up8(bool black_border)
{
    int by, d;
    unsigned filler;
    unsigned char *ptr;

    filler = black_border ? 0xFF : 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 1; by < _graybuf->bheight; by++)
    {
        ptr = _graybuf->data + MULU16(_graybuf->width, by);
        for (d = 0; d < _graybuf->depth; d++)
        {
            _gray_rb->memcpy(ptr - _graybuf->width, ptr, _graybuf->width);
            ptr += _graybuf->plane_size;
        }
    }
    /* fill last row */
    ptr = _graybuf->data + _graybuf->plane_size - _graybuf->width;
    for (d = 0; d < _graybuf->depth; d++) 
    {
        _gray_rb->memset(ptr, filler, _graybuf->width);
        ptr += _graybuf->plane_size;
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


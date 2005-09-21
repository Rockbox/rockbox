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
* gray_fillrect() function
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
 Draw a filled rectangle with the current drawinfo
 ----------------------------------------------------------------------------
 See gray_drawpixel() for details

 This one uses the block drawing optimization, so it is rather fast.
 */
void gray_fillrect(int x, int y, int nx, int ny)
{
    int shift;
    unsigned bits, mask_top, mask_bottom;
    unsigned char *dst, *dst_row;
    void (*blockfunc)(unsigned char *address, unsigned mask, unsigned bits);
    
    if ((unsigned) x >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;
        
    if ((unsigned) (y + ny) >= (unsigned) _graybuf->height) /* clip bottom */
        ny = _graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) _graybuf->width) /* clip right */
        nx = _graybuf->width - x;

    dst = _graybuf->data + x + MULU16(_graybuf->width, y >> 3);
    shift = y & 7;
    ny += shift;

    mask_top = 0xFFu << (y & 7);
    mask_bottom = ~(0xFEu << ((ny - 1) & 7));
    if (ny <= 8)
        mask_bottom &= mask_top;
    
    blockfunc = _gray_blockfuncs[_graybuf->drawmode];
    bits = (_graybuf->drawmode == GRAY_DRAW_BG) ? 0u : 0xFFu;

    if (ny > 8)
    {
        dst_row = dst;
        for (x = 0; x < nx; x++)
            blockfunc(dst_row++, mask_top, bits);

        dst += _graybuf->width;
            
        for (y = 8; y < ny - 8; y += 8)
        {
            dst_row = dst;
            for (x = 0; x < nx; x++)
                blockfunc(dst_row++, 0xFFu, bits);

            dst += _graybuf->width;
        }
    }

    for (x = 0; x < nx; x++)
        blockfunc(dst++, mask_bottom, bits);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


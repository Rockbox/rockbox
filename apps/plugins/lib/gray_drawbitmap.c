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
* gray_drawbitmap() function
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
 Display a bitmap with the current drawinfo
 ----------------------------------------------------------------------------
 The drawmode is used as described for gray_set_drawmode()

 This (now) uses the same bitmap format as the core b&w graphics routines,
 so you can use bmp2rb to generate bitmaps for use with this function as
 well.

 A bitmap contains one bit for every pixel that defines if that pixel is
 foreground (1) or background (0). Bits within a byte are arranged
 vertically, LSB at top.
 The bytes are stored in row-major order, with byte 0 being top left,
 byte 1 2nd from left etc. The first row of bytes defines pixel rows
 0..7, the second row defines pixel row 8..15 etc.

 The <stride> parameter is useful if you want to show only a part of a
 bitmap. It should always be set to the "row length" of the bitmap.
 */
void gray_drawbitmap(const unsigned char *src, int x, int y, int nx, int ny,
                     int stride)
{
    int shift;
    unsigned bits, mask_top, mask_bottom;
    const unsigned char *src_col;
    unsigned char *dst, *dst_col;
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

    for(x = 0; x < nx; x++)
    {
        src_col = src++;
        dst_col = dst++;
        bits = 0;
        y = 0;

        if (ny > 8)
        {
            bits = *src_col << shift;
            src_col += stride;

            blockfunc(dst_col, mask_top, bits);
            dst_col += _graybuf->width;
            bits >>= 8;

            for (y = 8; y < ny - 8; y += 8)
            {
                bits |= *src_col << shift;
                src_col += stride;
                
                blockfunc(dst_col, 0xFFu, bits);
                dst_col += _graybuf->width;
                bits >>= 8;
            }
        }
        if (y + shift < ny)
            bits |= *src_col << shift;

        blockfunc(dst_col, mask_bottom, bits);
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


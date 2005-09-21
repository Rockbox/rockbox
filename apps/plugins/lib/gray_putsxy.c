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
* gray_putsxy() function
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
 Display text starting at (x, y) with the current font and drawinfo
 ----------------------------------------------------------------------------
 The drawmode is used as described for gray_set_drawmode()
 */
void gray_putsxy(int x, int y, const unsigned char *str)
{
    int ch, width;
    const unsigned char *bits;
    struct font *pf = _graybuf->curfont;

    if ((unsigned) x >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;

    while ((ch = *str++) != '\0' && x < _graybuf->width)
    {
        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar + pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits */
        width = pf->width ? pf->width[ch] : pf->maxwidth;
        bits = pf->bits + (pf->offset ? pf->offset[ch] :
               MULU16((pf->height + 7) / 8, MULU16(pf->maxwidth, ch)));

        gray_drawbitmap((const unsigned char*) bits, x, y, width, pf->height,
                        width);
        x += width;
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


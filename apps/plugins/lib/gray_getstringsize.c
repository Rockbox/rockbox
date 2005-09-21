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
* gray_getstringsize() function
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
 Calculate width and height of the given text in pixels when rendered with
 the currently selected font.
 ----------------------------------------------------------------------------
 This works exactly the same way as the core lcd_getstringsize(), only that
 it uses the selected font for grayscale.
 */
int gray_getstringsize(const unsigned char *str, int *w, int *h)
{
    int ch;
    int width = 0;
    struct font *pf = _graybuf->curfont;

    while ((ch = *str++))
    {
        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar + pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width */
        width += pf->width ? pf->width[ch] : pf->maxwidth;
    }
    if (w)
        *w = width;
    if (h)
        *h = pf->height;
    return width;
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


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
* gray_scroll_right() function
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

#if CONFIG_LCD == LCD_SSD1815 /* only for Recorder/Ondio */
#include "gray.h"

/*---------------------------------------------------------------------------
 Scroll the whole grayscale buffer right by <count> pixels
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the left are black
 or white

 Scrolling left/right by an even pixel count is almost twice as fast as
 scrolling by an odd pixel count.
 */
void gray_scroll_right(int count, bool black_border)
{
    int by, d;
    unsigned filler;
    unsigned char *ptr;

    if ((unsigned) count >= (unsigned) _graybuf->width)
        return;

    filler = black_border ? 0xFF : 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 0; by < _graybuf->bheight; by++)
    {
        ptr = _graybuf->data + MULU16(_graybuf->width, by);
        for (d = 0; d < _graybuf->depth; d++)
        {
            asm volatile (
                "mov     %0,r1            \n" /* check if both source... */
                "or      %2,r1            \n" /* ...and offset are even */
                "shlr    r1               \n" /* -> lsb = 0 */
                "bf      .sr_start2       \n" /* -> copy word-wise */

                "add     #-1,%2           \n" /* copy byte-wise */
            ".sr_loop1:                   \n"
                "mov.b   @(%2,%0),r1      \n"
                "mov.b   r1,@-%0          \n"
                "cmp/hi  %1,%0            \n"
                "bt      .sr_loop1        \n"
                
                "bra     .sr_end          \n"
                "nop                      \n"
                
            ".sr_start2:                  \n" /* copy word-wise */
                "add     #-2,%2           \n"
            ".sr_loop2:                   \n"
                "mov.w   @(%2,%0),r1      \n"
                "mov.w   r1,@-%0          \n"
                "cmp/hi  %1,%0            \n"
                "bt      .sr_loop2        \n"
                
            ".sr_end:                     \n"
                : /* outputs */
                : /* inputs */
                /* %0 */ "r"(ptr + _graybuf->width),
                /* %1 */ "r"(ptr + count),
                /* %2 */ "z"(-count)
                : /* clobbers */
                "r1"
            );

            _gray_rb->memset(ptr, filler, count);
            ptr += _graybuf->plane_size;
        }
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


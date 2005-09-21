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
* gray_scroll_down() function
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
 Scroll the whole grayscale buffer down by <count> pixels (<= 7)
 ----------------------------------------------------------------------------
 black_border determines if the pixels scrolled in at the top are black
 or white

 Scrolling up/down pixel-wise is significantly slower than scrolling
 left/right or scrolling up/down byte-wise because it involves bit
 shifting. That's why it is asm optimized.
 */
void gray_scroll_down(int count, bool black_border)
{
    unsigned filler;

    if ((unsigned) count > 7)
        return;

    filler = black_border ? (0xFFu << count) : 0;

    /* scroll column by column to minimize flicker */
    asm volatile (
        "mov     #0,r6           \n"  /* x = 0 */
        "mova    .sd_shifttbl,r0 \n"  /* calculate jump destination for */
        "mov.b   @(r0,%6),%6     \n"  /*   shift amount from table */
        "bra     .sd_cloop       \n"  /* skip table */
        "add     r0,%6           \n"
        
        ".align  2               \n"
    ".sd_shifttbl:               \n"  /* shift jump offset table */
        ".byte   .sd_shift0 - .sd_shifttbl \n"
        ".byte   .sd_shift1 - .sd_shifttbl \n"
        ".byte   .sd_shift2 - .sd_shifttbl \n"
        ".byte   .sd_shift3 - .sd_shifttbl \n"
        ".byte   .sd_shift4 - .sd_shifttbl \n"
        ".byte   .sd_shift5 - .sd_shifttbl \n"
        ".byte   .sd_shift6 - .sd_shifttbl \n"
        ".byte   .sd_shift7 - .sd_shifttbl \n"

    ".sd_cloop:                  \n"  /* repeat for every column */
        "mov     %1,r2           \n"  /* get start address */
        "mov     #0,r3           \n"  /* current_plane = 0 */
        
    ".sd_oloop:                  \n"  /* repeat for every bitplane */
        "mov     r2,r4           \n"  /* get start address */
        "mov     #0,r5           \n"  /* current_row = 0 */
        "mov     %5,r1           \n"  /* get filler bits */
        
    ".sd_iloop:                  \n"  /* repeat for all rows */
        "shlr8   r1              \n"  /* shift right to get residue */
        "mov.b   @r4,r0          \n"  /* get data byte */
        "jmp     @%6             \n"  /* jump into shift "path" */
        "extu.b  r0,r0           \n"  /* extend unsigned */
        
    ".sd_shift6:                 \n"  /* shift left by 0..7 bits */
        "shll2   r0              \n"
    ".sd_shift4:                 \n"
        "shll2   r0              \n"
    ".sd_shift2:                 \n"
        "bra     .sd_shift0      \n"
        "shll2   r0              \n"
    ".sd_shift7:                 \n"
        "shll2   r0              \n"
    ".sd_shift5:                 \n"
        "shll2   r0              \n"
    ".sd_shift3:                 \n"
        "shll2   r0              \n"
    ".sd_shift1:                 \n"
        "shll    r0              \n"
    ".sd_shift0:                 \n"

        "or      r0,r1           \n"  /* combine with last residue */
        "mov.b   r1,@r4          \n"  /* store data */
        "add     %2,r4           \n"  /* address += width */
        "add     #1,r5           \n"  /* current_row++ */
        "cmp/hi  r5,%3           \n"  /* current_row < bheight ? */
        "bt      .sd_iloop       \n"

        "add     %4,r2           \n"  /* start_address += plane_size */
        "add     #1,r3           \n"  /* current_plane++ */
        "cmp/hi  r3,%0           \n"  /* current_plane < depth ? */
        "bt      .sd_oloop       \n"

        "add     #1,%1           \n"  /* start_address++ */
        "add     #1,r6           \n"  /* x++ */
        "cmp/hi  r6,%2           \n"  /* x < width ? */
        "bt      .sd_cloop       \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(_graybuf->depth),
        /* %1 */ "r"(_graybuf->data),
        /* %2 */ "r"(_graybuf->width),
        /* %3 */ "r"(_graybuf->bheight),
        /* %4 */ "r"(_graybuf->plane_size),
        /* %5 */ "r"(filler),
        /* %6 */ "r"(count)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6"
    );
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


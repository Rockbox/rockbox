/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Greyscale framework
* Scrolling routines
*
* This is a generic framework to use grayscale display within Rockbox
* plugins. It obviously does not work for the player.
*
* Copyright (C) 2004-2005 Jens Arnold
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

/* FIXME: intermediate solution until we have properly optimised memmove() */
static void *my_memmove(void *dst0, const void *src0, size_t len0)
{
    char *dst = (char *) dst0;
    char *src = (char *) src0;
    
    if (dst <= src)
    {
        while (len0--)
            *dst++ = *src++;
    }
    else
    {
        dst += len0;
        src += len0;
        
        while (len0--)
            *(--dst) = *(--src);
    }

    return dst0;
}

/*** Scrolling ***/

/* Scroll left */
void gray_scroll_left(int count)
{
    long shift, length;
    int blank;

    if ((unsigned)count >= (unsigned)_gray_info.width)
        return;

    shift = MULU16(_gray_info.height, count);
    length = MULU16(_gray_info.height, _gray_info.width - count);
    blank = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
            _gray_info.fg_brightness : _gray_info.bg_brightness;

    my_memmove(_gray_info.cur_buffer, _gray_info.cur_buffer + shift, length);
    _gray_rb->memset(_gray_info.cur_buffer + length, blank, shift);
}

/* Scroll right */
void gray_scroll_right(int count)
{
    long shift, length;
    int blank;

    if ((unsigned)count >= (unsigned)_gray_info.width)
        return;

    shift = MULU16(_gray_info.height, count);
    length = MULU16(_gray_info.height, _gray_info.width - count);
    blank = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
            _gray_info.fg_brightness : _gray_info.bg_brightness;

    my_memmove(_gray_info.cur_buffer + shift, _gray_info.cur_buffer, length);
    _gray_rb->memset(_gray_info.cur_buffer, blank, shift);
}

/* Scroll up */
void gray_scroll_up(int count)
{
    unsigned char *data, *data_end;
    int length, blank;

    if ((unsigned)count >= (unsigned)_gray_info.height)
        return;

    data = _gray_info.cur_buffer;
    data_end = data + MULU16(_gray_info.width, _gray_info.height);
    length = _gray_info.height - count;
    blank = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
            _gray_info.fg_brightness : _gray_info.bg_brightness;

    do
    {
        my_memmove(data, data + count, length);
        _gray_rb->memset(data + length, blank, count);
        data += _gray_info.height;
    }
    while (data < data_end);
}

/* Scroll down */
void gray_scroll_down(int count)
{
    unsigned char *data, *data_end;
    int length, blank;

    if ((unsigned)count >= (unsigned)_gray_info.height)
        return;

    data = _gray_info.cur_buffer;
    data_end = data + MULU16(_gray_info.width, _gray_info.height);
    length = _gray_info.height - count;
    blank = (_gray_info.drawmode & DRMODE_INVERSEVID) ?
            _gray_info.fg_brightness : _gray_info.bg_brightness;

    do
    {
        my_memmove(data + count, data, length);
        _gray_rb->memset(data, blank, count);
        data += _gray_info.height;
    }
    while (data < data_end);
}

/*** Unbuffered scrolling functions ***/

/* Scroll left */
void gray_ub_scroll_left(int count)
{
    int length;
    unsigned char *ptr, *ptr_end;

    if ((unsigned) count >= (unsigned) _gray_info.width)
        return;
        
    length = _gray_info.width - count;
    ptr = _gray_info.plane_data;
    ptr_end = ptr + _gray_info.plane_size;
    
    /* Scroll row by row to minimize flicker (pixel block rows) */
    do
    {
        unsigned char *ptr_row = ptr;
        unsigned char *row_end = ptr_row 
                               + MULU16(_gray_info.plane_size, _gray_info.depth);
        do
        {
            my_memmove(ptr_row, ptr_row + count, length);
            _gray_rb->memset(ptr_row + length, 0, count);
            ptr_row += _gray_info.plane_size;
        }
        while (ptr_row < row_end);

        ptr += _gray_info.width;
    }
    while (ptr < ptr_end);
}

/* Scroll right */
void gray_ub_scroll_right(int count)
{
    int length;
    unsigned char *ptr, *ptr_end;

    if ((unsigned) count >= (unsigned) _gray_info.width)
        return;
        
    length = _gray_info.width - count;
    ptr = _gray_info.plane_data;
    ptr_end = ptr + _gray_info.plane_size;
    
    /* Scroll row by row to minimize flicker (pixel block rows) */
    do
    {
        unsigned char *ptr_row = ptr;
        unsigned char *row_end = ptr_row 
                               + MULU16(_gray_info.plane_size, _gray_info.depth);
        do
        {
            my_memmove(ptr_row + count, ptr_row, length);
            _gray_rb->memset(ptr_row, 0, count);
            ptr_row += _gray_info.plane_size;
        }
        while (ptr_row < row_end);

        ptr += _gray_info.width;
    }
    while (ptr < ptr_end);
}

/* Scroll up */
void gray_ub_scroll_up(int count)
{
    int shift;
    long blockshift = 0;
    unsigned char *ptr, *ptr_end1, *ptr_end2;

    if ((unsigned) count >= (unsigned) _gray_info.height)
        return;
        
    shift = count >> _PBLOCK_EXP;
    count &= (_PBLOCK-1);
    
    if (shift)
    {
        blockshift = MULU16(_gray_info.width, shift);
        ptr      = _gray_info.plane_data;
        ptr_end2 = ptr + _gray_info.plane_size;
        ptr_end1 = ptr_end2 - blockshift;
        /* Scroll row by row to minimize flicker (pixel block rows) */
        do
        {
            unsigned char *ptr_row = ptr;
            unsigned char *row_end = ptr_row 
                             + MULU16(_gray_info.plane_size, _gray_info.depth);
            if (ptr < ptr_end1)
            {
                do
                {
                    _gray_rb->memcpy(ptr_row, ptr_row + blockshift,
                                     _gray_info.width);
                    ptr_row += _gray_info.plane_size;
                }
                while (ptr_row < row_end);
            }
            else
            {
                do
                {
                    _gray_rb->memset(ptr_row, 0, _gray_info.width);
                    ptr_row += _gray_info.plane_size;
                }
                while (ptr_row < row_end);
            }

            ptr += _gray_info.width;
        }
        while (ptr < ptr_end2);
    }
    if (count)
    {
#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
        /* scroll column by column to minimize flicker */
        asm (
            "mov     #0,r6       \n"  /* x = 0 */
            "mova    .su_shifttbl,r0 \n"  /* calculate jump destination for */
            "mov.b   @(r0,%5),%5 \n"  /* shift amount from table */
            "bra     .su_cloop   \n"  /* skip table */
            "add     r0,%5       \n"

            ".align  2           \n"
        ".su_shifttbl:           \n"  /* shift jump offset table */
            ".byte   .su_shift0 - .su_shifttbl \n"
            ".byte   .su_shift1 - .su_shifttbl \n"
            ".byte   .su_shift2 - .su_shifttbl \n"
            ".byte   .su_shift3 - .su_shifttbl \n"
            ".byte   .su_shift4 - .su_shifttbl \n"
            ".byte   .su_shift5 - .su_shifttbl \n"
            ".byte   .su_shift6 - .su_shifttbl \n"
            ".byte   .su_shift7 - .su_shifttbl \n"

        ".su_cloop:              \n"  /* repeat for every column */
            "mov     %1,r2       \n"  /* get start address */
            "mov     #0,r3       \n"  /* current_plane = 0 */

        ".su_oloop:              \n"  /* repeat for every bitplane */
            "mov     r2,r4       \n"  /* get start address */
            "mov     #0,r5       \n"  /* current_row = 0 */
            "mov     #0,r1       \n"  /* fill with zero */

        ".su_iloop:              \n"  /* repeat for all rows */
            "sub     %2,r4       \n"  /* address -= width */
            "mov.b   @r4,r0      \n"  /* get data byte */
            "shll8   r1          \n"  /* old data to 2nd byte */
            "extu.b  r0,r0       \n"  /* extend unsigned */
            "or      r1,r0       \n"  /* combine old data */
            "jmp     @%5         \n"  /* jump into shift "path" */
            "extu.b  r0,r1       \n"  /* store data for next round */

        ".su_shift6:             \n"  /* shift right by 0..7 bits */
            "shlr2   r0          \n"
        ".su_shift4:             \n"
            "shlr2   r0          \n"
        ".su_shift2:             \n"
            "bra     .su_shift0  \n"
            "shlr2   r0          \n"
        ".su_shift7:             \n"
            "shlr2   r0          \n"
        ".su_shift5:             \n"
            "shlr2   r0          \n"
        ".su_shift3:             \n"
            "shlr2   r0          \n"
        ".su_shift1:             \n"
            "shlr    r0          \n"
        ".su_shift0:             \n"

            "mov.b   r0,@r4      \n"  /* store data */
            "add     #1,r5       \n"  /* current_row++ */
            "cmp/hi  r5,%3       \n"  /* current_row < bheight - shift ? */
            "bt      .su_iloop   \n"

            "add     %4,r2       \n"  /* start_address += plane_size */
            "add     #1,r3       \n"  /* current_plane++ */
            "cmp/hi  r3,%0       \n"  /* current_plane < depth ? */
            "bt      .su_oloop   \n"

            "add     #1,%1       \n"  /* start_address++ */
            "add     #1,r6       \n"  /* x++ */
            "cmp/hi  r6,%2       \n"  /* x < width ? */
            "bt      .su_cloop   \n"
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(_gray_info.depth),
            /* %1 */ "r"(_gray_info.plane_data + _gray_info.plane_size - blockshift),
            /* %2 */ "r"(_gray_info.width),
            /* %3 */ "r"(_gray_info.bheight - shift),
            /* %4 */ "r"(_gray_info.plane_size),
            /* %5 */ "r"(count)
            : /* clobbers */
            "r0", "r1", "r2", "r3", "r4", "r5", "r6"
        );
#endif
    }
}

/* Scroll down */
void gray_ub_scroll_down(int count)
{
    int shift;
    long blockshift = 0;
    unsigned char *ptr, *ptr_end1, *ptr_end2;

    if ((unsigned) count >= (unsigned) _gray_info.height)
        return;

    shift = count >> _PBLOCK_EXP;
    count &= (_PBLOCK-1);
    
    if (shift)
    {
        blockshift = MULU16(_gray_info.width, shift);
        ptr_end2 = _gray_info.plane_data;
        ptr_end1 = ptr_end2 + blockshift;
        ptr      = ptr_end2 + _gray_info.plane_size;
        /* Scroll row by row to minimize flicker (pixel block rows) */
        do
        {
            unsigned char *ptr_row, *row_end;

            ptr -= _gray_info.width;
            ptr_row = ptr;
            row_end = ptr_row + MULU16(_gray_info.plane_size, _gray_info.depth);

            if (ptr >= ptr_end1)
            {
                do
                {
                    _gray_rb->memcpy(ptr_row, ptr_row - blockshift,
                                     _gray_info.width);
                    ptr_row += _gray_info.plane_size;
                }
                while (ptr_row < row_end);
            }
            else
            {
                do
                {
                    _gray_rb->memset(ptr_row, 0, _gray_info.width);
                    ptr_row += _gray_info.plane_size;
                }
                while (ptr_row < row_end);
            }
        }
        while (ptr > ptr_end2);
    }
    if (count)
    {
#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
        /* scroll column by column to minimize flicker */
        asm (
            "mov     #0,r6       \n"  /* x = 0 */
            "mova    .sd_shifttbl,r0 \n"  /* calculate jump destination for */
            "mov.b   @(r0,%5),%5 \n"  /* shift amount from table */
            "bra     .sd_cloop   \n"  /* skip table */
            "add     r0,%5       \n"

            ".align  2           \n"
        ".sd_shifttbl:           \n"  /* shift jump offset table */
            ".byte   .sd_shift0 - .sd_shifttbl \n"
            ".byte   .sd_shift1 - .sd_shifttbl \n"
            ".byte   .sd_shift2 - .sd_shifttbl \n"
            ".byte   .sd_shift3 - .sd_shifttbl \n"
            ".byte   .sd_shift4 - .sd_shifttbl \n"
            ".byte   .sd_shift5 - .sd_shifttbl \n"
            ".byte   .sd_shift6 - .sd_shifttbl \n"
            ".byte   .sd_shift7 - .sd_shifttbl \n"

        ".sd_cloop:              \n"  /* repeat for every column */
            "mov     %1,r2       \n"  /* get start address */
            "mov     #0,r3       \n"  /* current_plane = 0 */

        ".sd_oloop:              \n"  /* repeat for every bitplane */
            "mov     r2,r4       \n"  /* get start address */
            "mov     #0,r5       \n"  /* current_row = 0 */
            "mov     #0,r1       \n"  /* fill with zero */

        ".sd_iloop:              \n"  /* repeat for all rows */
            "shlr8   r1          \n"  /* shift right to get residue */
            "mov.b   @r4,r0      \n"  /* get data byte */
            "jmp     @%5         \n"  /* jump into shift "path" */
            "extu.b  r0,r0       \n"  /* extend unsigned */

        ".sd_shift6:             \n"  /* shift left by 0..7 bits */
            "shll2   r0          \n"
        ".sd_shift4:             \n"
            "shll2   r0          \n"
        ".sd_shift2:             \n"
            "bra     .sd_shift0  \n"
            "shll2   r0          \n"
        ".sd_shift7:             \n"
            "shll2   r0          \n"
        ".sd_shift5:             \n"
            "shll2   r0          \n"
        ".sd_shift3:             \n"
            "shll2   r0          \n"
        ".sd_shift1:             \n"
            "shll    r0          \n"
        ".sd_shift0:             \n"

            "or      r0,r1       \n"  /* combine with last residue */
            "mov.b   r1,@r4      \n"  /* store data */
            "add     %2,r4       \n"  /* address += width */
            "add     #1,r5       \n"  /* current_row++ */
            "cmp/hi  r5,%3       \n"  /* current_row < bheight - shift ? */
            "bt      .sd_iloop   \n"

            "add     %4,r2       \n"  /* start_address += plane_size */
            "add     #1,r3       \n"  /* current_plane++ */
            "cmp/hi  r3,%0       \n"  /* current_plane < depth ? */
            "bt      .sd_oloop   \n"

            "add     #1,%1       \n"  /* start_address++ */
            "add     #1,r6       \n"  /* x++ */
            "cmp/hi  r6,%2       \n"  /* x < width ? */
            "bt      .sd_cloop   \n"
            : /* outputs */
            : /* inputs */
            /* %0 */ "r"(_gray_info.depth),
            /* %1 */ "r"(_gray_info.plane_data + blockshift),
            /* %2 */ "r"(_gray_info.width),
            /* %3 */ "r"(_gray_info.bheight - shift),
            /* %4 */ "r"(_gray_info.plane_size),
            /* %5 */ "r"(count)
            : /* clobbers */
            "r0", "r1", "r2", "r3", "r4", "r5", "r6"
        );
#endif
    }
}

#endif /* HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */


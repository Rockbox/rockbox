/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Additional LCD routines not present in the rockbox core
* Scrolling functions
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP
#include "xlcd.h"

#if (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
static const unsigned short patterns[4] = {0xFFFF, 0xFF00, 0x00FF, 0x0000};
#endif

#if (LCD_PIXELFORMAT == HORIZONTAL_PACKING) && (LCD_DEPTH < 8)

/* Scroll left */
void xlcd_scroll_left(int count)
{
    int bitcount, oldmode;
    int blockcount, blocklen;

    if ((unsigned) count >= LCD_WIDTH)
        return;
        
#if LCD_DEPTH == 2
    blockcount = count >> 2;
    blocklen = LCD_FBWIDTH - blockcount;
    bitcount = 2 * (count & 3);
#endif

    if (blockcount)
    {
        unsigned char *data = _xlcd_rb->lcd_framebuffer;
        unsigned char *data_end = data + LCD_FBWIDTH*LCD_HEIGHT;

        do
        {
            _xlcd_rb->memmove(data, data + blockcount, blocklen);
            data += LCD_FBWIDTH;
        }
        while (data < data_end);
    }
    if (bitcount)
    {
        int bx, y;
        unsigned char *addr = _xlcd_rb->lcd_framebuffer + blocklen;
#if LCD_DEPTH == 2
        unsigned fill = 0x55 * (~_xlcd_rb->lcd_get_background() & 3);
#endif

        for (y = 0; y < LCD_HEIGHT; y++)
        {
            unsigned char *row_addr = addr;
            unsigned data = fill;

            for (bx = 0; bx < blocklen; bx++)
            {
                --row_addr;
                data = (data >> 8) | (*row_addr << bitcount);
                *row_addr = data;
            }
            addr += LCD_FBWIDTH;
        }
    }
    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(LCD_WIDTH - count, 0, count, LCD_HEIGHT);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

/* Scroll right */
void xlcd_scroll_right(int count)
{
    int bitcount, oldmode;
    int blockcount, blocklen;

    if ((unsigned) count >= LCD_WIDTH)
        return;
        
#if LCD_DEPTH == 2
    blockcount = count >> 2;
    blocklen = LCD_FBWIDTH - blockcount;
    bitcount = 2 * (count & 3);
#endif

    if (blockcount)
    {
        unsigned char *data = _xlcd_rb->lcd_framebuffer;
        unsigned char *data_end = data + LCD_FBWIDTH*LCD_HEIGHT;

        do
        {
            _xlcd_rb->memmove(data + blockcount, data, blocklen);
            data += LCD_FBWIDTH;
        }
        while (data < data_end);
    }
    if (bitcount)
    {
        int bx, y;
        unsigned char *addr = _xlcd_rb->lcd_framebuffer + blockcount;
#if LCD_DEPTH == 2
        unsigned fill = 0x55 * (~_xlcd_rb->lcd_get_background() & 3);
#endif

        for (y = 0; y < LCD_HEIGHT; y++)
        {
            unsigned char *row_addr = addr;
            unsigned data = fill;

            for (bx = 0; bx < blocklen; bx++)
            {
                data = (data << 8) | *row_addr;
                *row_addr = data >> bitcount;
                row_addr++;
            }
            addr += LCD_FBWIDTH;
        }
    }
    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

#else /* LCD_PIXELFORMAT vertical packed or >= 8bit / pixel */

/* Scroll left */
void xlcd_scroll_left(int count)
{
    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
        return;

    data = _xlcd_rb->lcd_framebuffer;
    data_end = data + LCD_WIDTH*LCD_FBHEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        _xlcd_rb->memmove(data, data + count, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(length, 0, count, LCD_HEIGHT);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

/* Scroll right */
void xlcd_scroll_right(int count)
{
    fb_data *data, *data_end;
    int length, oldmode;

    if ((unsigned)count >= LCD_WIDTH)
        return;

    data = _xlcd_rb->lcd_framebuffer;
    data_end = data + LCD_WIDTH*LCD_FBHEIGHT;
    length = LCD_WIDTH - count;

    do
    {
        _xlcd_rb->memmove(data + count, data, length * sizeof(fb_data));
        data += LCD_WIDTH;
    }
    while (data < data_end);

    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, 0, count, LCD_HEIGHT);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

#endif /* LCD_PIXELFORMAT, LCD_DEPTH */

#if (LCD_PIXELFORMAT == HORIZONTAL_PACKING) || (LCD_DEPTH >= 8)

/* Scroll up */
void xlcd_scroll_up(int count)
{
    int length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
        return;

    length = LCD_HEIGHT - count;

    _xlcd_rb->memmove(_xlcd_rb->lcd_framebuffer,
                      _xlcd_rb->lcd_framebuffer + count * LCD_FBWIDTH,
                      length * LCD_FBWIDTH * sizeof(fb_data));

    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, length, LCD_WIDTH, count);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

/* Scroll down */
void xlcd_scroll_down(int count)
{
    int length, oldmode;

    if ((unsigned)count >= LCD_HEIGHT)
        return;

    length = LCD_HEIGHT - count;

    _xlcd_rb->memmove(_xlcd_rb->lcd_framebuffer + count * LCD_FBWIDTH,
                      _xlcd_rb->lcd_framebuffer,
                      length * LCD_FBWIDTH * sizeof(fb_data));

    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

#else /* LCD_PIXELFORMAT == VERTICAL_PACKING, 
         LCD_PIXELFORMAT == VERTICAL_INTERLEAVED */

/* Scroll up */
void xlcd_scroll_up(int count)
{
    int bitcount, oldmode;
    int blockcount, blocklen;

    if ((unsigned) count >= LCD_HEIGHT)
        return;
        
#if (LCD_DEPTH == 1) \
 || (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    blockcount = count >> 3;
    bitcount = count & 7;
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_PACKING)
    blockcount = count >> 2;
    bitcount = 2 * (count & 3);
#endif
    blocklen = LCD_FBHEIGHT - blockcount;

    if (blockcount)
    {
        _xlcd_rb->memmove(_xlcd_rb->lcd_framebuffer,
                          _xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH,
                          blocklen * LCD_FBWIDTH * sizeof(fb_data));
    }
    if (bitcount)
    {
#if LCD_PIXELFORMAT == VERTICAL_PACKING

#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
        asm (
            "mov     #0,r4       \n"  /* x = 0 */
            "mova    .su_shifttbl,r0 \n"  /* calculate jump destination for */
            "mov.b   @(r0,%[cnt]),%[cnt] \n"  /* shift amount from table */
            "bra     .su_cloop   \n"  /* skip table */
            "add     r0,%[cnt]   \n"

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
            "mov     %[addr],r2  \n"  /* get start address */
            "mov     #0,r3       \n"  /* current_row = 0 */
            "mov     #0,r1       \n"  /* fill with zero */

        ".su_iloop:              \n"  /* repeat for all rows */
            "sub     %[wide],r2  \n"  /* address -= width */
            "mov.b   @r2,r0      \n"  /* get data byte */
            "shll8   r1          \n"  /* old data to 2nd byte */
            "extu.b  r0,r0       \n"  /* extend unsigned */
            "or      r1,r0       \n"  /* combine old data */
            "jmp     @%[cnt]     \n"  /* jump into shift "path" */
            "extu.b  r0,r1       \n"  /* store data for next round */

        ".su_shift6:             \n"  /* shift right by 0..7 bits */
            "shll2   r0          \n"
            "bra     .su_shift0  \n"
            "shlr8   r0          \n"
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

            "mov.b   r0,@r2      \n"  /* store data */
            "add     #1,r3       \n"  /* current_row++ */
            "cmp/hi  r3,%[rows]  \n"  /* current_row < bheight - shift ? */
            "bt      .su_iloop   \n"

            "add     #1,%[addr]  \n"  /* start_address++ */
            "add     #1,r4       \n"  /* x++ */
            "cmp/hi  r4,%[wide]  \n"  /* x < width ? */
            "bt      .su_cloop   \n"
            : /* outputs */
            : /* inputs */
            [addr]"r"(_xlcd_rb->lcd_framebuffer + blocklen * LCD_FBWIDTH),
            [wide]"r"(LCD_FBWIDTH),
            [rows]"r"(blocklen),
            [cnt] "r"(bitcount)
            : /* clobbers */
            "r0", "r1", "r2", "r3", "r4"
        );
#elif defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
        asm (
            "move.l  %[wide],%%d3\n"  /* columns = width */

        ".su_cloop:              \n"  /* repeat for every column */
            "move.l  %[addr],%%a1\n"  /* get start address */
            "move.l  %[rows],%%d2\n"  /* rows = row_count */
            "move.l  %[bkg],%%d1 \n"  /* fill with background */

        ".su_iloop:              \n"  /* repeat for all rows */
            "sub.l   %[wide],%%a1\n"  /* address -= width */

            "clr.l   %%d0        \n"
            "move.b  (%%a1),%%d0 \n"  /* get data byte */
            "lsl.l   #8,%%d1     \n"  /* old data to 2nd byte */
            "or.l    %%d1,%%d0   \n"  /* combine old data */
            "clr.l   %%d1        \n"
            "move.b  %%d0,%%d1   \n"  /* keep data for next round */
            "lsr.l   %[cnt],%%d0 \n"  /* shift right */
            "move.b  %%d0,(%%a1) \n"  /* store data */

            "subq.l  #1,%%d2     \n"  /* rows-- */
            "bne.b   .su_iloop   \n"

            "addq.l  #1,%[addr]  \n"  /* start_address++ */
            "subq.l  #1,%%d3     \n"  /* columns-- */
            "bne.b   .su_cloop   \n"
            : /* outputs */
            : /* inputs */
            [wide]"r"(LCD_FBWIDTH),
            [rows]"r"(blocklen),
            [addr]"a"(_xlcd_rb->lcd_framebuffer + blocklen * LCD_FBWIDTH),
            [cnt] "d"(bitcount),
            [bkg] "d"(0x55 * (~_xlcd_rb->lcd_get_background() & 3))
            : /* clobbers */
            "a1", "d0", "d1", "d2", "d3"
        );
#else /* C version */
        int x, by;
        unsigned char *addr = _xlcd_rb->lcd_framebuffer + blocklen * LCD_FBWIDTH;
#if LCD_DEPTH == 2
        unsigned fill = 0x55 * (~_xlcd_rb->lcd_get_background() & 3);
#else
        const unsigned fill = 0;
#endif

        for (x = 0; x < LCD_WIDTH; x++)
        {
            unsigned char *col_addr = addr++;
            unsigned data = fill;

            for (by = 0; by < blocklen; by++)
            {
                col_addr -= LCD_FBWIDTH;
                data = (data << 8) | *col_addr;
                *col_addr = data >> bitcount;
            }
        }
#endif /* CPU, LCD_DEPTH */

#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED

#if LCD_DEPTH == 2
        int x, by;
        fb_data *addr = _xlcd_rb->lcd_framebuffer + blocklen * LCD_FBWIDTH;
        unsigned fill, mask;

        fill = patterns[_xlcd_rb->lcd_get_background() & 3] << 8;
        mask = (0xFFu >> bitcount) << bitcount;
        mask |= mask << 8;

        for (x = 0; x < LCD_WIDTH; x++)
        {
            fb_data *col_addr = addr++;
            unsigned olddata = fill;
            unsigned data;

            for (by = 0; by < blocklen; by++)
            {
                col_addr -= LCD_FBWIDTH;
                data = *col_addr;
                *col_addr = (olddata ^ ((data ^ olddata) & mask)) >> bitcount;
                olddata = data << 8;
            }
        }
#endif /* LCD_DEPTH == 2 */

#endif /* LCD_PIXELFORMAT */
    }
    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, LCD_HEIGHT - count, LCD_WIDTH, count);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

/* Scroll up */
void xlcd_scroll_down(int count)
{
    int bitcount, oldmode;
    int blockcount, blocklen;

    if ((unsigned) count >= LCD_HEIGHT)
        return;
        
#if (LCD_DEPTH == 1) \
 || (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_INTERLEAVED)
    blockcount = count >> 3;
    bitcount = count & 7;
#elif (LCD_DEPTH == 2) && (LCD_PIXELFORMAT == VERTICAL_PACKING)
    blockcount = count >> 2;
    bitcount = 2 * (count & 3);
#endif
    blocklen = LCD_FBHEIGHT - blockcount;

    if (blockcount)
    {
        _xlcd_rb->memmove(_xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH,
                          _xlcd_rb->lcd_framebuffer,
                          blocklen * LCD_FBWIDTH * sizeof(fb_data));
    }
    if (bitcount)
    {
#if LCD_PIXELFORMAT == VERTICAL_PACKING

#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
        asm (
            "mov     #0,r4       \n"  /* x = 0 */
            "mova    .sd_shifttbl,r0 \n"  /* calculate jump destination for */
            "mov.b   @(r0,%[cnt]),%[cnt] \n"  /* shift amount from table */
            "bra     .sd_cloop   \n"  /* skip table */
            "add     r0,%[cnt]   \n"

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
            "mov     %[addr],r2  \n"  /* get start address */
            "mov     #0,r3       \n"  /* current_row = 0 */
            "mov     #0,r1       \n"  /* fill with zero */

        ".sd_iloop:              \n"  /* repeat for all rows */
            "shlr8   r1          \n"  /* shift right to get residue */
            "mov.b   @r2,r0      \n"  /* get data byte */
            "jmp     @%[cnt]     \n"  /* jump into shift "path" */
            "extu.b  r0,r0       \n"  /* extend unsigned */

        ".sd_shift6:             \n"  /* shift left by 0..7 bits */
            "shll8   r0          \n"
            "bra     .sd_shift0  \n"
            "shlr2   r0          \n"
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
            "mov.b   r1,@r2      \n"  /* store data */
            "add     %[wide],r2  \n"  /* address += width */
            "add     #1,r3       \n"  /* current_row++ */
            "cmp/hi  r3,%[rows]  \n"  /* current_row < bheight - shift ? */
            "bt      .sd_iloop   \n"

            "add     #1,%[addr]  \n"  /* start_address++ */
            "add     #1,r4       \n"  /* x++ */
            "cmp/hi  r4,%[wide]  \n"  /* x < width ? */
            "bt      .sd_cloop   \n"
            : /* outputs */
            : /* inputs */
            [addr]"r"(_xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH),
            [wide]"r"(LCD_WIDTH),
            [rows]"r"(blocklen),
            [cnt] "r"(bitcount)
            : /* clobbers */
            "r0", "r1", "r2", "r3", "r4"
        );
#elif defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
        asm (
            "move.l  %[wide],%%d3\n"  /* columns = width */

        ".sd_cloop:              \n"  /* repeat for every column */
            "move.l  %[addr],%%a1\n"  /* get start address */
            "move.l  %[rows],%%d2\n"  /* rows = row_count */
            "move.l  %[bkg],%%d1 \n"  /* fill with background */

        ".sd_iloop:              \n"  /* repeat for all rows */
            "lsr.l   #8,%%d1     \n"  /* shift right to get residue */
            "clr.l   %%d0        \n"
            "move.b  (%%a1),%%d0 \n"  /* get data byte */
            "lsl.l   %[cnt],%%d0 \n"
            "or.l    %%d0,%%d1   \n"  /* combine with last residue */
            "move.b  %%d1,(%%a1) \n"  /* store data */

            "add.l   %[wide],%%a1\n"  /* address += width */
            "subq.l  #1,%%d2     \n"  /* rows-- */
            "bne.b   .sd_iloop   \n"

            "lea.l   (1,%[addr]),%[addr] \n"  /* start_address++ */
            "subq.l  #1,%%d3     \n"  /* columns-- */
            "bne.b   .sd_cloop   \n"
            : /* outputs */
            : /* inputs */
            [wide]"r"(LCD_WIDTH),
            [rows]"r"(blocklen),
            [addr]"a"(_xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH),
            [cnt] "d"(bitcount),
            [bkg] "d"((0x55 * (~_xlcd_rb->lcd_get_background() & 3)) << bitcount)
            : /* clobbers */
            "a1", "d0", "d1", "d2", "d3"
        );
#else /* C version */
        int x, by;
        unsigned char *addr = _xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH;
#if LCD_DEPTH == 2
        unsigned fill = (0x55 * (~_xlcd_rb->lcd_get_background() & 3)) << bitcount;
#else
        const unsigned fill = 0;
#endif

        for (x = 0; x < LCD_WIDTH; x++)
        {
            unsigned char *col_addr = addr++;
            unsigned data = fill;

            for (by = 0; by < blocklen; by++)
            {
                data = (data >> 8) | (*col_addr << bitcount);
                *col_addr = data;
                col_addr += LCD_FBWIDTH;
            }
        }
#endif /* CPU, LCD_DEPTH */

#elif LCD_PIXELFORMAT == VERTICAL_INTERLEAVED

#if LCD_DEPTH == 2
        int x, by;
        fb_data *addr = _xlcd_rb->lcd_framebuffer + blockcount * LCD_FBWIDTH;
        unsigned fill, mask;
        
        fill = patterns[_xlcd_rb->lcd_get_background() & 3] >> (8 - bitcount);
        mask = (0xFFu >> bitcount) << bitcount;
        mask |= mask << 8;

        for (x = 0; x < LCD_WIDTH; x++)
        {
            fb_data *col_addr = addr++;
            unsigned olddata = fill;
            unsigned data;

            for (by = 0; by < blocklen; by++)
            {
                data = *col_addr << bitcount;
                *col_addr = olddata ^ ((data ^ olddata) & mask);
                olddata = data >> 8;
                col_addr += LCD_FBWIDTH;
            }
        }
#endif /* LCD_DEPTH == 2 */

#endif /* LCD_PIXELFORMAT */
    }
    oldmode = _xlcd_rb->lcd_get_drawmode();
    _xlcd_rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    _xlcd_rb->lcd_fillrect(0, 0, LCD_WIDTH, count);
    _xlcd_rb->lcd_set_drawmode(oldmode);
}

#endif /* LCD_PIXELFORMAT, LCD_DEPTH */

#endif /* HAVE_LCD_BITMAP */

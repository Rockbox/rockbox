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
* Core & miscellaneous functions
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

/* Global variables */
struct plugin_api *_gray_rb = NULL; /* global api struct pointer */
struct _gray_info _gray_info;       /* global info structure */
short _gray_random_buffer;          /* buffer for random number generator */

/* Prototypes */
static void _timer_isr(void);

/* Timer interrupt handler: display next bitplane */
static void _timer_isr(void)
{
    _gray_rb->lcd_blit(_gray_info.plane_data + MULU16(_gray_info.plane_size,
                       _gray_info.cur_plane), _gray_info.x, _gray_info.by,
                       _gray_info.width, _gray_info.bheight, _gray_info.width);

    if (++_gray_info.cur_plane >= _gray_info.depth)
        _gray_info.cur_plane = 0;

    if (_gray_info.flags & _GRAY_DEFERRED_UPDATE)  /* lcd_update() requested? */
    {
        int x1 = MAX(_gray_info.x, 0);
        int x2 = MIN(_gray_info.x + _gray_info.width, LCD_WIDTH);
        int y1 = MAX(_gray_info.by << _PBLOCK_EXP, 0);
        int y2 = MIN((_gray_info.by << _PBLOCK_EXP) + _gray_info.height, LCD_HEIGHT);

        if (y1 > 0)  /* refresh part above overlay, full width */
            _gray_rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

        if (y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
            _gray_rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

        if (x1 > 0) /* refresh part to the left of overlay */
            _gray_rb->lcd_update_rect(0, y1, x1, y2 - y1);

        if (x2 < LCD_WIDTH) /* refresh part to the right of overlay */
            _gray_rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);

        _gray_info.flags &= ~_GRAY_DEFERRED_UPDATE; /* clear request */
    }
}

/* Initialise the framework and prepare the greyscale display buffer

 arguments:
   newrb     = pointer to plugin api
   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
   gbuf_size = max usable size of the buffer
   buffered  = use chunky pixel buffering with delta buffer?
               This allows to use all drawing functions, but needs more
               memory. Unbuffered operation provides only a subset of
               drawing functions. (only gray_bitmap drawing and scrolling)
   width     = width in pixels  (1..LCD_WIDTH)
   bheight   = height in LCD pixel-block units (8 pixels for b&w LCD,
               4 pixels for 4-grey LCD) (1..LCD_HEIGHT/block_size)
   depth     = number of bitplanes to use (1..32 for b&w LCD, 1..16 for
               4-grey LCD).

 result:
   = depth  if there was enough memory
   < depth  if there wasn't enough memory. The number of displayable
            shades is smaller than desired, but it still works
   = 0      if there wasn't even enough memory for 1 bitplane

   You can request any depth in the allowed range, not just powers of 2. The
   routine performs "graceful degradation" if the memory is not sufficient for
   the desired depth. As long as there is at least enough memory for 1 bitplane,
   it creates as many bitplanes as fit into memory, although 1 bitplane won't
   deliver an enhancement over the native display.
 
   The number of displayable shades is calculated as follows:
   b&w LCD:    shades = depth + 1
   4-grey LCD: shades = 3 * depth + 1

   If you need info about the memory taken by the greyscale buffer, supply a
   long* as the last parameter. This long will then contain the number of bytes
   used. The total memory needed can be calculated as follows:
 total_mem =
     shades * sizeof(long)               (bitpatterns)
   + (width * bheight) * depth           (bitplane data)
   + buffered ?                          (chunky front- & backbuffer)
       (width * bheight * 8(4) * 2) : 0
   + 0..3                                (longword alignment) */
int gray_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
              bool buffered, int width, int bheight, int depth, long *buf_taken)
{
    int possible_depth, i, j;
    long plane_size, buftaken;
    
    _gray_rb = newrb;

    if ((unsigned) width > LCD_WIDTH
        || (unsigned) bheight > (LCD_HEIGHT/_PBLOCK)
        || depth < 1)
        return 0;

    /* the buffer has to be long aligned */
    buftaken = (-(long)gbuf) & 3;
    gbuf += buftaken;

    /* chunky front- & backbuffer */
    if (buffered)  
    {
        plane_size = MULU16(width, bheight << _PBLOCK_EXP);
        buftaken += 2 * plane_size;
        if (buftaken > gbuf_size)
            return 0;

        _gray_info.cur_buffer = gbuf;
        gbuf += plane_size;
        /* set backbuffer to 0xFF to guarantee the initial full update */
        _gray_rb->memset(gbuf, 0xFF, plane_size);
        _gray_info.back_buffer = gbuf;
        gbuf += plane_size;
    }

    plane_size = MULU16(width, bheight);
    possible_depth = (gbuf_size - buftaken - sizeof(long))
                     / (plane_size + sizeof(long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, 32);
    depth = MIN(depth, possible_depth);

    _gray_info.x = 0;
    _gray_info.by = 0;
    _gray_info.width = width;
    _gray_info.height = bheight << _PBLOCK_EXP;
    _gray_info.bheight = bheight;
    _gray_info.plane_size = plane_size;
    _gray_info.depth = depth;
    _gray_info.cur_plane = 0;
    _gray_info.flags = 0;
    _gray_info.plane_data = gbuf;
    gbuf += depth * plane_size;
    _gray_info.bitpattern = (unsigned long *)gbuf;
    buftaken += depth * plane_size + (depth + 1) * sizeof(long);
              
    i = depth - 1;
    j = 8;
    while (i != 0)
    {
        i >>= 1;
        j--;
    }
    _gray_info.randmask = 0xFFu >> j;

    /* Precalculate the bit patterns for all possible pixel values */
    for (i = 0; i <= depth; i++)
    {
        unsigned long pattern = 0;
        int value = 0;

        for (j = 0; j < depth; j++)
        {
            pattern <<= 1;
            value += i;

            if (value >= depth)
                value -= depth;   /* "white" bit */
            else
                pattern |= 1;     /* "black" bit */
        }
        /* now the lower <depth> bits contain the pattern */

        _gray_info.bitpattern[i] = pattern;
    }

    _gray_info.fg_brightness = 0;
    _gray_info.bg_brightness = depth;
    _gray_info.drawmode = DRMODE_SOLID;
    _gray_info.curfont = FONT_SYSFIXED;

    if (buf_taken)  /* caller requested info about space taken */
        *buf_taken = buftaken;

    return depth;
}

/* Release the greyscale display buffer and the library
   DO CALL either this function or at least gray_show_display(false)
   before you exit, otherwise nasty things may happen. */
void gray_release(void)
{
    gray_show(false);
}

/* Switch the greyscale overlay on or off
   DO NOT call lcd_update() or any other api function that directly accesses
   the lcd while the greyscale overlay is running! If you need to do
   lcd_update() to update something outside the greyscale overlay area, use
   gray_deferred_update() instead.

 Other functions to avoid are:
   lcd_blit() (obviously), lcd_update_rect(), lcd_set_contrast(),
   lcd_set_invert_display(), lcd_set_flip(), lcd_roll() */
void gray_show(bool enable)
{
#if (CONFIG_CPU == SH7034) && (CONFIG_LCD == LCD_SSD1815)
    if (enable)
    {
        _gray_info.flags |= _GRAY_RUNNING;
        _gray_rb->timer_register(1, NULL, FREQ / 67, 1, _timer_isr);
    }
    else
    {
        _gray_rb->timer_unregister();
        _gray_info.flags &= ~_GRAY_RUNNING;
        _gray_rb->lcd_update(); /* restore whatever there was before */
    }
#elif defined(CPU_COLDFIRE) && (CONFIG_LCD == LCD_S1D15E06)
    if (enable && !(_gray_info.flags & _GRAY_RUNNING))
    {
        _gray_info.flags |= _GRAY_RUNNING;
        _gray_rb->cpu_boost(true);  /* run at 120 MHz to avoid freq changes */
        _gray_rb->timer_register(1, NULL, *_gray_rb->cpu_frequency / 70, 1,
                                 _timer_isr);
    }
    else if (!enable && (_gray_info.flags & _GRAY_RUNNING))
    {
        _gray_rb->timer_unregister();
        _gray_rb->cpu_boost(false);
        _gray_info.flags &= ~_GRAY_RUNNING;
        _gray_rb->lcd_update(); /* restore whatever there was before */
    }
#endif
}

/* Update a rectangular area of the greyscale overlay */
void gray_update_rect(int x, int y, int width, int height)
{
    int ymax;
    long srcofs;
    unsigned char *dst;

    if (width <= 0)
        return; /* nothing to do */

    /* The Y coordinates have to work on whole pixel block rows */
    ymax = (y + height - 1) >> _PBLOCK_EXP;
    y >>= _PBLOCK_EXP;

    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (ymax >= _gray_info.bheight)
        ymax = _gray_info.bheight - 1;
        
    srcofs = (y << _PBLOCK_EXP) + MULU16(_gray_info.height, x);
    dst = _gray_info.plane_data + MULU16(_gray_info.width, y) + x;
    
    /* Copy specified rectange bitmap to hardware */
    for (; y <= ymax; y++)
    {
        long srcofs_row = srcofs;
        unsigned char *dst_row = dst;
        unsigned char *dst_end = dst_row + width;

        do
        {
#if (CONFIG_CPU == SH7034) && (LCD_DEPTH == 1)
            unsigned long pat_stack[8];
            unsigned long *pat_ptr;
            unsigned change, ofs;

            ofs = srcofs_row;
            asm volatile (
                "mov.l   @(%[ofs],%[cbuf]),r1\n"
                "mov.l   @(%[ofs],%[bbuf]),r2\n"
                "xor     r1,r2           \n"
                "add     #4,%[ofs]       \n"
                "mov.l   @(%[ofs],%[cbuf]),r1\n"
                "mov.l   @(%[ofs],%[bbuf]),%[chg]\n"
                "xor     r1,%[chg]       \n"
                "or      r2,%[chg]       \n"
                : /* outputs */
                [ofs] "+z"(ofs),
                [chg] "=r"(change)
                : /* inputs */
                [cbuf]"r"(_gray_info.cur_buffer),
                [bbuf]"r"(_gray_info.back_buffer)
                : /* clobbers */
                 "r1", "r2"
            );

            if (change != 0)
            {
                unsigned char *cbuf, *bbuf, *addr, *end;
                unsigned mask;

                pat_ptr = &pat_stack[8];
                cbuf = _gray_info.cur_buffer;
                bbuf = _gray_info.back_buffer;

                /* precalculate the bit patterns with random shifts
                 * for all 8 pixels and put them on an extra "stack" */
                asm volatile (
                    "add     %[ofs],%[cbuf]  \n"
                    "add     %[ofs],%[bbuf]  \n"
                    "mov     #8,r3       \n"  /* loop count in r3: 8 pixels */

                ".ur_pre_loop:           \n"
                    "mov.b   @%[cbuf]+,r0\n"  /* read current buffer */
                    "mov.b   @%[bbuf],r1 \n"  /* read back buffer */
                    "mov     #0,r2       \n"  /* preset for skipped pixel */
                    "mov.b   r0,@%[bbuf] \n"  /* update back buffer */
                    "add     #1,%[bbuf]  \n"
                    "cmp/eq  r0,r1       \n"  /* no change? */
                    "bt      .ur_skip    \n"  /* -> skip */

                    "shll2   r0          \n"  /* pixel value -> pattern offset */
                    "mov.l   @(r0,%[bpat]),r4\n"  /* r4 = bitpattern[byte]; */

                    "mov     #75,r0      \n"
                    "mulu    r0,%[rnd]   \n"  /* multiply by 75 */
                    "sts     macl,%[rnd] \n"
                    "add     #74,%[rnd]  \n"  /* add another 74 */
                    /* Since the lower bits are not very random: */
                    "swap.b  %[rnd],r1   \n"  /* get bits 8..15 (need max. 5) */
                    "and     %[rmsk],r1  \n"  /* mask out unneeded bits */

                    "cmp/hs  %[dpth],r1  \n"  /* random >= depth ? */
                    "bf      .ur_ntrim   \n"
                    "sub     %[dpth],r1  \n"  /* yes: random -= depth; */
                ".ur_ntrim:              \n"
                
                    "mov.l   .ashlsi3,r0 \n"  /** rotate pattern **/
                    "jsr     @r0         \n"  /* r4 -> r0, shift left by r5 */
                    "mov     r1,r5       \n"

                    "mov     %[dpth],r5  \n"
                    "sub     r1,r5       \n"  /* r5 = depth - r1 */
                    "mov.l   .lshrsi3,r1 \n"
                    "jsr     @r1         \n"  /* r4 -> r0, shift right by r5 */
                    "mov     r0,r2       \n"  /* store previous result in r2 */
                                         
                    "or      r0,r2       \n"  /* rotated_pattern = r2 | r0 */
                    "clrt                \n"  /* mask bit = 0 (replace) */

                ".ur_skip:               \n"  /* T == 1 if skipped */
                    "rotcr   %[mask]     \n"  /* get mask bit */
                    "mov.l   r2,@-%[patp]\n"  /* push on pattern stack */

                    "add     #-1,r3      \n"  /* decrease loop count */
                    "cmp/pl  r3          \n"  /* loop count > 0? */
                    "bt      .ur_pre_loop\n"  /* yes: loop */
                    "shlr8   %[mask]     \n"
                    "shlr16  %[mask]     \n"
                    : /* outputs */
                    [cbuf]"+r"(cbuf),
                    [bbuf]"+r"(bbuf),
                    [rnd] "+r"(_gray_random_buffer),
                    [patp]"+r"(pat_ptr),
                    [mask]"=&r"(mask)
                    : /* inputs */
                    [ofs] "[mask]"(srcofs_row),
                    [dpth]"r"(_gray_info.depth),
                    [bpat]"r"(_gray_info.bitpattern),
                    [rmsk]"r"(_gray_info.randmask)
                    : /* clobbers */
                    "r0", "r1", "r2", "r3", "r4", "r5", "macl", "pr"
                );

                addr = dst_row;
                end = addr + MULU16(_gray_info.depth, _gray_info.plane_size);

                /* set the bits for all 8 pixels in all bytes according to the
                 * precalculated patterns on the pattern stack */
                asm volatile (
                    "mov.l   @%[patp]+,r1\n"  /* pop all 8 patterns */
                    "mov.l   @%[patp]+,r2\n"
                    "mov.l   @%[patp]+,r3\n"
                    "mov.l   @%[patp]+,r6\n"
                    "mov.l   @%[patp]+,r7\n"
                    "mov.l   @%[patp]+,r8\n"
                    "mov.l   @%[patp]+,r9\n"
                    "mov.l   @%[patp]+,r10   \n"

                    "tst     %[mask],%[mask] \n"  /* nothing to keep? */
                    "bt      .ur_sloop   \n"  /* yes: jump to short loop */

                ".ur_floop:              \n"  /** full loop (there are bits to keep)**/
                    "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
                    "rotcl   r0          \n"  /* rotate t bit into r0 */
                    "shlr    r2          \n"
                    "rotcl   r0          \n"
                    "shlr    r3          \n"
                    "rotcl   r0          \n"
                    "shlr    r6          \n"
                    "rotcl   r0          \n"
                    "shlr    r7          \n"
                    "rotcl   r0          \n"
                    "shlr    r8          \n"
                    "rotcl   r0          \n"
                    "shlr    r9          \n"
                    "rotcl   r0          \n"
                    "shlr    r10         \n"
                    "mov.b   @%[addr],%[patp]\n"  /* read old value */
                    "rotcl   r0          \n"
                    "and     %[mask],%[patp] \n"  /* mask out unneeded bits */
                    "or      %[patp],r0  \n"  /* set new bits */
                    "mov.b   r0,@%[addr] \n"  /* store value to bitplane */
                    "add     %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp/hi  %[addr],%[end]  \n"  /* last bitplane done? */
                    "bt      .ur_floop   \n"  /* no: loop */

                    "bra     .ur_end     \n"
                    "nop                 \n"

                ".ur_sloop:              \n"  /** short loop (nothing to keep) **/
                    "shlr    r1          \n"  /* rotate lsb of pattern 1 to t bit */
                    "rotcl   r0          \n"  /* rotate t bit into r0 */
                    "shlr    r2          \n"
                    "rotcl   r0          \n"
                    "shlr    r3          \n"
                    "rotcl   r0          \n"
                    "shlr    r6          \n"
                    "rotcl   r0          \n"
                    "shlr    r7          \n"
                    "rotcl   r0          \n"
                    "shlr    r8          \n"
                    "rotcl   r0          \n"
                    "shlr    r9          \n"
                    "rotcl   r0          \n"
                    "shlr    r10         \n"
                    "rotcl   r0          \n"
                    "mov.b   r0,@%[addr]     \n"  /* store byte to bitplane */
                    "add     %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp/hi  %[addr],%[end]  \n"  /* last bitplane done? */
                    "bt      .ur_sloop   \n"  /* no: loop */

                ".ur_end:                \n"
                    : /* outputs */
                    [patp]"+r"(pat_ptr),
                    [addr]"+r"(addr),
                    [mask]"+r"(mask)
                    : /* inputs */
                    [end] "r"(end),
                    [psiz]"r"(_gray_info.plane_size)
                    : /* clobbers */
                    "r0", "r1", "r2", "r3", "r6", "r7", "r8", "r9", "r10"
                );
            }
#elif defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
            unsigned long pat_stack[4];
            unsigned long *pat_ptr;
            unsigned change;

            asm volatile (
                "move.l  (%[ofs]:l:1,%[cbuf]),%[chg] \n"
                "sub.l   (%[ofs]:l:1,%[bbuf]),%[chg] \n"
                : /* outputs */
                [chg] "=&d"(change)
                : /* inputs */
                [cbuf]"a"(_gray_info.cur_buffer),
                [bbuf]"a"(_gray_info.back_buffer),
                [ofs] "d"(srcofs_row)
            );

            if (change != 0)
            {
                unsigned char *cbuf, *bbuf, *addr, *end;
                unsigned mask;

                pat_ptr = &pat_stack[4];
                cbuf = _gray_info.cur_buffer;
                bbuf = _gray_info.back_buffer;

                /* precalculate the bit patterns with random shifts
                 * for all 4 pixels and put them on an extra "stack" */
                asm volatile (
                    "add.l   %[ofs],%[cbuf]  \n"
                    "add.l   %[ofs],%[bbuf]  \n"
                    "moveq.l #4,%%d3     \n"  /* loop count in d3: 4 pixels */
                    "clr.l   %[mask]     \n"

                ".ur_pre_loop:           \n"
                    "clr.l   %%d0        \n"
                    "move.b  (%[cbuf])+,%%d0  \n"  /* read current buffer */
                    "clr.l   %%d1        \n"
                    "move.b  (%[bbuf]),%%d1   \n"  /* read back buffer */
                    "move.b  %%d0,(%[bbuf])+  \n"  /* update back buffer */
                    "clr.l   %%d2        \n"  /* preset for skipped pixel */
                    "cmp.l   %%d0,%%d1   \n"  /* no change? */
                    "beq.b   .ur_skip    \n"  /* -> skip */

                    "move.l  (%%d0:l:4,%[bpat]),%%d2  \n"  /* d2 = bitpattern[byte]; */

                    "mulu.w  #75,%[rnd]  \n"  /* multiply by 75 */
                    "add.l   #74,%[rnd]  \n"  /* add another 74 */
                    /* Since the lower bits are not very random: */
                    "move.l  %[rnd],%%d1 \n"
                    "lsr.l   #8,%%d1     \n"  /* get bits 8..15 (need max. 5) */
                    "and.l   %[rmsk],%%d1\n"  /* mask out unneeded bits */

                    "cmp.l   %[dpth],%%d1\n"  /* random >= depth ? */
                    "blo.b   .ur_ntrim   \n"
                    "sub.l   %[dpth],%%d1\n"  /* yes: random -= depth; */
                ".ur_ntrim:              \n"

                    "move.l  %%d2,%%d0   \n"
                    "lsl.l   %%d1,%%d0   \n"
                    "sub.l   %[dpth],%%d1\n"
                    "neg.l   %%d1        \n"  /* d1 = depth - d1 */
                    "lsr.l   %%d1,%%d2   \n"
                    "or.l    %%d0,%%d2   \n"  /* rotated_pattern = d2 | d0 */

                    "or.l    #0x0300,%[mask] \n"  /* set mask bit */

                ".ur_skip:               \n"
                    "lsr.l   #2,%[mask]  \n"  /* shift mask */
                    "move.l  %%d2,-(%[patp]) \n"  /* push on pattern stack */

                    "subq.l  #1,%%d3     \n"  /* decrease loop count */
                    "bne.b   .ur_pre_loop\n"  /* yes: loop */
                    : /* outputs */
                    [cbuf]"+a"(cbuf),
                    [bbuf]"+a"(bbuf),
                    [patp]"+a"(pat_ptr),
                    [rnd] "+d"(_gray_random_buffer),
                    [mask]"=&d"(mask)
                    : /* inputs */
                    [ofs] "[mask]"(srcofs_row),
                    [bpat]"a"(_gray_info.bitpattern),
                    [dpth]"d"(_gray_info.depth),
                    [rmsk]"d"(_gray_info.randmask)
                    : /* clobbers */
                    "d0", "d1", "d2", "d3"
                );

                addr = dst_row;
                end = addr + MULU16(_gray_info.depth, _gray_info.plane_size);

                /* set the bits for all 4 pixels in all bytes according to the
                 * precalculated patterns on the pattern stack */
                asm volatile (
                    "movem.l (%[patp]),%%d2-%%d5 \n" /* pop all 4 patterns */

                    "not.l   %[mask]     \n"  /* set mask -> keep mask */
                    "and.l   #0xFF,%[mask]   \n"
                    "beq.b   .ur_sloop   \n"  /* yes: jump to short loop */

                ".ur_floop:              \n"  /** full loop (there are bits to keep)**/
                    "clr.l   %%d0        \n"
                    "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
                    "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
                    "lsl.l   #1,%%d0     \n"  /* shift by another 1 for a total of 2 */
                    "lsr.l   #1,%%d3     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsl.l   #1,%%d0     \n"
                    "lsr.l   #1,%%d4     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsl.l   #1,%%d0     \n"
                    "lsr.l   #1,%%d5     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d0,%%d1   \n"  /* duplicate bits 0, 2, 4, 6, ... */
                    "lsl.l   #1,%%d1     \n"  /* to 1, 3, 5, 7, ... */
                    "or.l    %%d1,%%d0   \n"

                    "move.b  (%[addr]),%%d1  \n"  /* read old value */
                    "and.l   %[mask],%%d1    \n"  /* mask out unneeded bits */
                    "or.l    %%d0,%%d1       \n"  /* set new bits */
                    "move.b  %%d1,(%[addr])  \n"  /* store value to bitplane */

                    "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
                    "bhi.b   .ur_floop   \n"  /* no: loop */

                    "bra.b   .ur_end     \n"

                ".ur_sloop:              \n"  /** short loop (nothing to keep) **/
                    "clr.l   %%d0        \n"
                    "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
                    "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
                    "lsl.l   #1,%%d0     \n"  /* shift by another 1 for a total of 2 */
                    "lsr.l   #1,%%d3     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsl.l   #1,%%d0     \n"
                    "lsr.l   #1,%%d4     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsl.l   #1,%%d0     \n"
                    "lsr.l   #1,%%d5     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d0,%%d1   \n"  /* duplicate bits 0, 2, 4, 6, ... */
                    "lsl.l   #1,%%d1     \n"  /* to 1, 3, 5, 7, ... */
                    "or.l    %%d1,%%d0   \n"

                    "move.b  %%d0,(%[addr])  \n"  /* store byte to bitplane */
                    "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
                    "bhi.b   .ur_sloop   \n"  /* no: loop */

                ".ur_end:                \n"
                    : /* outputs */
                    [addr]"+a"(addr),
                    [mask]"+d"(mask)
                    : /* inputs */
                    [psiz]"r"(_gray_info.plane_size),
                    [end] "a"(end),
                    [patp]"a"(pat_ptr)
                    : /* clobbers */
                    "d0", "d1", "d2", "d3", "d4", "d5"
                );
            }
#endif
            srcofs_row += _gray_info.height;
            dst_row++;
        }
        while (dst_row < dst_end);
        
        srcofs += _PBLOCK;
        dst += _gray_info.width;
    }
}

#if CONFIG_CPU == SH7034
/* References to C library routines used in gray_update_rect() */
asm (
    ".align  2           \n"
".ashlsi3:               \n"  /* C library routine: */
    ".long   ___ashlsi3  \n"  /* shift r4 left by r5, return in r0 */
".lshrsi3:               \n"  /* C library routine: */
    ".long   ___lshrsi3  \n"  /* shift r4 right by r5, return in r0 */
    /* both routines preserve r4, destroy r5 and take ~16 cycles */
);
#endif

/* Update the whole greyscale overlay */
void gray_update(void)
{
    gray_update_rect(0, 0, _gray_info.width, _gray_info.height);
}

/* Do an lcd_update() to show changes done by rb->lcd_xxx() functions
   (in areas of the screen not covered by the greyscale overlay). */
void gray_deferred_lcd_update(void)
{
    if (_gray_info.flags & _GRAY_RUNNING)
        _gray_info.flags |= _GRAY_DEFERRED_UPDATE;
    else
        _gray_rb->lcd_update();
}

/*** Screenshot ***/

#define BMP_NUMCOLORS  33
#define BMP_BPP        8
#define BMP_LINESIZE   ((LCD_WIDTH + 3) & ~3)
#define BMP_HEADERSIZE (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE   (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE  (BMP_HEADERSIZE + BMP_DATASIZE)

#define LE16_CONST(x) (x)&0xff, ((x)>>8)&0xff
#define LE32_CONST(x) (x)&0xff, ((x)>>8)&0xff, ((x)>>16)&0xff, ((x)>>24)&0xff

static const unsigned char bmpheader[] =
{
    0x42, 0x4d,                 /* 'BM' */
    LE32_CONST(BMP_TOTALSIZE),  /* Total file size */
    0x00, 0x00, 0x00, 0x00,     /* Reserved */
    LE32_CONST(BMP_HEADERSIZE), /* Offset to start of pixel data */

    0x28, 0x00, 0x00, 0x00,     /* Size of (2nd) header */
    LE32_CONST(LCD_WIDTH),      /* Width in pixels */
    LE32_CONST(LCD_HEIGHT),     /* Height in pixels */
    0x01, 0x00,                 /* Number of planes (always 1) */
    LE16_CONST(BMP_BPP),        /* Bits per pixel 1/4/8/16/24 */
    0x00, 0x00, 0x00, 0x00,     /* Compression mode, 0 = none */
    LE32_CONST(BMP_DATASIZE),   /* Size of bitmap data */
    0xc4, 0x0e, 0x00, 0x00,     /* Horizontal resolution (pixels/meter) */
    0xc4, 0x0e, 0x00, 0x00,     /* Vertical resolution (pixels/meter) */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of used colours */
    LE32_CONST(BMP_NUMCOLORS),  /* Number of important colours */
};

#if LCD_DEPTH == 1
#define BMP_RED   0x90
#define BMP_GREEN 0xee
#define BMP_BLUE  0x90
#elif LCD_DEPTH == 2
#define BMP_RED   0xad
#define BMP_GREEN 0xd8
#define BMP_BLUE  0xe6
#endif

static unsigned char linebuf[BMP_LINESIZE];

/* Save the current display content (b&w and greyscale overlay) to an 8-bit
 * BMP file in the root directory. */
void gray_screendump(void)
{
    int fh, i, bright;
    int y;
#if LCD_DEPTH == 1
    int x, by, mask;
    int gx, gby;
    unsigned char *lcdptr, *grayptr, *grayptr2;
#endif
    char filename[MAX_PATH];

#ifdef HAVE_RTC
    struct tm *tm = _gray_rb->get_time();

    _gray_rb->snprintf(filename, MAX_PATH,
                       "/graydump %04d-%02d-%02d %02d-%02d-%02d.bmp",
                       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
#else
    {
        DIR* dir;
        int max_dump_file = 1; /* default to graydump_0001.bmp */
        dir = _gray_rb->opendir("/");
        if (dir) /* found */
        {
            /* Search for the highest screendump filename present,
               increment behind that. So even with "holes"
               (deleted files), the newest will always have the
               highest number. */
            while(true)
            {   
                struct dirent* entry;
                int curr_dump_file;
                /* walk through the directory content */
                entry = _gray_rb->readdir(dir);
                if (!entry)
                {
                    _gray_rb->closedir(dir);
                    break; /* end of dir */
                }
                if (_gray_rb->strncasecmp(entry->d_name, "graydump_", 9))
                    continue; /* no screendump file */
                curr_dump_file = _gray_rb->atoi(&entry->d_name[9]);
                if (curr_dump_file >= max_dump_file)
                    max_dump_file = curr_dump_file + 1;
            }
        }
        _gray_rb->snprintf(filename, MAX_PATH, "/graydump_%04d.bmp",
                           max_dump_file);
    }
#endif

    fh = _gray_rb->creat(filename, O_WRONLY);

    if (fh < 0)
        return;
        
    _gray_rb->write(fh, bmpheader, sizeof(bmpheader));  /* write header */

    /* build clut */
    linebuf[3] = 0;

    for (i = 0; i < BMP_NUMCOLORS; i++)
    {
        bright = MIN(i, _gray_info.depth);
        linebuf[0] = MULU16(BMP_BLUE,  bright) / _gray_info.depth;
        linebuf[1] = MULU16(BMP_GREEN, bright) / _gray_info.depth;
        linebuf[2] = MULU16(BMP_RED,   bright) / _gray_info.depth;
        _gray_rb->write(fh, linebuf, 4);
    }

    /* 8-bit BMP image goes bottom -> top */
    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
        _gray_rb->memset(linebuf, BMP_NUMCOLORS-1, LCD_WIDTH);

#if LCD_DEPTH == 1
        mask = 1 << (y & 7);
        by = y >> 3;
        lcdptr = _gray_rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);
        gby = by - _gray_info.by;

        if ((_gray_info.flags & _GRAY_RUNNING)
            && (unsigned) gby < (unsigned) _gray_info.bheight)
        {   
            /* line contains greyscale (and maybe b&w) graphics */
            grayptr = _gray_info.plane_data + MULU16(_gray_info.width, gby);

            for (x = 0; x < LCD_WIDTH; x++)
            {
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
            
                gx = x - _gray_info.x;
                
                if ((unsigned)gx < (unsigned)_gray_info.width)
                {
                    bright = 0;
                    grayptr2 = grayptr + gx;

                    for (i = 0; i < _gray_info.depth; i++)
                    {
                        if (!(*grayptr2 & mask))
                            bright++;
                        grayptr2 += _gray_info.plane_size;
                    }
                    linebuf[x] = bright;
                }
            }
        }
        else  
        {   
            /* line contains only b&w graphics */
            for (x = 0; x < LCD_WIDTH; x++)
                if (*lcdptr++ & mask)
                    linebuf[x] = 0;
        }
#elif LCD_DEPTH == 2
        /* TODO */
#endif

        _gray_rb->write(fh, linebuf, sizeof(linebuf));
    }

    _gray_rb->close(fh);
}

#endif /* HAVE_LCD_BITMAP */
#endif /* !SIMULATOR */


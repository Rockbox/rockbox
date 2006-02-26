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
* This is a generic framework to display up to 33 shades of grey
* on low-depth bitmap LCDs (Archos b&w, Iriver 4-grey) within plugins.
*
* Copyright (C) 2004-2006 Jens Arnold
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
#include "gray.h"

/* Global variables */
struct plugin_api *_gray_rb = NULL; /* global api struct pointer */
struct _gray_info _gray_info;       /* global info structure */
#ifndef SIMULATOR
short _gray_random_buffer;          /* buffer for random number generator */
#endif

/* Prototypes */
static inline void _deferred_update(void) __attribute__ ((always_inline));
#ifdef SIMULATOR
static unsigned long _gray_get_pixel(int x, int y);
#else
static void _timer_isr(void);
#endif
static void gray_screendump_hook(int fd);

/* Update LCD areas not covered by the greyscale overlay */
static inline void _deferred_update(void)
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
}

#ifndef SIMULATOR
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
        _deferred_update();
        _gray_info.flags &= ~_GRAY_DEFERRED_UPDATE; /* clear request */
    }
}
#endif /* !SIMULATOR */

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
   bheight   = height in LCD pixel-block units (8 pixels) (1..LCD_HEIGHT/8)
   depth     = number of bitplanes to use (1..32).

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
   shades = depth + 1

   If you need info about the memory taken by the greyscale buffer, supply a
   long* as the last parameter. This long will then contain the number of bytes
   used. The total memory needed can be calculated as follows:
 total_mem =
     shades * sizeof(long)               (bitpatterns)
   + (width * bheight) * depth           (bitplane data)
   + buffered ?                          (chunky front- & backbuffer)
       (width * bheight * 8 * 2) : 0
   + 0..3                                (longword alignment)
   
   The function tries to be as authentic as possible regarding memory usage on
   the simulator, even if it doesn't use all of the allocated memory. There's
   one situation where it will consume more memory on the sim than on the
   target: if you're allocating a low depth (< 8) without buffering. */
int gray_init(struct plugin_api* newrb, unsigned char *gbuf, long gbuf_size,
              bool buffered, int width, int bheight, int depth, long *buf_taken)
{
    int possible_depth;
    long plane_size, buftaken;
#ifndef SIMULATOR
    int i, j;
#endif

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

#ifdef SIMULATOR
    if (!buffered)
    {
        long orig_size = depth * plane_size + (depth + 1) * sizeof(long);
        
        plane_size = MULU16(width, bheight << _PBLOCK_EXP);
        if (plane_size > orig_size)
        {
            buftaken += plane_size;
            if (buftaken > gbuf_size)
                return 0;
        }
        else
        {
            buftaken += orig_size;
        }
        _gray_info.cur_buffer = gbuf;
    }
    else
#endif
        buftaken += depth * plane_size + (depth + 1) * sizeof(long);

    _gray_info.x = 0;
    _gray_info.by = 0;
    _gray_info.width = width;
    _gray_info.height = bheight << _PBLOCK_EXP;
    _gray_info.bheight = bheight;
    _gray_info.depth = depth;
    _gray_info.flags = 0;
#ifndef SIMULATOR
    _gray_info.cur_plane = 0;
    _gray_info.plane_size = plane_size;
    _gray_info.plane_data = gbuf;
    gbuf += depth * plane_size;
    _gray_info.bitpattern = (unsigned long *)gbuf;
              
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
#endif

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
    if (enable && !(_gray_info.flags & _GRAY_RUNNING))
    {
        _gray_info.flags |= _GRAY_RUNNING;
#ifdef SIMULATOR
        _gray_rb->sim_lcd_ex_init(_gray_info.depth + 1, _gray_get_pixel);
        gray_update();
#else /* !SIMULATOR */
#if CONFIG_LCD == LCD_SSD1815
        _gray_rb->timer_register(1, NULL, CPU_FREQ / 67, 1, _timer_isr);
#elif CONFIG_LCD == LCD_S1D15E06
        _gray_rb->timer_register(1, NULL, CPU_FREQ / 70, 1, _timer_isr);
#elif CONFIG_LCD == LCD_IFP7XX
        /* TODO: implement for iFP */
        (void)_timer_isr;
#endif /* CONFIG_LCD */
#endif /* !SIMULATOR */
        _gray_rb->screen_dump_set_hook(gray_screendump_hook);
    }
    else if (!enable && (_gray_info.flags & _GRAY_RUNNING))
    {
#ifdef SIMULATOR
        _gray_rb->sim_lcd_ex_init(0, NULL);
#else
        _gray_rb->timer_unregister();
#endif
        _gray_info.flags &= ~_GRAY_RUNNING;
        _gray_rb->screen_dump_set_hook(NULL);
        _gray_rb->lcd_update(); /* restore whatever there was before */
    }
}

#ifdef SIMULATOR
/* Callback function for gray_update_rect() to read a pixel from the graybuffer.
   Note that x and y are in LCD coordinates, not graybuffer coordinates! */
static unsigned long _gray_get_pixel(int x, int y)
{
    return _gray_info.cur_buffer[MULU16(x - _gray_info.x, _gray_info.height)
                                 + y - (_gray_info.by << _PBLOCK_EXP)]
           + (1 << LCD_DEPTH);
}

/* Update a rectangular area of the greyscale overlay */
void gray_update_rect(int x, int y, int width, int height)
{
    if (x + width > _gray_info.width)
        width = _gray_info.width - x;
    if (y + height > _gray_info.height)
        height = _gray_info.height - y;
        
    x += _gray_info.x;
    y += _gray_info.by << _PBLOCK_EXP;

    if (x + width > LCD_WIDTH)
        width = LCD_WIDTH - x;
    if (y + height > LCD_HEIGHT)
        height = LCD_HEIGHT - y;
        
    _gray_rb->sim_lcd_ex_update_rect(x, y, width, height);
}

#else /* !SIMULATOR */
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
            unsigned char *cbuf, *bbuf;
            unsigned change;

            cbuf = _gray_info.cur_buffer + srcofs_row;
            bbuf = _gray_info.back_buffer + srcofs_row;

            asm volatile (
                "mov.l   @%[cbuf]+,r1    \n"
                "mov.l   @%[bbuf]+,r2    \n"
                "xor     r1,r2           \n"
                "mov.l   @%[cbuf],r1     \n"
                "mov.l   @%[bbuf],%[chg] \n"
                "xor     r1,%[chg]       \n"
                "or      r2,%[chg]       \n"
                : /* outputs */
                [cbuf]"+r"(cbuf),
                [bbuf]"+r"(bbuf),
                [chg] "=r"(change)
                : /* inputs */
                : /* clobbers */
                 "r1", "r2"
            );

            if (change != 0)
            {
                unsigned char *addr, *end;
                unsigned mask, trash;

                pat_ptr = &pat_stack[8];
                cbuf = _gray_info.cur_buffer + srcofs_row;
                bbuf = _gray_info.back_buffer + srcofs_row;

                /* precalculate the bit patterns with random shifts
                 * for all 8 pixels and put them on an extra "stack" */
                asm volatile (
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
                    "mov.l   @%[patp],r10\n"

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
                    "mov.b   @%[addr],%[rx]  \n"  /* read old value */
                    "rotcl   r0          \n"
                    "and     %[mask],%[rx]   \n"  /* mask out unneeded bits */
                    "or      %[rx],r0    \n"  /* set new bits */
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
                    [addr]"+r"(addr),
                    [mask]"+r"(mask),
                    [rx]  "=&r"(trash)
                    : /* inputs */
                    [psiz]"r"(_gray_info.plane_size),
                    [end] "r"(end),
                    [patp]"[rx]"(pat_ptr)
                    : /* clobbers */
                    "r0", "r1", "r2", "r3", "r6", "r7", "r8", "r9", "r10"
                );
            }
#elif defined(CPU_COLDFIRE) && (LCD_DEPTH == 2)
            unsigned long pat_stack[8];
            unsigned long *pat_ptr;
            unsigned char *cbuf, *bbuf;
            unsigned change;

            cbuf = _gray_info.cur_buffer + srcofs_row;
            bbuf = _gray_info.back_buffer + srcofs_row;

            asm volatile (
                "move.l  (%[cbuf])+,%%d0 \n"
                "move.l  (%[bbuf])+,%%d1 \n"
                "eor.l   %%d0,%%d1       \n"
                "move.l  (%[cbuf]),%%d0  \n"
                "move.l  (%[bbuf]),%[chg]\n"
                "eor.l   %%d0,%[chg]     \n"
                "or.l    %%d1,%[chg]     \n"
                : /* outputs */
                [cbuf]"+a"(cbuf),
                [bbuf]"+a"(bbuf),
                [chg] "=&d"(change)
                : /* inputs */
                : /* clobbers */
                "d0", "d1"
            );

            if (change != 0)
            {
                unsigned char *addr, *end;
                unsigned mask, trash;

                pat_ptr = &pat_stack[8];
                cbuf = _gray_info.cur_buffer + srcofs_row;
                bbuf = _gray_info.back_buffer + srcofs_row;

                /* precalculate the bit patterns with random shifts
                 * for all 8 pixels and put them on an extra "stack" */
                asm volatile (
                    "moveq.l #8,%%d3     \n"  /* loop count in d3: 8 pixels */
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

                    "or.l    #0x0100,%[mask] \n"  /* set mask bit */

                ".ur_skip:               \n"
                    "lsr.l   #1,%[mask]  \n"  /* shift mask */
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
                    [bpat]"a"(_gray_info.bitpattern),
                    [dpth]"d"(_gray_info.depth),
                    [rmsk]"d"(_gray_info.randmask)
                    : /* clobbers */
                    "d0", "d1", "d2", "d3"
                );

                addr = dst_row;
                end = addr + MULU16(_gray_info.depth, _gray_info.plane_size);

                /* set the bits for all 8 pixels in all bytes according to the
                 * precalculated patterns on the pattern stack */
                asm volatile (
                    "movem.l (%[patp]),%%d2-%%d6/%%a0-%%a1/%[ax] \n" 
                                              /* pop all 8 patterns */
                    "not.l   %[mask]     \n"  /* set mask -> keep mask */
                    "and.l   #0xFF,%[mask]   \n"
                    "beq.b   .ur_sstart  \n"  /* yes: jump to short loop */

                ".ur_floop:              \n"  /** full loop (there are bits to keep)**/
                    "clr.l   %%d0        \n"
                    "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
                    "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
                    "lsr.l   #1,%%d3     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d4     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d5     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d6     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%a0,%%d1   \n"
                    "lsr.l   #1,%%d1     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d1,%%a0   \n"
                    "move.l  %%a1,%%d1   \n"
                    "lsr.l   #1,%%d1     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d1,%%a1   \n"
                    "move.l  %[ax],%%d1  \n"
                    "lsr.l   #1,%%d1     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d1,%[ax]  \n"

                    "move.b  (%[addr]),%%d1  \n"  /* read old value */
                    "and.l   %[mask],%%d1    \n"  /* mask out unneeded bits */
                    "or.l    %%d0,%%d1       \n"  /* set new bits */
                    "move.b  %%d1,(%[addr])  \n"  /* store value to bitplane */

                    "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
                    "bhi.b   .ur_floop   \n"  /* no: loop */

                    "bra.b   .ur_end     \n"
                    
                ".ur_sstart:             \n"
                    "move.l  %%a0,%[mask]\n"  /* mask isn't needed here, reuse reg */

                ".ur_sloop:              \n"  /** short loop (nothing to keep) **/
                    "clr.l   %%d0        \n"
                    "lsr.l   #1,%%d2     \n"  /* shift out mask bit */
                    "addx.l  %%d0,%%d0   \n"  /* puts bit into LSB, shifts left by 1 */
                    "lsr.l   #1,%%d3     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d4     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d5     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%%d6     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "lsr.l   #1,%[mask]  \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%a1,%%d1   \n"
                    "lsr.l   #1,%%d1     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d1,%%a1   \n"
                    "move.l  %[ax],%%d1  \n"
                    "lsr.l   #1,%%d1     \n"
                    "addx.l  %%d0,%%d0   \n"
                    "move.l  %%d1,%[ax]  \n"

                    "move.b  %%d0,(%[addr])  \n"  /* store byte to bitplane */
                    "add.l   %[psiz],%[addr] \n"  /* advance to next bitplane */
                    "cmp.l   %[addr],%[end]  \n"  /* last bitplane done? */
                    "bhi.b   .ur_sloop   \n"  /* no: loop */

                ".ur_end:                \n"
                    : /* outputs */
                    [addr]"+a"(addr),
                    [mask]"+d"(mask),
                    [ax]  "=&a"(trash)
                    : /* inputs */
                    [psiz]"a"(_gray_info.plane_size),
                    [end] "a"(end),
                    [patp]"[ax]"(pat_ptr)
                    : /* clobbers */
                    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "a0", "a1"
                );
            }
#endif /* CONFIG_CPU, LCD_DEPTH */
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
#endif /* CONFIG_CPU == SH7034 */

#endif /* !SIMULATOR */

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
    {
#ifdef SIMULATOR
        _deferred_update();
#else
        _gray_info.flags |= _GRAY_DEFERRED_UPDATE;
#endif
    }
    else
        _gray_rb->lcd_update();
}

/*** Screenshot ***/

#define BMP_FIXEDCOLORS (1 << LCD_DEPTH)
#define BMP_VARCOLORS   33
#define BMP_NUMCOLORS   (BMP_FIXEDCOLORS + BMP_VARCOLORS)
#define BMP_BPP         8
#define BMP_LINESIZE    ((LCD_WIDTH + 3) & ~3)
#define BMP_HEADERSIZE  (54 + 4 * BMP_NUMCOLORS)
#define BMP_DATASIZE    (BMP_LINESIZE * LCD_HEIGHT)
#define BMP_TOTALSIZE   (BMP_HEADERSIZE + BMP_DATASIZE)

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

    /* Fixed colours */
#if LCD_DEPTH == 1
    0x90, 0xee, 0x90, 0x00,     /* Colour #0 */
    0x00, 0x00, 0x00, 0x00      /* Colour #1 */
#elif LCD_DEPTH == 2
    0xe6, 0xd8, 0xad, 0x00,     /* Colour #0 */
    0x99, 0x90, 0x73, 0x00,     /* Colour #1 */
    0x4c, 0x48, 0x39, 0x00,     /* Colour #2 */
    0x00, 0x00, 0x00, 0x00      /* Colour #3 */
#endif
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

/* Hook function for core screen_dump() to save the current display
   content (b&w and greyscale overlay) to an 8-bit BMP file. */
static void gray_screendump_hook(int fd)
{
    int i;
    int x, y, by;
    int gx, gy, mask;
#if LCD_DEPTH == 2
    int shift;
#endif
    unsigned char *clut_entry;
    unsigned char *lcdptr;
    unsigned char linebuf[MAX(4*BMP_VARCOLORS,BMP_LINESIZE)];

    _gray_rb->write(fd, bmpheader, sizeof(bmpheader));  /* write header */

    /* build clut */
    _gray_rb->memset(linebuf, 0, 4*BMP_VARCOLORS);
    clut_entry = linebuf;

    for (i = _gray_info.depth; i > 0; i--)
    {
        *clut_entry++ = MULU16(BMP_BLUE,  i) / _gray_info.depth;
        *clut_entry++ = MULU16(BMP_GREEN, i) / _gray_info.depth;
        *clut_entry++ = MULU16(BMP_RED,   i) / _gray_info.depth;
        clut_entry++;
    }
    _gray_rb->write(fd, linebuf, 4*BMP_VARCOLORS);

    /* BMP image goes bottom -> top */
    for (y = LCD_HEIGHT - 1; y >= 0; y--)
    {
        _gray_rb->memset(linebuf, 0, BMP_LINESIZE);

        gy = y - (_gray_info.by << _PBLOCK_EXP);
#if LCD_DEPTH == 1
        mask = 1 << (y & (_PBLOCK-1));
        by = y >> _PBLOCK_EXP;
        lcdptr = _gray_rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);

        if ((unsigned) gy < (unsigned) _gray_info.height)
        {
            /* line contains greyscale (and maybe b&w) graphics */
#ifndef SIMULATOR
            unsigned char *grayptr = _gray_info.plane_data 
                                   + MULU16(_gray_info.width, gy >> _PBLOCK_EXP);
#endif

            for (x = 0; x < LCD_WIDTH; x++)
            {
                gx = x - _gray_info.x;
                
                if ((unsigned)gx < (unsigned)_gray_info.width)
                {
#ifdef SIMULATOR
                    linebuf[x] = BMP_FIXEDCOLORS + _gray_info.depth
                               - _gray_info.cur_buffer[MULU16(gx, _gray_info.height) + gy];
#else
                    int idx = BMP_FIXEDCOLORS;
                    unsigned char *grayptr2 = grayptr + gx;

                    for (i = _gray_info.depth; i > 0; i--)
                    {
                        if (*grayptr2 & mask)
                            idx++;
                        grayptr2 += _gray_info.plane_size;
                    }
                    linebuf[x] = idx;
#endif
                }
                else
                {
                    linebuf[x] = (*lcdptr & mask) ? 1 : 0;
                }
                lcdptr++;
            }
        }
        else
        {
            /* line contains only b&w graphics */
            for (x = 0; x < LCD_WIDTH; x++)
                linebuf[x] = (*lcdptr++ & mask) ? 1 : 0;
        }
#elif LCD_DEPTH == 2
        shift = 2 * (y & 3);
        by = y >> 2;
        lcdptr = _gray_rb->lcd_framebuffer + MULU16(LCD_WIDTH, by);

        if ((unsigned)gy < (unsigned)_gray_info.height)
        {
            /* line contains greyscale (and maybe b&w) graphics */
#ifdef SIMULATOR
            (void)mask;
#else
            unsigned char *grayptr = _gray_info.plane_data
                                   + MULU16(_gray_info.width, gy >> _PBLOCK_EXP);
            mask = 1 << (gy & (_PBLOCK-1));
#endif

            for (x = 0; x < LCD_WIDTH; x++)
            {
                gx = x - _gray_info.x;
                
                if ((unsigned)gx < (unsigned)_gray_info.width)
                {
#ifdef SIMULATOR
                    linebuf[x] = BMP_FIXEDCOLORS + _gray_info.depth
                               - _gray_info.cur_buffer[MULU16(gx, _gray_info.height) + gy];
#else
                    int idx = BMP_FIXEDCOLORS;
                    unsigned char *grayptr2 = grayptr + gx;

                    for (i = _gray_info.depth; i > 0; i--)
                    {
                        if (*grayptr2 & mask)
                            idx++;
                        grayptr2 += _gray_info.plane_size;
                    }
                    linebuf[x] = idx;
#endif
                }
                else
                {
                    linebuf[x] = (*lcdptr >> shift) & 3;
                }
                lcdptr++;
            }
        }
        else
        {
            /* line contains only b&w graphics */
            for (x = 0; x < LCD_WIDTH; x++)
                linebuf[x] = (*lcdptr++ >> shift) & 3;
        }
#endif /* LCD_DEPTH */

        _gray_rb->write(fd, linebuf, BMP_LINESIZE);
    }
}

#endif /* HAVE_LCD_BITMAP */


/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* JPEG image viewer
* (This is a real mess if it has to be coded in one single C file)
*
* Copyright (C) 2004 Jörg Hohensohn aka [IDC]Dragon
* Grayscale framework (c) 2004 Jens Arnold
* Heavily borrowed from the IJG implementation (c) Thomas G. Lane
* Small & fast downscaling IDCT (c) 2002 by Guido Vollbeding  JPEGclub.org
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

/******************************* Globals ***********************************/

static struct plugin_api* rb;

/*********************** Begin grayscale framework *************************/

/* This is a generic framework to use grayscale display within rockbox
 * plugins. It obviously does not work for the player.
 *
 * If you want to use grayscale display within a plugin, copy this section
 * (up to "End grayscale framework") into your source and you are able to use
 * it. For detailed documentation look at the head of each public function.
 *
 * It requires a global Rockbox api pointer in "rb" and uses the rockbox
 * timer api so you cannot use that timer for other  purposes while
 * displaying grayscale.
 *
 * The framework consists of 3 sections:
 *
 * - internal core functions and definitions
 * - public core functions
 * - public optional functions
 *
 * Usually you will use functions from the latter two sections in your code.
 * You can cut out functions from the third section that you do not need in
 * order to not waste space. Don't forget to cut the prototype as well.
 */

/**** internal core functions and definitions ****/

/* You do not want to touch these if you don't know exactly what you're
 * doing. */

#define GRAY_RUNNING          0x0001  /* grayscale overlay is running */
#define GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) (((unsigned short) (a)) * ((unsigned short) (b)))

typedef struct
{
    int x;
    int by;        /* 8-pixel units */
    int width;
    int height;
    int bheight;   /* 8-pixel units */
    int plane_size;
    int depth;     /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane; /* for the timer isr */
    unsigned long randmask; /* mask for random value in graypixel() */
    unsigned long flags;    /* various flags, see #defines */
    unsigned char *data;    /* pointer to start of bitplane data */
    unsigned long *bitpattern; /* pointer to start of pattern table */
} tGraybuf;

static tGraybuf *graybuf = NULL;
static short gray_random_buffer;

/** prototypes **/

void gray_timer_isr(void);
void graypixel(int x, int y, unsigned long pattern);
void grayblock(int x, int by, unsigned char* src, int stride);
void grayinvertmasked(int x, int by, unsigned char mask);

/** implementation **/

/* timer interrupt handler: display next bitplane */
void gray_timer_isr(void)
{
    rb->lcd_blit(graybuf->data + MULU16(graybuf->plane_size, graybuf->cur_plane),
                 graybuf->x, graybuf->by, graybuf->width, graybuf->bheight,
                 graybuf->width);

    if (++graybuf->cur_plane >= graybuf->depth)
        graybuf->cur_plane = 0;

    if (graybuf->flags & GRAY_DEFERRED_UPDATE)  /* lcd_update() requested? */
    {
        int x1 = MAX(graybuf->x, 0);
        int x2 = MIN(graybuf->x + graybuf->width, LCD_WIDTH);
        int y1 = MAX(graybuf->by << 3, 0);
        int y2 = MIN((graybuf->by + graybuf->bheight) << 3, LCD_HEIGHT);

        if (y1 > 0)  /* refresh part above overlay, full width */
            rb->lcd_update_rect(0, 0, LCD_WIDTH, y1);

        if (y2 < LCD_HEIGHT) /* refresh part below overlay, full width */
            rb->lcd_update_rect(0, y2, LCD_WIDTH, LCD_HEIGHT - y2);

        if (x1 > 0) /* refresh part to the left of overlay */
            rb->lcd_update_rect(0, y1, x1, y2 - y1);

        if (x2 < LCD_WIDTH) /* refresh part to the right of overlay */
            rb->lcd_update_rect(x2, y1, LCD_WIDTH - x2, y2 - y1);

        graybuf->flags &= ~GRAY_DEFERRED_UPDATE; /* clear request */
    }
}

/* Set a pixel to a specific bit pattern
 * This is the fundamental graphics primitive, asm optimized */
void graypixel(int x, int y, unsigned long pattern)
{
    register long address, mask, random;

    /* Some (pseudo-)random function must be used here to shift the bit
     * pattern randomly, otherwise you would get flicker and/or moire.
     * Since rand() is relatively slow, I've implemented a simple, but very
     * fast pseudo-random generator based on linear congruency in assembler.
     * It delivers 16 pseudo-random bits in each iteration. */

    /* simple but fast pseudo-random generator */
    asm(
        "mov.w   @%1,%0          \n"  /* load last value */
        "mov     #75,r1          \n"
        "mulu    %0,r1           \n"  /* multiply by 75 */
        "sts     macl,%0         \n"  /* get result */
        "add     #74,%0          \n"  /* add another 74 */
        "mov.w   %0,@%1          \n"  /* store new value */
        /* Since the lower bits are not very random: */
        "shlr8   %0              \n"  /* get bits 8..15 (need max. 5) */
        "and     %2,%0           \n"  /* mask out unneeded bits */
        : /* outputs */
        /* %0 */ "=&r"(random)
        : /* inputs */
        /* %1 */ "r"(&gray_random_buffer),
        /* %2 */ "r"(graybuf->randmask)
        : /* clobbers */
        "r1","macl"
    );

    /* precalculate mask and byte address in first bitplane */
    asm(
        "mov     %3,%0           \n"  /* take y as base for address offset */
        "shlr2   %0              \n"  /* shift right by 3 (= divide by 8) */
        "shlr    %0              \n"
        "mulu    %0,%2           \n"  /* multiply with width */
        "and     #7,%3           \n"  /* get lower 3 bits of y */
        "sts     macl,%0         \n"  /* get mulu result */
        "add     %4,%0           \n"  /* add base + x to get final address */

        "mov     %3,%1           \n"  /* move lower 3 bits of y out of r0 */
        "mova    .pp_table,%3    \n"  /* get address of mask table in r0 */
        "bra     .pp_end         \n"  /* skip the table */
        "mov.b   @(%3,%1),%1     \n"  /* get entry from mask table */
        
        ".align  2               \n"
    ".pp_table:                  \n"  /* mask table */
        ".byte   0x01            \n"
        ".byte   0x02            \n"
        ".byte   0x04            \n"
        ".byte   0x08            \n"
        ".byte   0x10            \n"
        ".byte   0x20            \n"
        ".byte   0x40            \n"
        ".byte   0x80            \n"

    ".pp_end:                    \n"
        : /* outputs */
        /* %0 */ "=&r"(address),    
        /* %1 */ "=&r"(mask)
        : /* inputs */
        /* %2 */ "r"(graybuf->width),
        /* %3 = r0 */ "z"(y),
        /* %4 */ "r"(graybuf->data + x)
        : /* clobbers */
        "macl"
    );

    /* the hard part: set bits in all bitplanes according to pattern */
    asm(
        "cmp/hs  %1,%5           \n"  /* random >= depth ? */
        "bf      .p_ntrim        \n"
        "sub     %1,%5           \n"  /* yes: random -= depth */
        /* it's sufficient to do this once, since the mask guarantees
         * random < 2 * depth */
    ".p_ntrim:                   \n"
    
        /* calculate some addresses */
        "mulu    %4,%1           \n"  /* end address offset */
        "not     %3,r1           \n"  /* get inverse mask (for "and") */
        "sts     macl,%1         \n"  /* result of mulu */
        "mulu    %4,%5           \n"  /* address offset of <random>'th plane */
        "add     %2,%1           \n"  /* end offset -> end address */
        "sts     macl,%5         \n"  /* result of mulu */
        "add     %2,%5           \n"  /* address of <random>'th plane */
        "bra     .p_start1       \n"
        "mov     %5,r2           \n"  /* copy address */

        /* first loop: set bits from <random>'th bitplane to last */
    ".p_loop1:                   \n"
        "mov.b   @r2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "and     r1,r3           \n"  /* reset bit (-> "white") */
        "bf      .p_white1       \n"  /* t=0? -> "white" bit */
        "or      %3,r3           \n"  /* set bit ("black" bit) */
    ".p_white1:                  \n"
        "mov.b   r3,@r2          \n"  /* store data byte */
        "add     %4,r2           \n"  /* advance address to next bitplane */
    ".p_start1:                  \n"
        "cmp/hi  r2,%1           \n"  /* address < end address ? */
        "bt      .p_loop1        \n"
        
        "bra     .p_start2       \n"
        "nop                     \n"

        /* second loop: set bits from first to <random-1>'th bitplane
         * Bit setting works the other way round here to equalize average
         * execution times for bright and dark pixels */
    ".p_loop2:                   \n"
        "mov.b   @%2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "or      %3,r3           \n"  /* set bit (-> "black") */
        "bt      .p_black2       \n"  /* t=1? -> "black" bit */
        "and     r1,r3           \n"  /* reset bit ("white" bit) */
    ".p_black2:                  \n"
        "mov.b   r3,@%2          \n"  /* store data byte */
        "add     %4,%2           \n"  /* advance address to next bitplane */
    ".p_start2:                  \n"
        "cmp/hi  %2,%5           \n"  /* address < <random>'th address ? */
        "bt      .p_loop2        \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(pattern),
        /* %1 */ "r"(graybuf->depth),
        /* %2 */ "r"(address),
        /* %3 */ "r"(mask),
        /* %4 */ "r"(graybuf->plane_size),
        /* %5 */ "r"(random)
        : /* clobbers */
        "r1", "r2", "r3", "macl"
    );
}

/* Set 8 pixels to specific gray values at once, asm optimized
 * This greatly enhances performance of gray_fillrect() and gray_drawgraymap()
 * for larger rectangles and graymaps */
void grayblock(int x, int by, unsigned char* src, int stride)
{
    /* precalculate the bit patterns with random shifts (same RNG as graypixel,
     * see there for an explanation) for all 8 pixels and put them on the 
     * stack (!) */
    asm(
        "mova    .gb_reload,r0   \n"  /* set default loopback address */
        "tst     %1,%1           \n"  /* stride == 0 ? */
        "bf      .gb_needreload  \n"  /* no: keep that address */
        "mova    .gb_reuse,r0    \n"  /* yes: set shortcut (no reload) */
    ".gb_needreload:             \n"
        "mov     r0,r2           \n"  /* loopback address to r2 */
        "mov     #7,r3           \n"  /* loop count in r3: 8 pixels */

        ".align  2               \n"  /** load pattern for pixel **/
    ".gb_reload:                 \n"
        "mov.b   @%0,r0          \n"  /* load src byte */
        "extu.b  r0,r0           \n"  /* extend unsigned */
        "mulu    %2,r0           \n"  /* macl = byte * depth; */
        "add     %1,%0           \n"  /* src += stride; */
        "sts     macl,r4         \n"  /* r4 = macl; */
        "add     r4,r0           \n"  /* byte += r4; */
        "shlr8   r0              \n"  /* byte >>= 8; */
        "shll2   r0              \n"
        "mov.l   @(r0,%3),r4     \n"  /* r4 = bitpattern[byte]; */

        ".align  2               \n"  /** RNG **/
    ".gb_reuse:                  \n"
        "mov.w   @%4,r1          \n"  /* load last value */
        "mov     #75,r0          \n"
        "mulu    r0,r1           \n"  /* multiply by 75 */
        "sts     macl,r1         \n"
        "add     #74,r1          \n"  /* add another 74 */
        "mov.w   r1,@%4          \n"  /* store new value */
        /* Since the lower bits are not very random: */
        "shlr8   r1              \n"  /* get bits 8..15 (need max. 5) */
        "and     %5,r1           \n"  /* mask out unneeded bits */

        "cmp/hs  %2,r1           \n"  /* random >= depth ? */
        "bf      .gb_ntrim       \n"
        "sub     %2,r1           \n"  /* yes: random -= depth; */
    ".gb_ntrim:                  \n"

        "mov.l   .ashlsi3,r0     \n"  /** rotate pattern **/
        "jsr     @r0             \n"  /* shift r4 left by r1 */
        "mov     r1,r5           \n"

        "mov     %2,r5           \n"
        "sub     r1,r5           \n"  /* r5 = depth - r1 */
        "mov.l   .lshrsi3,r1     \n"
        "jsr     @r1             \n"  /* shift r4 right by r5 */
        "mov     r0,r1           \n"  /* last result stored in r1 */

        "or      r1,r0           \n"  /* rotated_pattern = r0 | r1 */
        "mov.l   r0,@-r15        \n"  /* push pattern */
        
        "cmp/pl  r3              \n"  /* loop count > 0? */
        "bf      .gb_patdone     \n"  /* no: done */

        "jmp     @r2             \n"  /* yes: loop */
        "add     #-1,r3          \n"  /* decrease loop count */

        ".align  2               \n"
    ".ashlsi3:                   \n"  /* C library routine: */
        ".long   ___ashlsi3      \n"  /* shift r4 left by r5, return in r0 */
    ".lshrsi3:                   \n"  /* C library routine: */
        ".long   ___lshrsi3      \n"  /* shift r4 right by r5, return in r0 */
        /* both routines preserve r4, destroy r5 and take ~16 cycles */

    ".gb_patdone:                \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(src),
        /* %1 */ "r"(stride),
        /* %2 */ "r"(graybuf->depth),
        /* %3 */ "r"(graybuf->bitpattern),
        /* %4 */ "r"(&gray_random_buffer),
        /* %5 */ "r"(graybuf->randmask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "macl"
    );

    /* calculate start address in first bitplane and end address */
    register unsigned char *address = graybuf->data + x
                                    + MULU16(graybuf->width, by);
    register unsigned char *end_addr = address
                           + MULU16(graybuf->depth, graybuf->plane_size);

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the stack */
    asm (
        "mov.l   @r15+,r1        \n"  /* pop all 8 patterns */
        "mov.l   @r15+,r2        \n"
        "mov.l   @r15+,r3        \n"
        "mov.l   @r15+,r4        \n"
        "mov.l   @r15+,r5        \n"
        "mov.l   @r15+,r6        \n"
        "mov.l   @r15+,r7        \n"
        "mov.l   @r15+,r8        \n"

    ".gb_loop:                   \n"  /* loop for all bitplanes */
        "shlr    r1              \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0              \n"  /* rotate t bit into r0 */
        "shlr    r2              \n"
        "rotcl   r0              \n"
        "shlr    r3              \n"
        "rotcl   r0              \n"
        "shlr    r4              \n"
        "rotcl   r0              \n"
        "shlr    r5              \n"
        "rotcl   r0              \n"
        "shlr    r6              \n"
        "rotcl   r0              \n"
        "shlr    r7              \n"
        "rotcl   r0              \n"
        "shlr    r8              \n"
        "rotcl   r0              \n"
        "mov.b   r0,@%0          \n"  /* store byte to bitplane */
        "add     %2,%0           \n"  /* advance to next bitplane */
        "cmp/hi  %0,%1           \n"  /* last bitplane done? */
        "bt      .gb_loop        \n"  /* no: loop */
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(address),
        /* %1 */ "r"(end_addr),
        /* %2 */ "r"(graybuf->plane_size)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8"
    );
}

/* Invert the bits for 1-8 pixels within the buffer */
void grayinvertmasked(int x, int by, unsigned char mask)
{
    asm(
        "mulu    %4,%5           \n"  /* width * by (offset of row) */
        "mov     #0,r1           \n"  /* current_plane = 0 */
        "sts     macl,r2         \n"  /* get mulu result */
        "add     r2,%1           \n"  /* -> address in 1st bitplane */
        
    ".i_loop:                    \n"
        "mov.b   @%1,r2          \n"  /* get data byte */
        "add     #1,r1           \n"  /* current_plane++; */
        "xor     %2,r2           \n"  /* invert bits */
        "mov.b   r2,@%1          \n"  /* store data byte */
        "add     %3,%1           \n"  /* advance address to next bitplane */
        "cmp/hi  r1,%0           \n"  /* current_plane < depth ? */
        "bt      .i_loop         \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(graybuf->data + x),
        /* %2 */ "r"(mask),
        /* %3 */ "r"(graybuf->plane_size),
        /* %4 */ "r"(graybuf->width),
        /* %5 */ "r"(by)
        : /* clobbers */
        "r1", "r2", "macl"
    );
}

/*** public core functions ***/

/** prototypes **/

int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth);
void gray_release_buffer(void);
void gray_position_display(int x, int by);
void gray_show_display(bool enable);

/** implementation **/

/* Prepare the grayscale display buffer
 *
 * arguments:
 *   gbuf      = pointer to the memory area to use (e.g. plugin buffer)
 *   gbuf_size = max usable size of the buffer
 *   width     = width in pixels  (1..112)
 *   bheight   = height in 8-pixel units  (1..8)
 *   depth     = desired number of shades - 1  (1..32)
 *
 * result:
 *   = depth  if there was enough memory
 *   < depth  if there wasn't enough memory. The number of displayable
 *            shades is smaller than desired, but it still works
 *   = 0      if there wasn't even enough memory for 1 bitplane (black & white)
 *
 * You can request any depth from 1 to 32, not just powers of 2. The routine
 * performs "graceful degradation" if the memory is not sufficient for the
 * desired depth. As long as there is at least enough memory for 1 bitplane,
 * it creates as many bitplanes as fit into memory, although 1 bitplane will
 * only deliver black & white display.
 *
 * The total memory needed can be calculated as follows:
 *   total_mem =
 *     sizeof(tGraymap)      (= 48 bytes currently)
 *   + sizeof(long)          (=  4 bytes)
 *   + (width * bheight + sizeof(long)) * depth
 *   + 0..3                  (longword alignment of grayscale display buffer)
 */
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth)
{
    int possible_depth, plane_size;
    int i, j;

    if ((unsigned) width > LCD_WIDTH 
        || (unsigned) bheight > (LCD_HEIGHT >> 3) 
        || depth < 1)
        return 0;

    while ((unsigned long)gbuf & 3)  /* the buffer has to be long aligned */
    { 
        gbuf++;
        gbuf_size--;
    }

    plane_size = width * bheight;
    possible_depth = (gbuf_size - sizeof(tGraybuf) - sizeof(unsigned long))
                     / (plane_size + sizeof(unsigned long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, 32);
    depth = MIN(depth, possible_depth);

    graybuf = (tGraybuf *) gbuf; /* global pointer to buffer structure */

    graybuf->x = 0;
    graybuf->by = 0;
    graybuf->width = width;
    graybuf->height = bheight << 3;
    graybuf->bheight = bheight;
    graybuf->plane_size = plane_size;
    graybuf->depth = depth;
    graybuf->cur_plane = 0;
    graybuf->flags = 0;
    graybuf->data = gbuf + sizeof(tGraybuf);
    graybuf->bitpattern = (unsigned long *) (graybuf->data 
                           + depth * plane_size);
    
    i = depth;
    j = 8;
    while (i != 0)
    {
        i >>= 1;
        j--;
    }
    graybuf->randmask = 0xFF >> j;

    /* initial state is all white */
    rb->memset(graybuf->data, 0, depth * plane_size);
    
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

        graybuf->bitpattern[i] = pattern;
    }
    
    return depth;
}

/* Release the grayscale display buffer
 *
 * Switches the grayscale overlay off at first if it is still running,
 * then sets the pointer to NULL.
 * DO CALL either this function or at least gray_show_display(false)
 * before you exit, otherwise nasty things may happen.
 */
void gray_release_buffer(void)
{
    gray_show_display(false);
    graybuf = NULL;
}

/* Set position of the top left corner of the grayscale overlay
 *
 * arguments:
 *   x  = left margin in pixels
 *   by = top margin in 8-pixel units
 *
 * You may set this in a way that the overlay spills across the right or
 * bottom display border. In this case it will simply be clipped by the 
 * LCD controller. You can even set negative values, this will clip at the
 * left or top border. I did not test it, but the limits may be +127 / -128
 *
 * If you use this while the grayscale overlay is running, the now-freed area
 * will be restored.
 */
void gray_position_display(int x, int by)
{
    if (graybuf == NULL)
        return;

    graybuf->x = x;
    graybuf->by = by;
    
    if (graybuf->flags & GRAY_RUNNING)
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
}

/* Switch the grayscale overlay on or off
 *
 * arguments:
 *   enable = true:  the grayscale overlay is switched on if initialized
 *          = false: the grayscale overlay is switched off and the regular lcd
 *                   content is restored
 *
 * DO NOT call lcd_update() or any other api function that directly accesses
 * the lcd while the grayscale overlay is running! If you need to do
 * lcd_update() to update something outside the grayscale overlay area, use 
 * gray_deferred_update() instead.
 *
 * Other functions to avoid are:
 *   lcd_blit() (obviously), lcd_update_rect(), lcd_set_contrast(),
 *   lcd_set_invert_display(), lcd_set_flip(), lcd_roll()
 *
 * The grayscale display consumes ~50 % CPU power (for a full screen overlay,
 * less if the overlay is smaller) when switched on. You can switch the overlay
 * on and off as many times as you want.
 */
void gray_show_display(bool enable)
{
    if (graybuf == NULL)
        return;

    if (enable)
    {
        graybuf->flags |= GRAY_RUNNING;
        rb->plugin_register_timer(FREQ / 67, 1, gray_timer_isr);
    }
    else
    {
        rb->plugin_unregister_timer();
        graybuf->flags &= ~GRAY_RUNNING;
        rb->lcd_update(); /* restore whatever there was before */
    }
}

/*** public optional functions ***/

/* Here are the various graphics primitives. Cut out functions you do not
 * need in order to keep plugin code size down.
 */

/** prototypes **/

/* functions affecting the whole display */
void gray_clear_display(void);
void gray_black_display(void);
void gray_deferred_update(void);

/* scrolling functions */
void gray_scroll_left(int count, bool black_border);
void gray_scroll_right(int count, bool black_border);
void gray_scroll_up8(bool black_border);
void gray_scroll_down8(bool black_border);
void gray_scroll_up(int count, bool black_border);
void gray_scroll_down(int count, bool black_border);

/* pixel functions */
void gray_drawpixel(int x, int y, int brightness);
void gray_invertpixel(int x, int y);

/* line functions */
void gray_drawline(int x1, int y1, int x2, int y2, int brightness);
void gray_invertline(int x1, int y1, int x2, int y2);

/* rectangle functions */
void gray_drawrect(int x1, int y1, int x2, int y2, int brightness);
void gray_fillrect(int x1, int y1, int x2, int y2, int brightness);
void gray_invertrect(int x1, int y1, int x2, int y2);

/* bitmap functions */
void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny,
                      int stride);
void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
                     int stride, bool draw_bg, int fg_brightness,
                     int bg_brightness);

/** implementation **/

/* Clear the grayscale display (sets all pixels to white)
 */
void gray_clear_display(void)
{
    if (graybuf == NULL)
        return;

    rb->memset(graybuf->data, 0, MULU16(graybuf->depth, graybuf->plane_size));
}

/* Set the grayscale display to all black
 */
void gray_black_display(void)
{
    if (graybuf == NULL)
        return;

    rb->memset(graybuf->data, 0xFF, MULU16(graybuf->depth, graybuf->plane_size));
}

/* Do an lcd_update() to show changes done by rb->lcd_xxx() functions (in areas
 * of the screen not covered by the grayscale overlay). If the grayscale 
 * overlay is running, the update will be done in the next call of the
 * interrupt routine, otherwise it will be performed right away. See also
 * comment for the gray_show_display() function.
 */
void gray_deferred_update(void)
{
    if (graybuf != NULL && (graybuf->flags & GRAY_RUNNING))
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
    else
        rb->lcd_update();
}

/* Scroll the whole grayscale buffer left by <count> pixels
 *
 * black_border determines if the pixels scrolled in at the right are black
 * or white 
 *
 * Scrolling left/right by an even pixel count is almost twice as fast as
 * scrolling by an odd pixel count.
 */
void gray_scroll_left(int count, bool black_border)
{
    int by, d;
    unsigned char *ptr;
    unsigned char filler;

    if (graybuf == NULL || (unsigned) count >= (unsigned) graybuf->width)
        return;
        
    if (black_border)
        filler = 0xFF;
    else
        filler = 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 0; by < graybuf->bheight; by++)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
        {   
            if (count & 1)  /* odd count: scroll byte-wise */
                asm volatile (
                ".sl_loop1:                   \n"
                    "mov.b   @%0+,r1          \n"
                    "mov.b   r1,@(%2,%0)      \n"
                    "cmp/hi  %0,%1            \n"
                    "bt      .sl_loop1        \n"
                    : /* outputs */
                    : /* inputs */
                    /* %0 */ "r"(ptr + count),
                    /* %1 */ "r"(ptr + graybuf->width),
                    /* %2 */ "z"(-count - 1)
                    : /* clobbers */
                    "r1"
                );
            else           /* even count: scroll word-wise */
                asm volatile (
                ".sl_loop2:                   \n"
                    "mov.w   @%0+,r1          \n"
                    "mov.w   r1,@(%2,%0)      \n"
                    "cmp/hi  %0,%1            \n"
                    "bt      .sl_loop2        \n"
                    : /* outputs */
                    : /* inputs */
                    /* %0 */ "r"(ptr + count),
                    /* %1 */ "r"(ptr + graybuf->width),
                    /* %2 */ "z"(-count - 2)
                    : /* clobbers */
                    "r1"
                );

            rb->memset(ptr + graybuf->width - count, filler, count);
            ptr += graybuf->plane_size;
        }
    }
}

/* Scroll the whole grayscale buffer right by <count> pixels
 *
 * black_border determines if the pixels scrolled in at the left are black
 * or white
 *
 * Scrolling left/right by an even pixel count is almost twice as fast as
 * scrolling by an odd pixel count.
 */
void gray_scroll_right(int count, bool black_border)
{
    int by, d;
    unsigned char *ptr;
    unsigned char filler;

    if (graybuf == NULL || (unsigned) count >= (unsigned) graybuf->width)
        return;

    if (black_border)
        filler = 0xFF;
    else
        filler = 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 0; by < graybuf->bheight; by++)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
        {
            if (count & 1)  /* odd count: scroll byte-wise */
                asm volatile (
                ".sr_loop1:                   \n"
                    "mov.b   @(%2,%0),r1      \n"
                    "mov.b   r1,@-%0          \n"
                    "cmp/hi  %1,%0            \n"
                    "bt      .sr_loop1        \n"
                    : /* outputs */
                    : /* inputs */
                    /* %0 */ "r"(ptr + graybuf->width),
                    /* %1 */ "r"(ptr + count),
                    /* %2 */ "z"(-count - 1)
                    : /* clobbers */
                    "r1"
                );
            else            /* even count: scroll word-wise */
                asm volatile (
                ".sr_loop2:                   \n"
                    "mov.w   @(%2,%0),r1      \n"
                    "mov.w   r1,@-%0          \n"
                    "cmp/hi  %1,%0            \n"
                    "bt      .sr_loop2        \n"
                    : /* outputs */
                    : /* inputs */
                    /* %0 */ "r"(ptr + graybuf->width),
                    /* %1 */ "r"(ptr + count),
                    /* %2 */ "z"(-count - 2)
                    : /* clobbers */
                    "r1"
                );

            rb->memset(ptr, filler, count);
            ptr += graybuf->plane_size;
        }
    }
}

/* Scroll the whole grayscale buffer up by 8 pixels
 *
 * black_border determines if the pixels scrolled in at the bottom are black
 * or white
 *
 * Scrolling up/down by 8 pixels is very fast.
 */
void gray_scroll_up8(bool black_border)
{
    int by, d;
    unsigned char *ptr;
    unsigned char filler;

    if (graybuf == NULL)
        return;
        
    if (black_border)
        filler = 0xFF;
    else
        filler = 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 1; by < graybuf->bheight; by++)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
        {
            rb->memcpy(ptr - graybuf->width, ptr, graybuf->width);
            ptr += graybuf->plane_size;
        }
    }
    /* fill last row */
    ptr = graybuf->data + graybuf->plane_size - graybuf->width;
    for (d = 0; d < graybuf->depth; d++) 
    {
        rb->memset(ptr, filler, graybuf->width);
        ptr += graybuf->plane_size;
    }
}

/* Scroll the whole grayscale buffer down by 8 pixels
 *
 * black_border determines if the pixels scrolled in at the top are black
 * or white
 *
 * Scrolling up/down by 8 pixels is very fast.
 */
void gray_scroll_down8(bool black_border)
{
    int by, d;
    unsigned char *ptr;
    unsigned char filler;

    if (graybuf == NULL)
        return;
        
    if (black_border)
        filler = 0xFF;
    else
        filler = 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = graybuf->bheight - 1; by > 0; by--)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
        {
            rb->memcpy(ptr, ptr - graybuf->width, graybuf->width);
            ptr += graybuf->plane_size;
        }
    }
    /* fill first row */
    ptr = graybuf->data;
    for (d = 0; d < graybuf->depth; d++) 
    {
        rb->memset(ptr, filler, graybuf->width);
        ptr += graybuf->plane_size;
    }
}

/* Scroll the whole grayscale buffer up by <count> pixels (<= 7)
 *
 * black_border determines if the pixels scrolled in at the bottom are black
 * or white
 *
 * Scrolling up/down pixel-wise is significantly slower than scrolling
 * left/right or scrolling up/down byte-wise because it involves bit
 * shifting. That's why it is asm optimized.
 */
void gray_scroll_up(int count, bool black_border)
{
    unsigned long filler;

    if (graybuf == NULL || (unsigned) count > 7)
        return;

    if (black_border)
        filler = 0xFF;
    else
        filler = 0;
    
    /* scroll column by column to minimize flicker */
    asm(
        "mov     #0,r6           \n"  /* x = 0 */
        "mova    .su_shifttbl,r0 \n"  /* calculate jump destination for */
        "mov.b   @(r0,%6),%6     \n"  /*   shift amount from table */
        "bra     .su_cloop       \n"  /* skip table */
        "add     r0,%6           \n"

        ".align  2               \n"
    ".su_shifttbl:               \n"  /* shift jump offset table */
        ".byte   .su_shift0 - .su_shifttbl \n"
        ".byte   .su_shift1 - .su_shifttbl \n"
        ".byte   .su_shift2 - .su_shifttbl \n"
        ".byte   .su_shift3 - .su_shifttbl \n"
        ".byte   .su_shift4 - .su_shifttbl \n"
        ".byte   .su_shift5 - .su_shifttbl \n"
        ".byte   .su_shift6 - .su_shifttbl \n"
        ".byte   .su_shift7 - .su_shifttbl \n"

    ".su_cloop:                  \n"  /* repeat for every column */
        "mov     %1,r2           \n"  /* get start address */
        "mov     #0,r3           \n"  /* current_plane = 0 */
        
    ".su_oloop:                  \n"  /* repeat for every bitplane */
        "mov     r2,r4           \n"  /* get start address */
        "mov     #0,r5           \n"  /* current_row = 0 */
        "mov     %5,r1           \n"  /* get filler bits */
        
    ".su_iloop:                  \n"  /* repeat for all rows */
        "sub     %2,r4           \n"  /* address -= width */
        "mov.b   @r4,r0          \n"  /* get data byte */
        "shll8   r1              \n"  /* old data to 2nd byte */
        "extu.b  r0,r0           \n"  /* extend unsigned */
        "or      r1,r0           \n"  /* combine old data */
        "jmp     @%6             \n"  /* jump into shift "path" */
        "extu.b  r0,r1           \n"  /* store data for next round */
        
    ".su_shift6:                 \n"  /* shift right by 0..7 bits */
        "shlr2   r0              \n"
    ".su_shift4:                 \n"
        "shlr2   r0              \n"
    ".su_shift2:                 \n"
        "bra     .su_shift0      \n"
        "shlr2   r0              \n"
    ".su_shift7:                 \n"
        "shlr2   r0              \n"
    ".su_shift5:                 \n"
        "shlr2   r0              \n"
    ".su_shift3:                 \n"
        "shlr2   r0              \n"
    ".su_shift1:                 \n"
        "shlr    r0              \n"
    ".su_shift0:                 \n"

        "mov.b   r0,@r4          \n"  /* store data */
        "add     #1,r5           \n"  /* current_row++ */
        "cmp/hi  r5,%3           \n"  /* current_row < bheight ? */
        "bt      .su_iloop       \n"

        "add     %4,r2           \n"  /* start_address += plane_size */
        "add     #1,r3           \n"  /* current_plane++ */
        "cmp/hi  r3,%0           \n"  /* current_plane < depth ? */
        "bt      .su_oloop       \n"

        "add     #1,%1           \n"  /* start_address++ */
        "add     #1,r6           \n"  /* x++ */
        "cmp/hi  r6,%2           \n"  /* x < width ? */
        "bt      .su_cloop       \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(graybuf->data + graybuf->plane_size),
        /* %2 */ "r"(graybuf->width),
        /* %3 */ "r"(graybuf->bheight),
        /* %4 */ "r"(graybuf->plane_size),
        /* %5 */ "r"(filler),
        /* %6 */ "r"(count)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6"
    );
}

/* Scroll the whole grayscale buffer down by <count> pixels (<= 7)
 *
 * black_border determines if the pixels scrolled in at the top are black
 * or white
 *
 * Scrolling up/down pixel-wise is significantly slower than scrolling
 * left/right or scrolling up/down byte-wise because it involves bit
 * shifting. That's why it is asm optimized.
 */
void gray_scroll_down(int count, bool black_border)
{
    unsigned long filler;

    if (graybuf == NULL || (unsigned) count > 7)
        return;

    if (black_border)
        filler = 0xFF << count; /* calculate filler bits */
    else
        filler = 0;
    
    /* scroll column by column to minimize flicker */
    asm(
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
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(graybuf->data),
        /* %2 */ "r"(graybuf->width),
        /* %3 */ "r"(graybuf->bheight),
        /* %4 */ "r"(graybuf->plane_size),
        /* %5 */ "r"(filler),
        /* %6 */ "r"(count)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6"
    );
}

/* Set a pixel to a specific gray value
 *
 * brightness is 0..255 (black to white) regardless of real bit depth
 */
void gray_drawpixel(int x, int y, int brightness)
{
    if (graybuf == NULL 
        || (unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height
        || (unsigned) brightness > 255)
        return;

    graypixel(x, y, graybuf->bitpattern[MULU16(brightness,
                    graybuf->depth + 1) >> 8]);
}

/* Invert a pixel
 *
 * The bit pattern for that pixel in the buffer is inverted, so white becomes
 * black, light gray becomes dark gray etc.
 */
void gray_invertpixel(int x, int y)
{
    if (graybuf == NULL 
        || (unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;

    grayinvertmasked(x, (y >> 3), 1 << (y & 7));
}

/* Draw a line from (x1, y1) to (x2, y2) with a specific gray value
 *
 * brightness is 0..255 (black to white) regardless of real bit depth
 */
void gray_drawline(int x1, int y1, int x2, int y2, int brightness)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    unsigned long pattern;

    if (graybuf == NULL 
        || (unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y2 >= (unsigned) graybuf->height
        || (unsigned) brightness > 255)
        return;

    pattern = graybuf->bitpattern[MULU16(brightness, graybuf->depth + 1) >> 8];

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i=0; i<numpixels; i++)
    {
        graypixel(x, y, pattern);

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Invert a line from (x1, y1) to (x2, y2)
 *
 * The bit patterns for the pixels of the line are inverted, so white becomes
 * black, light gray becomes dark gray etc.
 */
void gray_invertline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;

    if (graybuf == NULL 
        || (unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y2 >= (unsigned) graybuf->height)
        return;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);
    xinc2 = 1;
    yinc2 = 1;

    if (deltax >= deltay)
    {
        numpixels = deltax;
        d = 2 * deltay - deltax;
        dinc1 = deltay * 2;
        dinc2 = (deltay - deltax) * 2;
        xinc1 = 1;
        yinc1 = 0;
    }
    else
    {
        numpixels = deltay;
        d = 2 * deltax - deltay;
        dinc1 = deltax * 2;
        dinc2 = (deltax - deltay) * 2;
        xinc1 = 0;
        yinc1 = 1;
    }
    numpixels++; /* include endpoints */

    if (x1 > x2)
    {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }

    if (y1 > y2)
    {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;

    for (i=0; i<numpixels; i++)
    {
        grayinvertmasked(x, (y >> 3), 1 << (y & 7));

        if (d < 0)
        {
            d += dinc1;
            x += xinc1;
            y += yinc1;
        }
        else
        {
            d += dinc2;
            x += xinc2;
            y += yinc2;
        }
    }
}

/* Draw a (hollow) rectangle with a specific gray value,
 * corners are (x1, y1) and (x2, y2)
 *
 * brightness is 0..255 (black to white) regardless of real bit depth
 */
void gray_drawrect(int x1, int y1, int x2, int y2, int brightness)
{
    int x, y;
    unsigned long pattern;
    unsigned char srcpixel;

    if (graybuf == NULL 
        || (unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y2 >= (unsigned) graybuf->height
        || (unsigned) brightness > 255)
        return;

    if (y1 > y2)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }
    if (x1 > x2)
    {
        x = x1;
        x1 = x2; 
        x2 = x;
    }

    pattern = graybuf->bitpattern[MULU16(brightness, graybuf->depth + 1) >> 8];
    srcpixel = brightness;

    for (x = x1 + 1; x < x2; x++)
    {
        graypixel(x, y1, pattern);
        graypixel(x, y2, pattern);
    }
    for (y = y1; y <= y2; )
    {
        if (!(y & 7) && (y2 - y >= 7))
        /* current row byte aligned in fb & at least 8 rows left */
        {
            /* shortcut: draw all 8 rows at once: 2..3 times faster */
            grayblock(x1, y >> 3, &srcpixel, 0);
            grayblock(x2, y >> 3, &srcpixel, 0);
            y += 8;
        }
        else
        {
            graypixel(x1, y, pattern);
            graypixel(x2, y, pattern);
            y++;
        }
    }
}

/* Fill a rectangle with a specific gray value 
 * corners are (x1, y1) and (x2, y2)
 *
 * brightness is 0..255 (black to white) regardless of real bit depth
 */
void gray_fillrect(int x1, int y1, int x2, int y2, int brightness)
{
    int x, y;
    unsigned long pattern;
    unsigned char srcpixel;

    if (graybuf == NULL 
        || (unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y2 >= (unsigned) graybuf->height
        || (unsigned) brightness > 255)
        return;
        
    if (y1 > y2)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }
    if (x1 > x2)
    {
        x = x1;
        x1 = x2; 
        x2 = x;
    }

    pattern = graybuf->bitpattern[MULU16(brightness, graybuf->depth + 1) >> 8];
    srcpixel = brightness;

    for (y = y1; y <= y2; )
    {
        if (!(y & 7) && (y2 - y >= 7))
        /* current row byte aligned in fb & at least 8 rows left */
        {
            for (x = x1; x <= x2; x++)
            {
                /* shortcut: draw all 8 rows at once: 2..3 times faster */
                grayblock(x, y >> 3, &srcpixel, 0);
            }
            y += 8;
        }
        else
        {
            for (x = x1; x <= x2; x++)
            {
                graypixel(x, y, pattern);
            }
            y++;
        }
    }
}

/* Invert a (solid) rectangle, corners are (x1, y1) and (x2, y2)
 *
 * The bit patterns for all pixels of the rectangle are inverted, so white
 * becomes black, light gray becomes dark gray etc. This is the fastest of
 * all gray_xxxrect() functions! Perfectly suited for cursors.
 */
void gray_invertrect(int x1, int y1, int x2, int y2)
{
    int x, yb, yb1, yb2;
    unsigned char mask;

    if (graybuf == NULL 
        || (unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y2 >= (unsigned) graybuf->height)
        return;

    if (y1 > y2)
    {
        yb = y1;
        y1 = y2;
        y2 = yb;
    }
    if (x1 > x2)
    {
        x = x1;
        x1 = x2; 
        x2 = x;
    }
    
    yb1 = y1 >> 3;
    yb2 = y2 >> 3;

    if (yb1 == yb2)
    {
        mask = 0xFF << (y1 & 7);
        mask &= 0xFF >> (7 - (y2 & 7));

        for (x = x1; x <= x2; x++)
            grayinvertmasked(x, yb1, mask);
    }
    else
    {
        mask = 0xFF << (y1 & 7);

        for (x = x1; x <= x2; x++)
            grayinvertmasked(x, yb1, mask);
            
        for (yb = yb1 + 1; yb < yb2; yb++)
        {
            for (x = x1; x <= x2; x++)
                grayinvertmasked(x, yb, 0xFF);
        }
        
        mask = 0xFF >> (7 - (y2 & 7));

        for (x = x1; x <= x2; x++)
            grayinvertmasked(x, yb2, mask);
    }
}

/* Copy a grayscale bitmap into the display
 * 
 * A grayscale bitmap contains one byte for every pixel that defines the
 * brightness of the pixel (0..255). Bytes are read in row-major order.
 * The <stride> parameter is useful if you want to show only a part of a
 * bitmap. It should always be set to the "row length" of the bitmap, so
 * for displaying the whole bitmap, nx == stride.
 */
void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny,
                      int stride)
{
    int xi, yi;
    unsigned char *row;

    if (graybuf == NULL 
        || (unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;
    
    if ((y + ny) >= graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((x + nx) >= graybuf->width) /* clip right */
        nx = graybuf->width - x;

    for (yi = y; yi < y + ny; )
    {
        row = src;

        if (!(yi & 7) && (y + ny - yi > 7))
        /* current row byte aligned in fb & at least 8 rows left */
        {
            for (xi = x; xi < x + nx; xi++)
            {
                /* shortcut: draw all 8 rows at once: 2..3 times faster */
                grayblock(xi, yi >> 3, row++, stride);
            }
            yi += 8;
            src += stride << 3;
        }
        else
        {
            for (xi = x; xi < x + nx; xi++)
            {
                graypixel(xi, yi, graybuf->bitpattern[MULU16(*row++,
                                  graybuf->depth + 1) >> 8]);
            }
            yi++;
            src += stride;
        }
    }
}

/* Display a bitmap with specific foreground and background gray values
 *
 * This (now) uses the same bitmap format as the core b&w graphics routines,
 * so you can use bmp2rb to generate bitmaps for use with this function as
 * well.
 *
 * A bitmap contains one bit for every pixel that defines if that pixel is
 * foreground (1) or background (0). Bits within a byte are arranged
 * vertically, LSB at top.
 * The bytes are stored in row-major order, with byte 0 being top left,
 * byte 1 2nd from left etc. The first row of bytes defines pixel rows
 * 0..7, the second row defines pixel row 8..15 etc.
 *
 * The <stride> parameter is useful if you want to show only a part of a
 * bitmap. It should always be set to the "row length" of the bitmap.
 *
 * If draw_bg is false, only foreground pixels are drawn, so the background
 * is transparent. In this case bg_brightness is ignored.
 */
void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
                     int stride, bool draw_bg, int fg_brightness,
                     int bg_brightness)
{
    int xi, dy;
    int bits = 0;  /* Have to initialize to prevent warning */
    unsigned long fg_pattern, bg_pattern;
    unsigned char *col;

    if (graybuf == NULL
        || (unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height
        || (unsigned) fg_brightness > 255
        || (unsigned) bg_brightness > 255)
        return;

    if ((y + ny) >= graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((x + nx) >= graybuf->width) /* clip right */
        nx = graybuf->width - x;

    fg_pattern = graybuf->bitpattern[MULU16(fg_brightness,
                                     graybuf->depth + 1) >> 8];

    bg_pattern = graybuf->bitpattern[MULU16(bg_brightness,
                                     graybuf->depth + 1) >> 8];

    for (xi = x; xi < x + nx; xi++)
    {
        col = src++;
        for (dy = 0; dy < ny; dy++)
        {
            if (!(dy & 7))   /* get next 8 bits */
            {
                bits = (int)(*col);
                col += stride;
            }

            if (bits & 0x01)
                graypixel(xi, y + dy, fg_pattern);
            else
                if (draw_bg)
                    graypixel(xi, y + dy, bg_pattern);

            bits >>= 1;
        }
    }
}

/*********************** end grayscale framework ***************************/


/* for portability of below JPEG code */
#define MEMSET(p,v,c) rb->memset(p,v,c)
#define INLINE static inline
#define ENDIAN_SWAP16(n) n /* only for poor little endian machines */



/**************** begin JPEG code ********************/

/* LUT for IDCT, this could also be used for gamma correction */
const unsigned char range_limit[1024] = 
{
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,

    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
    96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
};


/* IDCT implementation */


#define CONST_BITS 13
#define PASS1_BITS 2


/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
* causing a lot of useless floating-point operations at run time.
* To get around this we use the following pre-calculated constants.
* If you change CONST_BITS you may want to add appropriate values.
* (With a reasonable C compiler, you can just rely on the FIX() macro...)
*/
#define FIX_0_298631336  2446 /* FIX(0.298631336) */
#define FIX_0_390180644  3196 /* FIX(0.390180644) */
#define FIX_0_541196100  4433 /* FIX(0.541196100) */
#define FIX_0_765366865  6270 /* FIX(0.765366865) */
#define FIX_0_899976223  7373 /* FIX(0.899976223) */
#define FIX_1_175875602  9633 /* FIX(1.175875602) */
#define FIX_1_501321110 12299 /* FIX(1.501321110) */
#define FIX_1_847759065 15137 /* FIX(1.847759065) */
#define FIX_1_961570560 16069 /* FIX(1.961570560) */
#define FIX_2_053119869 16819 /* FIX(2.053119869) */
#define FIX_2_562915447 20995 /* FIX(2.562915447) */
#define FIX_3_072711026 25172 /* FIX(3.072711026) */



/* Multiply an long variable by an long constant to yield an long result.
* For 8-bit samples with the recommended scaling, all the variable
* and constant values involved are no more than 16 bits wide, so a
* 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
* For 12-bit samples, a full 32-bit multiplication will be needed.
*/
#define MULTIPLY16(var,const)  (((short) (var)) * ((short) (const)))


/* Dequantize a coefficient by multiplying it by the multiplier-table
* entry; produce an int result.  In this module, both inputs and result
* are 16 bits or less, so either int or short multiply will work.
*/
/* #define DEQUANTIZE(coef,quantval)  (((int) (coef)) * (quantval)) */
#define DEQUANTIZE MULTIPLY16

/* Descale and correctly round an int value that's scaled by N bits.
* We assume RIGHT_SHIFT rounds towards minus infinity, so adding
* the fudge factor is correct for either sign of X.
*/
#define DESCALE(x,n) (((x) + (1l << ((n)-1))) >> (n))

#define RANGE_MASK (255 * 4 + 3) /* 2 bits wider than legal samples */



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 1x1 output block.
*/
void idct1x1(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    (void)skip_line; /* unused */
    *p_byte = range_limit[(inptr[0] * quantptr[0] >> 3) & RANGE_MASK];
}



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 2x2 output block.
*/
void idct2x2(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
    unsigned char* outptr;

    /* Pass 1: process columns from input, store into work array. */

    /* Column 0 */
    tmp4 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
    tmp5 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);

    tmp0 = tmp4 + tmp5;
    tmp2 = tmp4 - tmp5;

    /* Column 1 */
    tmp4 = DEQUANTIZE(inptr[8*0+1], quantptr[8*0+1]);
    tmp5 = DEQUANTIZE(inptr[8*1+1], quantptr[8*1+1]);

    tmp1 = tmp4 + tmp5;
    tmp3 = tmp4 - tmp5;

    /* Pass 2: process 2 rows, store into output array. */

    /* Row 0 */
    outptr = p_byte;

    outptr[0] = range_limit[(int) DESCALE(tmp0 + tmp1, 3)
        & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp0 - tmp1, 3)
        & RANGE_MASK];

    /* Row 1 */
    outptr = p_byte + skip_line;

    outptr[0] = range_limit[(int) DESCALE(tmp2 + tmp3, 3)
        & RANGE_MASK];
    outptr[1] = range_limit[(int) DESCALE(tmp2 - tmp3, 3)
        & RANGE_MASK];
}



/*
* Perform dequantization and inverse DCT on one block of coefficients,
* producing a reduced-size 4x4 output block.
*/
void idct4x4(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    int tmp0, tmp2, tmp10, tmp12;
    int z1, z2, z3;
    int * wsptr;
    unsigned char* outptr;
    int ctr;
    int workspace[4*4]; /* buffers data between passes */

    /* Pass 1: process columns from input, store into work array. */

    wsptr = workspace;
    for (ctr = 0; ctr < 4; ctr++, inptr++, quantptr++, wsptr++) 
    {
        /* Even part */

        tmp0 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
        tmp2 = DEQUANTIZE(inptr[8*2], quantptr[8*2]);

        tmp10 = (tmp0 + tmp2) << PASS1_BITS;
        tmp12 = (tmp0 - tmp2) << PASS1_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);
        z3 = DEQUANTIZE(inptr[8*3], quantptr[8*3]);

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp0 = DESCALE(z1 + MULTIPLY16(z3, - FIX_1_847759065), CONST_BITS-PASS1_BITS);
        tmp2 = DESCALE(z1 + MULTIPLY16(z2, FIX_0_765366865), CONST_BITS-PASS1_BITS);

        /* Final output stage */

        wsptr[4*0] = (int) (tmp10 + tmp2);
        wsptr[4*3] = (int) (tmp10 - tmp2);
        wsptr[4*1] = (int) (tmp12 + tmp0);
        wsptr[4*2] = (int) (tmp12 - tmp0);
    }

    /* Pass 2: process 4 rows from work array, store into output array. */

    wsptr = workspace;
    for (ctr = 0; ctr < 4; ctr++)
    {
        outptr = p_byte + (ctr*skip_line);
        /* Even part */

        tmp0 = (int) wsptr[0];
        tmp2 = (int) wsptr[2];

        tmp10 = (tmp0 + tmp2) << CONST_BITS;
        tmp12 = (tmp0 - tmp2) << CONST_BITS;

        /* Odd part */
        /* Same rotation as in the even part of the 8x8 LL&M IDCT */

        z2 = (int) wsptr[1];
        z3 = (int) wsptr[3];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp0 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp2 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        /* Final output stage */

        outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp2,
            CONST_BITS+PASS1_BITS+3)
            & RANGE_MASK];
        outptr[3] = range_limit[(int) DESCALE(tmp10 - tmp2,
            CONST_BITS+PASS1_BITS+3)
            & RANGE_MASK];
        outptr[1] = range_limit[(int) DESCALE(tmp12 + tmp0,
            CONST_BITS+PASS1_BITS+3)
            & RANGE_MASK];
        outptr[2] = range_limit[(int) DESCALE(tmp12 - tmp0,
            CONST_BITS+PASS1_BITS+3)
            & RANGE_MASK];

        wsptr += 4;     /* advance pointer to next row */
    }
}



/*
* Perform dequantization and inverse DCT on one block of coefficients.
*/
void idct8x8(unsigned char* p_byte, int* inptr, int* quantptr, int skip_line)
{
    long tmp0, tmp1, tmp2, tmp3;
    long tmp10, tmp11, tmp12, tmp13;
    long z1, z2, z3, z4, z5;
    int * wsptr;
    unsigned char* outptr;
    int ctr;
    static int workspace[64];  /* buffers data between passes */

    /* Pass 1: process columns from input, store into work array. */
    /* Note results are scaled up by sqrt(8) compared to a true IDCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */

    wsptr = workspace;
    for (ctr = 8; ctr > 0; ctr--)
    {
    /* Due to quantization, we will usually find that many of the input
    * coefficients are zero, especially the AC terms.  We can exploit this
    * by short-circuiting the IDCT calculation for any column in which all
    * the AC terms are zero.  In that case each output is equal to the
    * DC coefficient (with scale factor as needed).
    * With typical images and quantization tables, half or more of the
    * column DCT calculations can be simplified this way.
    */

        if ((inptr[8*1] | inptr[8*2] | inptr[8*3] 
           | inptr[8*4] | inptr[8*5] | inptr[8*6] | inptr[8*7]) == 0)
        {
            /* AC terms all zero */
            int dcval = DEQUANTIZE(inptr[8*0], quantptr[8*0]) << PASS1_BITS;

            wsptr[8*0] = wsptr[8*1] = wsptr[8*2] = wsptr[8*3] = wsptr[8*4] 
                       = wsptr[8*5] = wsptr[8*6] = wsptr[8*7] = dcval;
            inptr++;      /* advance pointers to next column */
            quantptr++;
            wsptr++;
            continue;
        }

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = DEQUANTIZE(inptr[8*2], quantptr[8*2]);
        z3 = DEQUANTIZE(inptr[8*6], quantptr[8*6]);

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        z2 = DEQUANTIZE(inptr[8*0], quantptr[8*0]);
        z3 = DEQUANTIZE(inptr[8*4], quantptr[8*4]);

        tmp0 = (z2 + z3) << CONST_BITS;
        tmp1 = (z2 - z3) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
           transpose is its inverse.  i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = DEQUANTIZE(inptr[8*7], quantptr[8*7]);
        tmp1 = DEQUANTIZE(inptr[8*5], quantptr[8*5]);
        tmp2 = DEQUANTIZE(inptr[8*3], quantptr[8*3]);
        tmp3 = DEQUANTIZE(inptr[8*1], quantptr[8*1]);

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        wsptr[8*0] = (int) DESCALE(tmp10 + tmp3, CONST_BITS-PASS1_BITS);
        wsptr[8*7] = (int) DESCALE(tmp10 - tmp3, CONST_BITS-PASS1_BITS);
        wsptr[8*1] = (int) DESCALE(tmp11 + tmp2, CONST_BITS-PASS1_BITS);
        wsptr[8*6] = (int) DESCALE(tmp11 - tmp2, CONST_BITS-PASS1_BITS);
        wsptr[8*2] = (int) DESCALE(tmp12 + tmp1, CONST_BITS-PASS1_BITS);
        wsptr[8*5] = (int) DESCALE(tmp12 - tmp1, CONST_BITS-PASS1_BITS);
        wsptr[8*3] = (int) DESCALE(tmp13 + tmp0, CONST_BITS-PASS1_BITS);
        wsptr[8*4] = (int) DESCALE(tmp13 - tmp0, CONST_BITS-PASS1_BITS);

        inptr++; /* advance pointers to next column */
        quantptr++;
        wsptr++;
    }

    /* Pass 2: process rows from work array, store into output array. */
    /* Note that we must descale the results by a factor of 8 == 2**3, */
    /* and also undo the PASS1_BITS scaling. */

    wsptr = workspace;
    for (ctr = 0; ctr < 8; ctr++) 
    {
        outptr = p_byte + (ctr*skip_line);
        /* Rows of zeroes can be exploited in the same way as we did with columns.
        * However, the column calculation has created many nonzero AC terms, so
        * the simplification applies less often (typically 5% to 10% of the time).
        * On machines with very fast multiplication, it's possible that the
        * test takes more time than it's worth.  In that case this section
        * may be commented out.
        */

#ifndef NO_ZERO_ROW_TEST
        if ((wsptr[1] | wsptr[2] | wsptr[3] 
           | wsptr[4] | wsptr[5] | wsptr[6] | wsptr[7]) == 0)
        {
            /* AC terms all zero */
            unsigned char dcval = range_limit[(int) DESCALE((long) wsptr[0], 
                PASS1_BITS+3) & RANGE_MASK];

            outptr[0] = dcval;
            outptr[1] = dcval;
            outptr[2] = dcval;
            outptr[3] = dcval;
            outptr[4] = dcval;
            outptr[5] = dcval;
            outptr[6] = dcval;
            outptr[7] = dcval;

            wsptr += 8; /* advance pointer to next row */
            continue;
        }
#endif

        /* Even part: reverse the even part of the forward DCT. */
        /* The rotator is sqrt(2)*c(-6). */

        z2 = (long) wsptr[2];
        z3 = (long) wsptr[6];

        z1 = MULTIPLY16(z2 + z3, FIX_0_541196100);
        tmp2 = z1 + MULTIPLY16(z3, - FIX_1_847759065);
        tmp3 = z1 + MULTIPLY16(z2, FIX_0_765366865);

        tmp0 = ((long) wsptr[0] + (long) wsptr[4]) << CONST_BITS;
        tmp1 = ((long) wsptr[0] - (long) wsptr[4]) << CONST_BITS;

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        /* Odd part per figure 8; the matrix is unitary and hence its
        * transpose is its inverse. i0..i3 are y7,y5,y3,y1 respectively. */

        tmp0 = (long) wsptr[7];
        tmp1 = (long) wsptr[5];
        tmp2 = (long) wsptr[3];
        tmp3 = (long) wsptr[1];

        z1 = tmp0 + tmp3;
        z2 = tmp1 + tmp2;
        z3 = tmp0 + tmp2;
        z4 = tmp1 + tmp3;
        z5 = MULTIPLY16(z3 + z4, FIX_1_175875602); /* sqrt(2) * c3 */

        tmp0 = MULTIPLY16(tmp0, FIX_0_298631336); /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp1 = MULTIPLY16(tmp1, FIX_2_053119869); /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp2 = MULTIPLY16(tmp2, FIX_3_072711026); /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp3 = MULTIPLY16(tmp3, FIX_1_501321110); /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = MULTIPLY16(z1, - FIX_0_899976223); /* sqrt(2) * (c7-c3) */
        z2 = MULTIPLY16(z2, - FIX_2_562915447); /* sqrt(2) * (-c1-c3) */
        z3 = MULTIPLY16(z3, - FIX_1_961570560); /* sqrt(2) * (-c3-c5) */
        z4 = MULTIPLY16(z4, - FIX_0_390180644); /* sqrt(2) * (c5-c3) */

        z3 += z5;
        z4 += z5;

        tmp0 += z1 + z3;
        tmp1 += z2 + z4;
        tmp2 += z2 + z3;
        tmp3 += z1 + z4;

        /* Final output stage: inputs are tmp10..tmp13, tmp0..tmp3 */

        outptr[0] = range_limit[(int) DESCALE(tmp10 + tmp3, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[7] = range_limit[(int) DESCALE(tmp10 - tmp3, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[1] = range_limit[(int) DESCALE(tmp11 + tmp2, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[6] = range_limit[(int) DESCALE(tmp11 - tmp2, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[2] = range_limit[(int) DESCALE(tmp12 + tmp1, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[5] = range_limit[(int) DESCALE(tmp12 - tmp1, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[3] = range_limit[(int) DESCALE(tmp13 + tmp0, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];
        outptr[4] = range_limit[(int) DESCALE(tmp13 - tmp0, 
            CONST_BITS+PASS1_BITS+3) & RANGE_MASK];

        wsptr += 8; /* advance pointer to next row */
    }
}



/* JPEG decoder implementation */


#define HUFF_LOOKAHEAD 8 /* # of bits of lookahead */

struct derived_tbl
{
    /* Basic tables: (element [0] of each array is unused) */
    long mincode[17]; /* smallest code of length k */
    long maxcode[18]; /* largest code of length k (-1 if none) */
    /* (maxcode[17] is a sentinel to ensure huff_DECODE terminates) */
    int valptr[17]; /* huffval[] index of 1st symbol of length k */

    /* Back link to public Huffman table (needed only in slow_DECODE) */
    int* pub;

    /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
    the input data stream.  If the next Huffman code is no more
    than HUFF_LOOKAHEAD bits long, we can obtain its length and
    the corresponding symbol directly from these tables. */
    int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
    unsigned char look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
};

#define QUANT_TABLE_LENGTH  64

/* for type of Huffman table */
#define DC_LEN 28
#define AC_LEN 178

struct huffman_table
{   /* length and code according to JFIF format */
    int huffmancodes_dc[DC_LEN];
    int huffmancodes_ac[AC_LEN];
};

struct frame_component
{
    int ID;
    int horizontal_sampling;
    int vertical_sampling;
    int quanttable_select;
};

struct scan_component
{
    int ID;
    int DC_select;
    int AC_select;
};

struct bitstream
{
    unsigned long get_buffer; /* current bit-extraction buffer */
    int bits_left; /* # of unused bits in it */
    unsigned short* next_input_word;
    long words_left; /* # of words remaining in source buffer */
};

struct jpeg
{
    int x_size, y_size; /* size of image (can be less than block boundary) */
    int x_phys, y_phys; /* physical size, block aligned */
    int x_mbl; /* x dimension of MBL */
    int y_mbl; /* y dimension of MBL */
    int blocks; /* blocks per MBL */

    unsigned short* p_entropy_data;
    long words_in_buffer; /* # of valid words in source buffer */

    int quanttable[4][QUANT_TABLE_LENGTH]; /* raw quantization tables 0-3 */
    int qt_idct[2][QUANT_TABLE_LENGTH]; /* quantization tables for IDCT */

    struct huffman_table hufftable[2]; /* Huffman tables  */
    struct derived_tbl dc_derived_tbls[2]; /* Huffman-LUTs */
    struct derived_tbl ac_derived_tbls[2];

    struct frame_component frameheader[3]; /* Component descriptor */
    struct scan_component scanheader[3]; /* currently not used */

    int mcu_membership[6]; /* info per block */
    int tab_membership[6];
};


/* possible return flags for process_markers() */
#define HUFFTAB   0x0001 /* with huffman table */
#define QUANTTAB  0x0002 /* with quantization table */
#define APP0_JFIF 0x0004 /* with APP0 segment following JFIF standard */
#define FILL_FF   0x0008 /* with 0xFF padding bytes at begin/end */
#define SOF0      0x0010 /* with SOF0-Segment */
#define DHT       0x0020 /* with Definition of huffman tables */
#define SOS       0x0040 /* with Start-of-Scan segment */
#define DQT       0x0080 /* with definition of quantization table */

/* Preprocess the JPEG JFIF file */
int process_markers(unsigned char* p_bytes, long size, struct jpeg* p_jpeg)
{
    unsigned char* p_src = p_bytes;
    /* write without markers nor stuffing in same buffer */
    unsigned char* p_dest;
    int marker_size; /* variable length of marker segment */
    int i, j, n;
    int ret = 0; /* returned flags */

    while (p_src < p_bytes + size)
    {
        if (*p_src++ != 0xFF) /* no marker? */
        {
            p_src--; /* it's image data, put it back */
            break; /* exit marker processing */
        }

        switch (*p_src++)
        {
        case 0xFF: /* Fill byte */
            ret |= FILL_FF;
        case 0x00: /* Zero stuffed byte - entropy data */
            p_src--; /* put it back */
            continue;

        case 0xC0: /* SOF Huff  - Baseline DCT */
            {
                ret |= SOF0;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                n = *p_src++; /* sample precision (= 8 or 12) */
                if (n != 8)
                {
                    return(-1); /* Unsupported sample precision */
                }
                p_jpeg->y_size = *p_src++ << 8; /* Highbyte */
                p_jpeg->y_size |= *p_src++; /* Lowbyte */
                p_jpeg->x_size = *p_src++ << 8; /* Highbyte */
                p_jpeg->x_size |= *p_src++; /* Lowbyte */

                n = (marker_size-2-6)/3;
                if (*p_src++ != n || (n != 1 && n != 3))
                {
                    return(-2); /* Unsupported SOF0 component specification */
                }
                for (i=0; i<n; i++)
                {
                    p_jpeg->frameheader[i].ID = *p_src++; /* Component info */
                    p_jpeg->frameheader[i].horizontal_sampling = *p_src >> 4;
                    p_jpeg->frameheader[i].vertical_sampling = *p_src++ & 0x0F;
                    p_jpeg->frameheader[i].quanttable_select = *p_src++;
                }

                /* assignments for the decoding of blocks */
                if (p_jpeg->frameheader[0].horizontal_sampling == 2
                    && p_jpeg->frameheader[0].vertical_sampling == 1)
                {   /* 4:2:2 */
                    p_jpeg->blocks = 4;
                    p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
                    p_jpeg->x_phys = p_jpeg->x_mbl * 16;
                    p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
                    p_jpeg->y_phys = p_jpeg->y_mbl * 8;
                    p_jpeg->mcu_membership[0] = 0; /* Y1=Y2=0, U=2, V=3 */
                    p_jpeg->mcu_membership[1] = 0;
                    p_jpeg->mcu_membership[2] = 2;
                    p_jpeg->mcu_membership[3] = 3;
                    p_jpeg->tab_membership[0] = 0; /* DC, DC, AC, AC */
                    p_jpeg->tab_membership[1] = 0;
                    p_jpeg->tab_membership[2] = 1;
                    p_jpeg->tab_membership[3] = 1;
                }
                else if (p_jpeg->frameheader[0].horizontal_sampling == 2
                    && p_jpeg->frameheader[0].vertical_sampling == 2)
                {   /* 4:2:0 */
                    p_jpeg->blocks = 6;
                    p_jpeg->x_mbl = (p_jpeg->x_size+15) / 16;
                    p_jpeg->x_phys = p_jpeg->x_mbl * 16;
                    p_jpeg->y_mbl = (p_jpeg->y_size+15) / 16;
                    p_jpeg->y_phys = p_jpeg->y_mbl * 16;
                    p_jpeg->mcu_membership[0] = 0;
                    p_jpeg->mcu_membership[1] = 0;
                    p_jpeg->mcu_membership[2] = 0;
                    p_jpeg->mcu_membership[3] = 0;
                    p_jpeg->mcu_membership[4] = 2;
                    p_jpeg->mcu_membership[5] = 3;
                    p_jpeg->tab_membership[0] = 0;
                    p_jpeg->tab_membership[1] = 0;
                    p_jpeg->tab_membership[2] = 0;
                    p_jpeg->tab_membership[3] = 0;
                    p_jpeg->tab_membership[4] = 1;
                    p_jpeg->tab_membership[5] = 1;
                }
                else if (p_jpeg->frameheader[0].horizontal_sampling == 1
                    && p_jpeg->frameheader[0].vertical_sampling == 1)
                {   /* 4:4:4 */
                    p_jpeg->blocks = n;
                    p_jpeg->x_mbl = (p_jpeg->x_size+7) / 8;
                    p_jpeg->x_phys = p_jpeg->x_mbl * 8;
                    p_jpeg->y_mbl = (p_jpeg->y_size+7) / 8;
                    p_jpeg->y_phys = p_jpeg->y_mbl * 8;
                    p_jpeg->mcu_membership[0] = 0;
                    p_jpeg->mcu_membership[1] = 2;
                    p_jpeg->mcu_membership[2] = 3;
                    p_jpeg->tab_membership[0] = 0;
                    p_jpeg->tab_membership[1] = 1;
                    p_jpeg->tab_membership[2] = 1;
                }
                else
                {
                    return(-3); /* Unsupported SOF0 subsampling */
                }

            }
            break;

        case 0xC1: /* SOF Huff  - Extended sequential DCT*/
        case 0xC2: /* SOF Huff  - Progressive DCT*/
        case 0xC3: /* SOF Huff  - Spatial (sequential) lossless*/
        case 0xC5: /* SOF Huff  - Differential sequential DCT*/
        case 0xC6: /* SOF Huff  - Differential progressive DCT*/
        case 0xC7: /* SOF Huff  - Differential spatial*/
        case 0xC8: /* SOF Arith - Reserved for JPEG extensions*/
        case 0xC9: /* SOF Arith - Extended sequential DCT*/
        case 0xCA: /* SOF Arith - Progressive DCT*/
        case 0xCB: /* SOF Arith - Spatial (sequential) lossless*/
        case 0xCD: /* SOF Arith - Differential sequential DCT*/
        case 0xCE: /* SOF Arith - Differential progressive DCT*/
        case 0xCF: /* SOF Arith - Differential spatial*/
            {
                return (-4); /* other DCT model than baseline not implemented */
            }

        case 0xC4: /* Define Huffman Table(s) */
            {
                unsigned char* p_temp;

                ret |= DHT;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */

                p_temp = p_src;
                while (p_src < p_temp+marker_size-2-17) /* another table */
                {
                    int sum = 0;
                    i = *p_src & 0x0F; /* table index */
                    if (i > 1)
                    {
                        return (-5); /* Huffman table index out of range */
                    }
                    else if (*p_src++ & 0xF0) /* AC table */
                    {
                        for (j=0; j<16; j++)
                        {
                            sum += *p_src;
                            p_jpeg->hufftable[i].huffmancodes_ac[j] = *p_src++;
                        }
                        if(16 + sum > AC_LEN)
                            return -10; /* longer than allowed */

                        for (; j < 16 + sum; j++)
                            p_jpeg->hufftable[i].huffmancodes_ac[j] = *p_src++;
                    }
                    else /* DC table */
                    {
                        for (j=0; j<16; j++)
                        {
                            sum += *p_src;
                            p_jpeg->hufftable[i].huffmancodes_dc[j] = *p_src++;
                        }
                        if(16 + sum > DC_LEN)
                            return -11; /* longer than allowed */

                        for (; j < 16 + sum; j++)
                            p_jpeg->hufftable[i].huffmancodes_dc[j] = *p_src++;
                    }
                } /* while */
                p_src = p_temp+marker_size - 2; // skip possible residue
            }
            break;

        case 0xCC: /* Define Arithmetic coding conditioning(s) */
            return(-6); /* Arithmetic coding not supported */

        case 0xD0: /* Restart with modulo 8 count 0 */
        case 0xD1: /* Restart with modulo 8 count 1 */
        case 0xD2: /* Restart with modulo 8 count 2 */
        case 0xD3: /* Restart with modulo 8 count 3 */
        case 0xD4: /* Restart with modulo 8 count 4 */
        case 0xD5: /* Restart with modulo 8 count 5 */
        case 0xD6: /* Restart with modulo 8 count 6 */
        case 0xD7: /* Restart with modulo 8 count 7 */
        case 0xD8: /* Start of Image */
        case 0xD9: /* End of Image */
        case 0x01: /* for temp private use arith code */
            break; /* skip parameterless marker */


        case 0xDA: /* Start of Scan */
            {
                ret |= SOS;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */

                n = (marker_size-2-1-3)/2;
                if (*p_src++ != n || (n != 1 && n != 3))
                {
                    return (-7); /* Unsupported SOS component specification */
                }
                for (i=0; i<n; i++)
                {
                    p_jpeg->scanheader[i].ID = *p_src++;
                    p_jpeg->scanheader[i].DC_select = *p_src >> 4;
                    p_jpeg->scanheader[i].AC_select = *p_src++ & 0x0F;
                }
                p_src += 3; /* skip spectral information */
            }
            break;

        case 0xDB: /* Define quantization Table(s) */
            {
                ret |= DQT;
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                n = (marker_size-2)/(QUANT_TABLE_LENGTH+1); /* # of tables */
                for (i=0; i<n; i++)
                {
                    int id = *p_src++; /* ID */
                    if (id >= 4)
                    {
                        return (-8); /* Unsupported quantization table */
                    }
                    /* Read Quantisation table: */
                    for (j=0; j<QUANT_TABLE_LENGTH; j++)
                        p_jpeg->quanttable[id][j] = *p_src++;
                }
            }
            break;

        case 0xDC: /* Define Number of Lines */
        case 0xDD: /* Define Restart Interval */
        case 0xDE: /* Define Hierarchical progression */
        case 0xDF: /* Expand Reference Component(s) */
        case 0xE0: /* Application Field 0*/
        case 0xE1: /* Application Field 1*/
        case 0xE2: /* Application Field 2*/
        case 0xE3: /* Application Field 3*/
        case 0xE4: /* Application Field 4*/
        case 0xE5: /* Application Field 5*/
        case 0xE6: /* Application Field 6*/
        case 0xE7: /* Application Field 7*/
        case 0xE8: /* Application Field 8*/
        case 0xE9: /* Application Field 9*/
        case 0xEA: /* Application Field 10*/
        case 0xEB: /* Application Field 11*/
        case 0xEC: /* Application Field 12*/
        case 0xED: /* Application Field 13*/
        case 0xEE: /* Application Field 14*/
        case 0xEF: /* Application Field 15*/
        case 0xFE: /* Comment */
            {
                marker_size = *p_src++ << 8; /* Highbyte */
                marker_size |= *p_src++; /* Lowbyte */
                p_src += marker_size-2; /* skip segment */
            }
            break;

        case 0xF0: /* Reserved for JPEG extensions */
        case 0xF1: /* Reserved for JPEG extensions */
        case 0xF2: /* Reserved for JPEG extensions */
        case 0xF3: /* Reserved for JPEG extensions */
        case 0xF4: /* Reserved for JPEG extensions */
        case 0xF5: /* Reserved for JPEG extensions */
        case 0xF6: /* Reserved for JPEG extensions */
        case 0xF7: /* Reserved for JPEG extensions */
        case 0xF8: /* Reserved for JPEG extensions */
        case 0xF9: /* Reserved for JPEG extensions */
        case 0xFA: /* Reserved for JPEG extensions */
        case 0xFB: /* Reserved for JPEG extensions */
        case 0xFC: /* Reserved for JPEG extensions */
        case 0xFD: /* Reserved for JPEG extensions */
        case 0x02: /* Reserved */
        default:
            return (-9); /* Unknown marker */
        } /* switch */
    } /* while */


    /* memory location for later decompress (16-bit aligned) */
    p_dest = (unsigned char*)(((int)p_bytes + 1) & ~1);
    p_jpeg->p_entropy_data = (unsigned short*)p_dest;

    
    /* remove byte stuffing and restart markers, if present */
    while (p_src < p_bytes + size)
    {
        if ((*p_dest++ = *p_src++) != 0xFF)
            continue;

        /* 0xFF marker found, have a closer look at the next byte */

        if (*p_src == 0x00)
        {
            p_src++; /* continue reading after marker */
            continue; /* stuffing byte, a true 0xFF */
        }
        else if (*p_src >= 0xD0 && *p_src <= 0xD7) /* restart marker */
        {
            return (-12); /* can't decode such images for now */
            /* below won't work, is not seamless to the huffman decoder */
            p_src++; /* continue reading after marker */
            p_dest--; /* roll back, don't copy it */
            continue; /* ignore */
        }
        else
            break; /* exit on any other marker */
    }
    MEMSET(p_dest, 0, size - (p_dest - p_bytes)); /* fill tail with zeros */
    p_jpeg->words_in_buffer = (p_dest - p_bytes) / sizeof(unsigned short);
    return (ret); /* return flags with seen markers */
}


void default_huff_tbl(struct jpeg* p_jpeg)
{
    static const struct huffman_table luma_table = 
    {
        {
            0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
        },
        {
            0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,
            0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
            0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
            0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
            0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
            0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
            0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
            0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
            0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
            0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
            0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
            0xF9,0xFA
        }
    };

    static const struct huffman_table chroma_table = 
    {
        {
            0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
            0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B
        },
        {
            0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,
            0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
            0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,
            0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,
            0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
            0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
            0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
            0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
            0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
            0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
            0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
            0xF9,0xFA
        }
    };

    p_jpeg->hufftable[0] = luma_table;
    p_jpeg->hufftable[1] = chroma_table;

    return;
}

/* Compute the derived values for a Huffman table */
void fix_huff_tbl(int* htbl, struct derived_tbl* dtbl)
{
    int p, i, l, si;
    int lookbits, ctr;
    char huffsize[257];
    unsigned int huffcode[257];
    unsigned int code;

    dtbl->pub = htbl; /* fill in back link */

    /* Figure C.1: make table of Huffman code length for each symbol */
    /* Note that this is in code-length order. */

    p = 0;
    for (l = 1; l <= 16; l++)
    {    /* all possible code length */
        for (i = 1; i <= (int) htbl[l-1]; i++)  /* all codes per length */
            huffsize[p++] = (char) l;
    }
    huffsize[p] = 0;

    /* Figure C.2: generate the codes themselves */
    /* Note that this is in code-length order. */

    code = 0;
    si = huffsize[0];
    p = 0;
    while (huffsize[p])
    {
        while (((int) huffsize[p]) == si)
        {
            huffcode[p++] = code;
            code++;
        }
        code <<= 1;
        si++;
    }

    /* Figure F.15: generate decoding tables for bit-sequential decoding */

    p = 0;
    for (l = 1; l <= 16; l++)
    {
        if (htbl[l-1])
        {
            dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
            dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
            p += htbl[l-1];
            dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
        }
        else
        {
            dtbl->maxcode[l] = -1;  /* -1 if no codes of this length */
        }
    }
    dtbl->maxcode[17] = 0xFFFFFL; /* ensures huff_DECODE terminates */

    /* Compute lookahead tables to speed up decoding.
    * First we set all the table entries to 0, indicating "too long";
    * then we iterate through the Huffman codes that are short enough and
    * fill in all the entries that correspond to bit sequences starting
    * with that code.
    */

    MEMSET(dtbl->look_nbits, 0, sizeof(dtbl->look_nbits));

    p = 0;
    for (l = 1; l <= HUFF_LOOKAHEAD; l++)
    {
        for (i = 1; i <= (int) htbl[l-1]; i++, p++)
        {
            /* l = current code's length, p = its index in huffcode[] & huffval[]. */
            /* Generate left-justified code followed by all possible bit sequences */
            lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
            for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--)
            {
                dtbl->look_nbits[lookbits] = l;
                dtbl->look_sym[lookbits] = htbl[16+p];
                lookbits++;
            }
        }
    }
}


/* zag[i] is the natural-order position of the i'th element of zigzag order.
 * If the incoming data is corrupted, decode_mcu could attempt to
 * reference values beyond the end of the array.  To avoid a wild store,
 * we put some extra zeroes after the real entries.
 */
static const int zag[] = 
{
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
     0,  0,  0,  0,  0,  0,  0,  0, /* extra entries in case k>63 below */
     0,  0,  0,  0,  0,  0,  0,  0
};

void build_lut(struct jpeg* p_jpeg)
{
    int i;
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_dc, 
        &p_jpeg->dc_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[0].huffmancodes_ac, 
        &p_jpeg->ac_derived_tbls[0]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_dc, 
        &p_jpeg->dc_derived_tbls[1]);
    fix_huff_tbl(p_jpeg->hufftable[1].huffmancodes_ac, 
        &p_jpeg->ac_derived_tbls[1]);

    /* build the dequantization tables for the IDCT (De-ZiZagged) */
    for (i=0; i<64; i++)
    {
        p_jpeg->qt_idct[0][zag[i]] = p_jpeg->quanttable[0][i];
        p_jpeg->qt_idct[1][zag[i]] = p_jpeg->quanttable[1][i];
    }
}


/*
* These functions/macros provide the in-line portion of bit fetching.
* Use check_bit_buffer to ensure there are N bits in get_buffer
* before using get_bits, peek_bits, or drop_bits.
*  check_bit_buffer(state,n,action);
*    Ensure there are N bits in get_buffer; if suspend, take action.
*  val = get_bits(n);
*    Fetch next N bits.
*  val = peek_bits(n);
*    Fetch next N bits without removing them from the buffer.
*  drop_bits(n);
*    Discard next N bits.
* The value N should be a simple variable, not an expression, because it
* is evaluated multiple times.
*/

INLINE void check_bit_buffer(struct bitstream* pb, int nbits)
{
    if (pb->bits_left < nbits)
    {
        pb->words_left--;
        pb->get_buffer = (pb->get_buffer << 16)
                       | ENDIAN_SWAP16(*pb->next_input_word++);
        pb->bits_left += 16;
    }
}

INLINE int get_bits(struct bitstream* pb, int nbits)
{
    return ((int) (pb->get_buffer >> (pb->bits_left -= nbits))) & ((1<<nbits)-1);
}

INLINE int peek_bits(struct bitstream* pb, int nbits)
{
    return ((int) (pb->get_buffer >> (pb->bits_left - nbits))) & ((1<<nbits)-1);
}

INLINE void drop_bits(struct bitstream* pb, int nbits)
{
    pb->bits_left -= nbits;
}

/* Figure F.12: extend sign bit. */
#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
{ 
    0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 
};

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
{ 
    0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 
};

/* Decode a single value */
INLINE int huff_decode_dc(struct bitstream* bs, struct derived_tbl* tbl)
{
    int nb, look, s, r;

    check_bit_buffer(bs, HUFF_LOOKAHEAD);
    look = peek_bits(bs, HUFF_LOOKAHEAD);
    if ((nb = tbl->look_nbits[look]) != 0)
    {
        drop_bits(bs, nb);
        s = tbl->look_sym[look];
        check_bit_buffer(bs, s);
        r = get_bits(bs, s);
        s = HUFF_EXTEND(r, s);
    }
    else
    {   /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */
        long code;
        nb=HUFF_LOOKAHEAD+1;
        check_bit_buffer(bs, nb);
        code = get_bits(bs, nb);
        while (code > tbl->maxcode[nb])
        {
            code <<= 1;
            check_bit_buffer(bs, 1);
            code |= get_bits(bs, 1);
            nb++;
        }
        if (nb > 16) /* error in Huffman */
        {
            s=0; /* fake a zero, this is most safe */
        }
        else
        {
            s = tbl->pub[16 + tbl->valptr[nb] + ((int) (code - tbl->mincode[nb])) ];
            check_bit_buffer(bs, s);
            r = get_bits(bs, s);
            s = HUFF_EXTEND(r, s);
        }
    } /* end slow decode */
    return s;
}

INLINE int huff_decode_ac(struct bitstream* bs, struct derived_tbl* tbl)
{
    int nb, look, s;

    check_bit_buffer(bs, HUFF_LOOKAHEAD);
    look = peek_bits(bs, HUFF_LOOKAHEAD);
    if ((nb = tbl->look_nbits[look]) != 0)
    {
        drop_bits(bs, nb);
        s = tbl->look_sym[look];
    }
    else
    {   /*  slow_DECODE(s, HUFF_LOOKAHEAD+1)) < 0); */
        long code;
        nb=HUFF_LOOKAHEAD+1;
        check_bit_buffer(bs, nb);
        code = get_bits(bs, nb);
        while (code > tbl->maxcode[nb])
        {
            code <<= 1;
            check_bit_buffer(bs, 1);
            code |= get_bits(bs, 1);
            nb++;
        }
        if (nb > 16) /* error in Huffman */
        {
            s=0; /* fake a zero, this is most safe */
        }
        else
        {
            s = tbl->pub[16 + tbl->valptr[nb] + ((int) (code - tbl->mincode[nb])) ];
        }
    } /* end slow decode */
    return s;
}


/* a JPEG decoder specialized in decoding only the luminance (b&w) */
int jpeg_decode(struct jpeg* p_jpeg, unsigned char* p_pixel, int downscale, 
                void (*pf_progress)(int current, int total))
{
    struct bitstream bs; /* bitstream "object" */
    static int block[64]; /* decoded DCT coefficients */

    int width, height;
    int skip_line; /* bytes from one line to the next (skip_line) */
    int skip_strip, skip_mcu; /* bytes to next DCT row / column */

    int x, y; /* loop counter */

    unsigned char* p_byte; /* bitmap pointer */

    void (*pf_idct)(unsigned char*, int*, int*, int); /* selected IDCT */
    int k_need; /* AC coefficients needed up to here */
    int zero_need; /* init the block with this many zeros */

    int last_dc_val = 0;
    int store_offs[4]; /* memory offsets: order of Y11 Y12 Y21 Y22 U V */

    /* pick the IDCT we want, determine how to work with coefs */
    if (downscale == 1)
    {
        pf_idct = idct8x8;
        k_need = 64; /* all */
        zero_need = 63; /* all */
    }
    else if (downscale == 2)
    {
        pf_idct = idct4x4;
        k_need = 25; /* this far in zig-zag to cover 4*4 */
        zero_need = 27; /* clear this far in linear order */
    }
    else if (downscale == 4)
    {
        pf_idct = idct2x2;
        k_need = 5; /* this far in zig-zag to cover 2*2 */
        zero_need = 9; /* clear this far in linear order */
    }
    else if (downscale == 8)
    {
        pf_idct = idct1x1;
        k_need = 0; /* no AC, not needed */
        zero_need = 0; /* no AC, not needed */
    }
    else return -1; /* not supported */

    /* init bitstream */
    bs.bits_left = 0;
    bs.next_input_word = p_jpeg->p_entropy_data;
    bs.words_left = p_jpeg->words_in_buffer;

    width  = p_jpeg->x_phys / downscale;
    height = p_jpeg->y_phys / downscale;
    skip_line = width;
    skip_strip = skip_line * (height / p_jpeg->y_mbl);
    skip_mcu = (width/p_jpeg->x_mbl);

    /* prepare offsets about where to store the different blocks */
    store_offs[0] = 0;
    store_offs[1] = 8 / downscale; /* to the right */
    store_offs[2] = width * 8 / downscale; /* below */
    store_offs[3] = store_offs[1] + store_offs[2]; /* right+below */

    for(y=0; y<p_jpeg->y_mbl; y++)
    {
        p_byte = p_pixel;
        p_pixel += skip_strip;
        for (x=0; x<p_jpeg->x_mbl; x++)
        {
            int blkn;

            /* Outer loop handles each block in the MCU */

            for (blkn = 0; blkn < p_jpeg->blocks && bs.words_left>=0; blkn++)
            {   /* Decode a single block's worth of coefficients */
                int k = 1; /* coefficient index */
                int s, r; /* huffman values */
                int ci = p_jpeg->mcu_membership[blkn]; /* component index */
                int ti = p_jpeg->tab_membership[blkn]; /* table index */
                struct derived_tbl* dctbl = &p_jpeg->dc_derived_tbls[ti];
                struct derived_tbl* actbl = &p_jpeg->ac_derived_tbls[ti];

                /* Section F.2.2.1: decode the DC coefficient difference */
                s = huff_decode_dc(&bs, dctbl);

                if (ci == 0) /* only for Y component */
                {
                    last_dc_val += s;
                    block[0] = last_dc_val; /* output it (assumes zag[0] = 0) */

                    /* coefficient buffer must be cleared */
                    MEMSET(block+1, 0, zero_need*sizeof(int));

                    /* Section F.2.2.2: decode the AC coefficients */
                    for (; k < k_need; k++)
                    {
                        s = huff_decode_ac(&bs, actbl);
                        r = s >> 4;
                        s &= 15;

                        if (s)
                        {
                            k += r;
                            check_bit_buffer(&bs, s);
                            r = get_bits(&bs, s);
                            block[zag[k]] = HUFF_EXTEND(r, s);
                        }
                        else
                        {
                            if (r != 15)
                            {
                                k = 64;
                                break;
                            }
                            k += r;
                        }
                    }  /* for k */
                }
                /* In this path we just discard the values */
                for (; k < 64; k++)
                {
                    s = huff_decode_ac(&bs, actbl);
                    r = s >> 4;
                    s &= 15;

                    if (s)
                    {
                        k += r;
                        check_bit_buffer(&bs, s);
                        drop_bits(&bs, s);
                    }
                    else
                    {
                        if (r != 15)
                            break;
                        k += r;
                    }
                }  /* for k */

                if (ci == 0)
                {   /* only for Y component */
                    pf_idct(p_byte+store_offs[blkn], block, p_jpeg->qt_idct[ti], 
                        skip_line);
                }
            } /* for blkn */
            p_byte += skip_mcu;
        } /* for x */
        if (pf_progress != NULL)
            pf_progress(y, p_jpeg->y_mbl-1); /* notify about decoding progress */
    } /* for y */

    return 0; /* success */
}

/**************** end JPEG code ********************/



/**************** begin Application ********************/


/************************* Types ***************************/

struct t_disp
{
    unsigned char* bitmap;
    int width;
    int height;
    int stride;
    int x, y;
};


/************************* Globals ***************************/

/* decompressed image in the possible sizes (1,2,4,8), wasting the other */
struct t_disp disp[9]; 

/* my memory pool (from the mp3 buffer) */
char print[32]; /* use a common snprintf() buffer */
unsigned char* buf; /* up to here currently used by image(s) */
int buf_size;
unsigned char* buf_root; /* the root of the images */
int root_size;

/************************* Implementation ***************************/

#define ZOOM_IN  100 // return codes for below function
#define ZOOM_OUT 101

/* interactively scroll around the image */
int scroll_bmp(struct t_disp* pdisp)
{
    while (true)
    {
        int button;
        int move;

        /* we're unfortunately slower than key repeat,
           so empty the button queue, to avoid post-scroll */
        while(rb->button_get(false) != BUTTON_NONE);
        
        button = rb->button_get(true);

        if (button == SYS_USB_CONNECTED)
        {
            gray_release_buffer(); /* switch off overlay and deinitialize */
            return PLUGIN_USB_CONNECTED;
        }

        switch(button & ~(BUTTON_REPEAT))
        {
        case BUTTON_LEFT:
            move = MIN(10, pdisp->x);
            if (move > 0)
            {
                gray_scroll_right(move, false); /* scroll right */
                pdisp->x -= move;
                gray_drawgraymap(
                    pdisp->bitmap + pdisp->y * pdisp->stride + pdisp->x,
                    0, MAX(0, (LCD_HEIGHT-pdisp->height)/2), // x, y
                    move, MIN(LCD_HEIGHT, pdisp->height), // w, h
                    pdisp->stride);
            }
            break;

        case BUTTON_RIGHT:
            move = MIN(10, pdisp->width - pdisp->x - LCD_WIDTH);
            if (move > 0)
            {
                gray_scroll_left(move, false); /* scroll left */
                pdisp->x += move;
                gray_drawgraymap(
                    pdisp->bitmap + pdisp->y * pdisp->stride + pdisp->x + LCD_WIDTH - move,
                    LCD_WIDTH - move, MAX(0, (LCD_HEIGHT-pdisp->height)/2), /* x, y */
                    move, MIN(LCD_HEIGHT, pdisp->height), /* w, h */
                    pdisp->stride);
            }
            break;

        case BUTTON_UP:
            move = MIN(8, pdisp->y);
            if (move > 0)
            {
                if (move == 8)
                    gray_scroll_down8(false); /* scroll down by 8 pixel */
                else
                    gray_scroll_down(move, false); /* scroll down 1..7 pixel */
                pdisp->y -= move;
                gray_drawgraymap(
                    pdisp->bitmap + pdisp->y * pdisp->stride + pdisp->x,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), 0, /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move, /* w, h */
                    pdisp->stride);
            }
            break;

        case BUTTON_DOWN:
            move = MIN(8, pdisp->height - pdisp->y - LCD_HEIGHT);
            if (move > 0)
            {
                if (move == 8)
                    gray_scroll_up8(false); /* scroll up by 8 pixel */
                else
                    gray_scroll_up(move, false); /* scroll up 1..7 pixel */
                pdisp->y += move;
                gray_drawgraymap(
                    pdisp->bitmap + (pdisp->y + LCD_HEIGHT - move) * pdisp->stride + pdisp->x,
                    MAX(0, (LCD_WIDTH-pdisp->width)/2), LCD_HEIGHT - move, /* x, y */
                    MIN(LCD_WIDTH, pdisp->width), move, /* w, h */
                    pdisp->stride);
            }
            break;

        case BUTTON_PLAY:
            return ZOOM_IN;
            break;

        case BUTTON_ON:
            return ZOOM_OUT;
            break;

        case BUTTON_OFF:
            return PLUGIN_OK;
        } /* switch */
    } /* while (true) */
}

/********************* main function *************************/

/* debug function */
int wait_for_button(void)
{
    int button;
    
    do
    {
        button = rb->button_get(true);
    } while ((button & BUTTON_REL) && button != SYS_USB_CONNECTED);
    
    return button;
}

/* callback updating a progress meter while JPEG decoding */
void cb_progess(int current, int total)
{
    rb->yield(); /* be nice to the other threads */
    rb->progressbar(0, LCD_HEIGHT-8, LCD_WIDTH, 8, 
        current*100/total, 0 /*Grow_Right*/);
    rb->lcd_update_rect(0, LCD_HEIGHT-8, LCD_WIDTH, 8);
}

/* helper to align a buffer to a given power of two */
void align(unsigned char** ppbuf, int* plen, int align)
{
    unsigned int orig = (unsigned int)*ppbuf;
    unsigned int aligned = (orig + (align-1)) & ~(align-1);

    *plen -= aligned - orig;
    *ppbuf = (unsigned char*)aligned;
}


/* how far can we zoom in without running out of memory */
int min_downscale(int x, int y, int bufsize)
{
    int downscale = 8;

    if ((x/8) * (y/8) > bufsize)
        return 0; /* error, too large, even 1:8 doesn't fit */

    while ((x*2/downscale) * (y*2/downscale) < bufsize
        && downscale > 1)
    {
        downscale /= 2;
    }
    return downscale;
}


/* how far can we zoom out, to fit image into the LCD */
int max_downscale(int x, int y)
{
    int downscale = 1;

    while ((x/downscale > LCD_WIDTH || y/downscale > LCD_HEIGHT)
        && downscale < 8)
    {
        downscale *= 2;
    }

    return downscale;
}


/* return decoded or cached image */
struct t_disp* get_image(struct jpeg* p_jpg, int ds)
{
    int w, h; /* used to center output */
    int size; /* decompressed image size */
    long time; /* measured ticks */
    int status;

    struct t_disp* p_disp = &disp[ds]; /* short cut */

    if (p_disp->bitmap != NULL)
    {
        return p_disp; /* we still have it */
    }

    /* assign image buffer */

     /* physical size needed for decoding */
    size = (p_jpg->x_phys/ds) * (p_jpg->y_phys / ds);
    if (buf_size <= size)
    {   /* have to discard the current */
        int i;
        for (i=1; i<=8; i++)
            disp[i].bitmap = NULL; /* invalidate all bitmaps */
        buf = buf_root; /* start again from the beginning of the buffer */
        buf_size = root_size;
    }

    /* size may be less when decoded (if height is not block aligned) */
    size = (p_jpg->x_phys/ds) * (p_jpg->y_size / ds);
    p_disp->bitmap = buf;
    buf += size;
    buf_size -= size;
    
    rb->snprintf(print, sizeof(print), "decoding %d*%d", 
        p_jpg->x_size/ds, p_jpg->y_size/ds);
    rb->lcd_puts(0, 3, print);
    rb->lcd_update();

    /* update image properties */
    p_disp->width = p_jpg->x_size/ds;
    p_disp->stride = p_jpg->x_phys / ds; /* use physical size for stride */
    p_disp->height = p_jpg->y_size/ds;

    /* the actual decoding */
    time = *rb->current_tick;
    status = jpeg_decode(p_jpg, p_disp->bitmap, ds, cb_progess);
    if (status)
    {
        rb->splash(HZ*2, true, "decode error %d", status);
        return NULL;
    }
    time = *rb->current_tick - time;
    rb->snprintf(print, sizeof(print), " %d.%02d sec ", time/HZ, time%HZ);
    rb->lcd_getstringsize(print, &w, &h); /* centered in progress bar */
    rb->lcd_putsxy((LCD_WIDTH - w)/2, LCD_HEIGHT - h, print);
    rb->lcd_update();

    return p_disp;
}


/* set the view to the given center point, limit if necessary */
void set_view (struct t_disp* p_disp, int cx, int cy)
{
    int x, y;

    /* plain center to available width/height */
    x = cx - MIN(LCD_WIDTH, p_disp->width) / 2;
    y = cy - MIN(LCD_HEIGHT, p_disp->height) / 2;

    /* limit against upper image size */
    x = MIN(p_disp->width - LCD_WIDTH, x);
    y = MIN(p_disp->height - LCD_HEIGHT, y);

    /* limit against negative side */
    x = MAX(0, x);
    y = MAX(0, y);

    p_disp->x = x; /* set the values */
    p_disp->y = y;
}


/* calculate the view center based on the bitmap position */
void get_view(struct t_disp* p_disp, int* p_cx, int* p_cy)
{
    *p_cx = p_disp->x + MIN(LCD_WIDTH, p_disp->width) / 2;
    *p_cy = p_disp->y + MIN(LCD_HEIGHT, p_disp->height) / 2;
}


/* load, decode, display the image */
int main(char* filename) 
{
    int fd;
    int filesize;
    int grayscales;
    int graysize; // helper
    unsigned char* buf_jpeg; /* compressed JPEG image */
    static struct jpeg jpg; /* too large for stack */
    int status;
    int ds, ds_min, ds_max; /* scaling and limits */
    struct t_disp* p_disp; /* currenly displayed image */
    int cx, cy; /* view center */

    fd = rb->open(filename, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(HZ*2, true, "fopen err");
        return PLUGIN_ERROR;
    }
    filesize = rb->filesize(fd);

    rb->memset(&disp, 0, sizeof(disp));

    buf = rb->plugin_get_mp3_buffer(&buf_size); /* start munching memory */


    /* initialize the grayscale buffer:
     * 112 pixels wide, 8 rows (64 pixels) high, (try to) reserve
     * 32 bitplanes for 33 shades of gray. (uses 28856 bytes)*/
    align(&buf, &buf_size, 4); // 32 bit align
    graysize = sizeof(tGraybuf) + sizeof(long) + (112 * 8 + sizeof(long)) * 32;
    grayscales = gray_init_buffer(buf, graysize, 112, 8, 32) + 1;
    buf += graysize;
    buf_size -= graysize;
    if (grayscales < 33 || buf_size <= 0)
    {
        rb->splash(HZ*2, true, "gray buf error");
        return PLUGIN_ERROR;
    }


    /* allocate JPEG buffer */
    align(&buf, &buf_size, 2); /* 16 bit align */
    buf_jpeg = (unsigned char*)(((int)buf + 1) & ~1);
    buf += filesize;
    buf_size -= filesize;
    if (buf_size <= 0)
    {
        rb->splash(HZ*2, true, "out of memory");
        rb->close(fd);
        return PLUGIN_ERROR;
    }

    rb->snprintf(print, sizeof(print), "loading %d bytes", filesize);
    rb->lcd_puts(0, 0, print);
    rb->lcd_update();
    
    rb->read(fd, buf_jpeg, filesize);
    rb->close(fd);

    rb->snprintf(print, sizeof(print), "decoding markers");
    rb->lcd_puts(0, 1, print);
    rb->lcd_update();
    
    rb->memset(&jpg, 0, sizeof(jpg)); /* clear info struct */
    /* process markers, unstuffing */
    status = process_markers(buf_jpeg, filesize, &jpg);
    if (status < 0 || (status & (DQT | SOF0)) != (DQT | SOF0))
    {   /* bad format or minimum components not contained */
        rb->splash(HZ*2, true, "unsupported %d", status);
        return PLUGIN_ERROR;
    }
    if (!(status & DHT)) /* if no Huffman table present: */
        default_huff_tbl(&jpg); /* use default */
    build_lut(&jpg); /* derive Huffman and other lookup-tables */
    
    /* I can correct the buffer now, re-gain what the removed markers took */
    buf -= filesize; /* back to before */
    buf_size += filesize;
    buf += jpg.words_in_buffer * sizeof(short); /* real space */
    buf_size -= jpg.words_in_buffer * sizeof(short);
    buf_root = buf; /* we can start the images here */
    root_size = buf_size;

    rb->snprintf(print, sizeof(print), "image %d*%d", jpg.x_size, jpg.y_size);
    rb->lcd_puts(0, 2, print);
    rb->lcd_update();

    /* check display constraint */
    ds_max = max_downscale(jpg.x_size, jpg.y_size); 
    /* check memory constraint */
    ds_min = min_downscale(jpg.x_phys, jpg.y_phys, buf_size);
    if (ds_min == 0)
    {
        rb->splash(HZ*2, true, "too large");
        return PLUGIN_ERROR;
    }
    ds = ds_max; /* initials setting */
    cx = jpg.x_size/ds/2; /* center the view */
    cy = jpg.y_size/ds/2;
    
    do  /* loop the image prepare and decoding when zoomed */
    {
        p_disp = get_image(&jpg, ds); /* decode or fetch from cache */
        if (p_disp == NULL)
            return PLUGIN_ERROR;

        set_view(p_disp, cx, cy);

        rb->snprintf(print, sizeof(print), "showing %d*%d", 
            p_disp->width, p_disp->height);
        rb->lcd_puts(0, 3, print);
        rb->lcd_update();

        gray_clear_display();
        gray_drawgraymap(
            p_disp->bitmap + p_disp->y * p_disp->stride + p_disp->x,
            MAX(0, (LCD_WIDTH - p_disp->width) / 2), 
            MAX(0, (LCD_HEIGHT - p_disp->height) / 2), 
            MIN(LCD_WIDTH, p_disp->width), 
            MIN(LCD_HEIGHT, p_disp->height), 
            p_disp->stride);

        gray_show_display(true); /* switch on grayscale overlay */

        /* drawing is now finished, play around with scrolling 
         * until you press OFF or connect USB
         */
        while (1)
        {
            status = scroll_bmp(p_disp);
            if (status == ZOOM_IN)
            {
                if (ds > ds_min)
                {
                    ds /= 2; /* reduce downscaling to zoom in */
                    get_view(p_disp, &cx, &cy);
                    cx *= 2; /* prepare the position in the new image */
                    cy *= 2;
                }
                else
                    continue;
            }

            if (status == ZOOM_OUT)
            {
                if (ds < ds_max)
                {
                    ds *= 2; /* increase downscaling to zoom out */
                    get_view(p_disp, &cx, &cy);
                    cx /= 2; /* prepare the position in the new image */
                    cy /= 2;
                }
                else
                    continue;
            }
            break;
        }

        gray_show_display(false); /* switch off overlay */

    }
    while (status > 0 && status != SYS_USB_CONNECTED);

    gray_release_buffer(); /* deinitialize */

    return status;
}

/******************** Plugin entry point *********************/

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int ret;
    /* this macro should be called as the first thing you do in the plugin.
    it test that the api version and model the plugin was compiled for
    matches the machine it is running on */
    TEST_PLUGIN_API(api);

    rb = api; /* copy to global api pointer */
    
    ret = main((char*)parameter);

    if (ret == PLUGIN_USB_CONNECTED)
        rb->usb_screen();
    return ret;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
#endif /* #ifndef SIMULATOR */


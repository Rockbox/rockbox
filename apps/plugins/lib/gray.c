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

/**** internal core functions and definitions ****/

/* You do not want to touch these if you don't know exactly what you're
 * doing. */

#define GRAY_RUNNING          0x0001  /* grayscale overlay is running */
#define GRAY_DEFERRED_UPDATE  0x0002  /* lcd_update() requested */

/* unsigned 16 bit multiplication (a single instruction on the SH) */
#define MULU16(a, b) ((unsigned long) \
                     (((unsigned short) (a)) * ((unsigned short) (b))))

/* The grayscale buffer management structure */
typedef struct
{
    int x;
    int by;         /* 8-pixel units */
    int width;
    int height;
    int bheight;    /* 8-pixel units */
    int plane_size;
    int depth;      /* number_of_bitplanes  = (number_of_grayscales - 1) */
    int cur_plane;  /* for the timer isr */
    unsigned long randmask;    /* mask for random value in _writepixel() */
    unsigned long flags;       /* various flags, see #defines */
    unsigned long *bitpattern; /* pointer to start of pattern table */
    unsigned char *data;       /* pointer to start of bitplane data */
    unsigned long fg_pattern;  /* current foreground pattern */
    unsigned long bg_pattern;  /* current background pattern */
    int drawmode;              /* current draw mode */
    struct font *curfont;      /* current selected font */
} tGraybuf;

/** globals **/

static struct plugin_api *rb = NULL; /* global api struct pointer */
static tGraybuf *graybuf = NULL;     /* pointer to grayscale buffer */
static short random_buffer;

/** prototypes **/

static void _timer_isr(void);

static void _writepixel(int x, int y, unsigned long pattern);
static void _invertpixel(int x, int y, unsigned long pattern);

static void _writearray(unsigned char *address, unsigned char *src, int stride,
                        unsigned mask);

static void _writeblock(unsigned char *address, unsigned mask, unsigned bits);
static void _invertblock(unsigned char *address, unsigned mask, unsigned bits);
static void _writeblockfg(unsigned char *address, unsigned mask, unsigned bits);
static void _writeblockbg(unsigned char *address, unsigned mask, unsigned bits);

/** function pointer arrays **/

static void (*_pixelfuncs[4])(int x, int y, unsigned long pattern) = {
    _invertpixel, _writepixel, _writepixel, _writepixel
};

static void (*_blockfuncs[4])(unsigned char *address, unsigned mask,
                              unsigned bits) = {
    _invertblock, _writeblockfg, _writeblockbg, _writeblock
};

/** implementation **/

/* timer interrupt handler: display next bitplane */
static void _timer_isr(void)
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
        int y2 = MIN((graybuf->by << 3) + graybuf->height, LCD_HEIGHT);

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

/* Set a pixel to a specific bit pattern (low level routine) */
static void _writepixel(int x, int y, unsigned long pattern)
{
    register unsigned mask, random;
    register unsigned char *address;

    /* Some (pseudo-)random function must be used here to shift the bit
     * pattern randomly, otherwise you would get flicker and/or moire.
     * Since rand() is relatively slow, I've implemented a simple, but very
     * fast pseudo-random generator based on linear congruency in assembler.
     * It delivers max. 16 pseudo-random bits in each iteration. */

    /* simple but fast pseudo-random generator */
    asm (
        "mov     #75,r1          \n"
        "mulu    %1,r1           \n"  /* multiply by 75 */
        "sts     macl,%1         \n"  /* get result */
        "add     #74,%1          \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "swap.b  %1,%0           \n"  /* get bits 8..15 (need max. 5) */
        "and     %2,%0           \n"  /* mask out unneeded bits */
        : /* outputs */
        /* %0 */ "=&r"(random),
        /* %1, in & out */ "+r"(random_buffer)
        : /* inputs */
        /* %2 */ "r"(graybuf->randmask)
        : /* clobbers */
        "r1", "macl"
    );

    /* precalculate mask and byte address in first bitplane */
    asm (
        "mov     %3,%0           \n"  /* take y as base for address offset */
        "shlr2   %0              \n"  /* shift right by 3 (= divide by 8) */
        "shlr    %0              \n"
        "mulu    %0,%2           \n"  /* multiply with width */
        "and     #7,%3           \n"  /* get lower 3 bits of y */
        "sts     macl,%0         \n"  /* get mulu result */
        "add     %4,%0           \n"  /* add base + x to get final address */

        "mov     %3,%1           \n"  /* move lower 3 bits of y out of r0 */
        "mova    .wp_masktable,%3\n"  /* get address of mask table in r0 */
        "bra     .wp_predone     \n"  /* skip the table */
        "mov.b   @(%3,%1),%1     \n"  /* get entry from mask table */

        ".align  2               \n"
    ".wp_masktable:              \n"  /* mask table */
        ".byte   0x01            \n"
        ".byte   0x02            \n"
        ".byte   0x04            \n"
        ".byte   0x08            \n"
        ".byte   0x10            \n"
        ".byte   0x20            \n"
        ".byte   0x40            \n"
        ".byte   0x80            \n"

    ".wp_predone:                \n"
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
    asm volatile (
        "cmp/hs  %1,%5           \n"  /* random >= depth ? */
        "bf      .wp_ntrim       \n"
        "sub     %1,%5           \n"  /* yes: random -= depth */
        /* it's sufficient to do this once, since the mask guarantees
         * random < 2 * depth */
    ".wp_ntrim:                  \n"

        /* calculate some addresses */
        "mulu    %4,%1           \n"  /* end address offset */
        "not     %3,r1           \n"  /* get inverse mask (for "and") */
        "sts     macl,%1         \n"  /* result of mulu */
        "mulu    %4,%5           \n"  /* address offset of <random>'th plane */
        "add     %2,%1           \n"  /* end offset -> end address */
        "sts     macl,%5         \n"  /* result of mulu */
        "add     %2,%5           \n"  /* address of <random>'th plane */
        "bra     .wp_start1      \n"
        "mov     %5,r2           \n"  /* copy address */

        /* first loop: set bits from <random>'th bitplane to last */
    ".wp_loop1:                  \n"
        "mov.b   @r2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "and     r1,r3           \n"  /* reset bit (-> "white") */
        "bf      .wp_white1      \n"  /* t=0? -> "white" bit */
        "or      %3,r3           \n"  /* set bit ("black" bit) */
    ".wp_white1:                 \n"
        "mov.b   r3,@r2          \n"  /* store data byte */
        "add     %4,r2           \n"  /* advance address to next bitplane */
    ".wp_start1:                 \n"
        "cmp/hi  r2,%1           \n"  /* address < end address ? */
        "bt      .wp_loop1       \n"

        "bra     .wp_start2      \n"
        "nop                     \n"

        /* second loop: set bits from first to <random-1>'th bitplane
         * Bit setting works the other way round here to equalize average
         * execution times for bright and dark pixels */
    ".wp_loop2:                  \n"
        "mov.b   @%2,r3          \n"  /* get data byte */
        "shlr    %0              \n"  /* shift bit mask, sets t bit */
        "or      %3,r3           \n"  /* set bit (-> "black") */
        "bt      .wp_black2      \n"  /* t=1? -> "black" bit */
        "and     r1,r3           \n"  /* reset bit ("white" bit) */
    ".wp_black2:                 \n"
        "mov.b   r3,@%2          \n"  /* store data byte */
        "add     %4,%2           \n"  /* advance address to next bitplane */
    ".wp_start2:                 \n"
        "cmp/hi  %2,%5           \n"  /* address < <random>'th address ? */
        "bt      .wp_loop2       \n"
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

/* invert all bits for one pixel (low level routine) */
static void _invertpixel(int x, int y, unsigned long pattern)
{
    register unsigned mask;
    register unsigned char *address;

    (void) pattern; /* not used for invert */

    /* precalculate mask and byte address in first bitplane */
    asm (
        "mov     %3,%0           \n"  /* take y as base for address offset */
        "shlr2   %0              \n"  /* shift right by 3 (= divide by 8) */
        "shlr    %0              \n"
        "mulu    %0,%2           \n"  /* multiply with width */
        "and     #7,%3           \n"  /* get lower 3 bits of y */
        "sts     macl,%0         \n"  /* get mulu result */
        "add     %4,%0           \n"  /* add base + x to get final address */

        "mov     %3,%1           \n"  /* move lower 3 bits of y out of r0 */
        "mova    .ip_masktable,%3\n"  /* get address of mask table in r0 */
        "bra     .ip_predone     \n"  /* skip the table */
        "mov.b   @(%3,%1),%1     \n"  /* get entry from mask table */

        ".align  2               \n"
    ".ip_masktable:              \n"  /* mask table */
        ".byte   0x01            \n"
        ".byte   0x02            \n"
        ".byte   0x04            \n"
        ".byte   0x08            \n"
        ".byte   0x10            \n"
        ".byte   0x20            \n"
        ".byte   0x40            \n"
        ".byte   0x80            \n"

    ".ip_predone:                \n"
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

    /* invert bits in all bitplanes */
    asm volatile (
        "mov     #0,r1           \n"  /* current_plane = 0 */

    ".ip_loop:                   \n"
        "mov.b   @%1,r2          \n"  /* get data byte */
        "add     #1,r1           \n"  /* current_plane++; */
        "xor     %2,r2           \n"  /* invert bits */
        "mov.b   r2,@%1          \n"  /* store data byte */
        "add     %3,%1           \n"  /* advance address to next bitplane */
        "cmp/hi  r1,%0           \n"  /* current_plane < depth ? */
        "bt      .ip_loop        \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(address),
        /* %2 */ "r"(mask),
        /* %3 */ "r"(graybuf->plane_size)
        : /* clobbers */
        "r1", "r2"
    );
}

/* Write an 8-pixel block, defined by their brightnesses in a graymap.
 * Address is the byte in the first bitplane, src is the graymap start address,
 * stride is the increment for the graymap to get to the next pixel, mask
 * determines which pixels of the destination block are changed. For "0" bits,
 * the src address is not incremented! */
static void _writearray(unsigned char *address, unsigned char *src, int stride,
                        unsigned mask)
{
    unsigned long pat_stack[8];
    register unsigned char *end_addr;
    register unsigned long *pat_ptr = &pat_stack[8];

    /* precalculate the bit patterns with random shifts (same RNG as
     * _writepixel, see there for an explanation) for all 8 pixels and put them
     * on an extra "stack" */
    asm (
        "mov     #8,r3           \n"  /* loop count in r3: 8 pixels */
        "mov     %7,r2           \n"  /* copy mask */

    ".wa_loop:                   \n"  /** load pattern for pixel **/
        "mov     #0,r0           \n"  /* pattern for skipped pixel must be 0 */
        "shlr    r2              \n"  /* shift out lsb of mask */
        "bf      .wa_skip        \n"  /* skip this pixel */

        "mov.b   @%2,r0          \n"  /* load src byte */
        "extu.b  r0,r0           \n"  /* extend unsigned */
        "mulu    %4,r0           \n"  /* macl = byte * depth; */
        "add     %3,%2           \n"  /* src += stride; */
        "sts     macl,r4         \n"  /* r4 = macl; */
        "add     r4,r0           \n"  /* byte += r4; */
        "shlr8   r0              \n"  /* byte >>= 8; */
        "shll2   r0              \n"
        "mov.l   @(r0,%5),r4     \n"  /* r4 = bitpattern[byte]; */

        "mov     #75,r0          \n"
        "mulu    r0,%0           \n"  /* multiply by 75 */
        "sts     macl,%0         \n"
        "add     #74,%0          \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "swap.b  %0,r1           \n"  /* get bits 8..15 (need max. 5) */
        "and     %6,r1           \n"  /* mask out unneeded bits */

        "cmp/hs  %4,r1           \n"  /* random >= depth ? */
        "bf      .wa_ntrim       \n"
        "sub     %4,r1           \n"  /* yes: random -= depth; */
    ".wa_ntrim:                  \n"

        "mov.l   .ashlsi3,r0     \n"  /** rotate pattern **/
        "jsr     @r0             \n"  /* r4 -> r0, shift left by r5 */
        "mov     r1,r5           \n"

        "mov     %4,r5           \n"
        "sub     r1,r5           \n"  /* r5 = depth - r1 */
        "mov.l   .lshrsi3,r1     \n"
        "jsr     @r1             \n"  /* r4 -> r0, shift right by r5 */
        "mov     r0,r1           \n"  /* store previous result in r1 */

        "or      r1,r0           \n"  /* rotated_pattern = r0 | r1 */

    ".wa_skip:                   \n"
        "mov.l   r0,@-%1         \n"  /* push on pattern stack */

        "add     #-1,r3          \n"  /* decrease loop count */
        "cmp/pl  r3              \n"  /* loop count > 0? */
        "bt      .wa_loop        \n"  /* yes: loop */
        : /* outputs */
        /* %0, in & out */ "+r"(random_buffer),
        /* %1, in & out */ "+r"(pat_ptr)
        : /* inputs */
        /* %2 */ "r"(src),
        /* %3 */ "r"(stride),
        /* %4 */ "r"(graybuf->depth),
        /* %5 */ "r"(graybuf->bitpattern),
        /* %6 */ "r"(graybuf->randmask),
        /* %7 */ "r"(mask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "macl"
    );

    end_addr = address + MULU16(graybuf->depth, graybuf->plane_size);

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile (
        "mov.l   @%3+,r1         \n"  /* pop all 8 patterns */
        "mov.l   @%3+,r2         \n"
        "mov.l   @%3+,r3         \n"
        "mov.l   @%3+,r8         \n"
        "mov.l   @%3+,r9         \n"
        "mov.l   @%3+,r10        \n"
        "mov.l   @%3+,r11        \n"
        "mov.l   @%3+,r12        \n"

        "not     %4,%4           \n"  /* "set" mask -> "keep" mask */
        "extu.b  %4,%4           \n"  /* mask out high bits */
        "tst     %4,%4           \n"  /* nothing to keep? */
        "bt      .wa_sloop       \n"  /* yes: jump to short loop */

    ".wa_floop:                  \n"  /** full loop (there are bits to keep)**/
        "shlr    r1              \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0              \n"  /* rotate t bit into r0 */
        "shlr    r2              \n"
        "rotcl   r0              \n"
        "shlr    r3              \n"
        "rotcl   r0              \n"
        "shlr    r8              \n"
        "rotcl   r0              \n"
        "shlr    r9              \n"
        "rotcl   r0              \n"
        "shlr    r10             \n"
        "rotcl   r0              \n"
        "shlr    r11             \n"
        "rotcl   r0              \n"
        "shlr    r12             \n"
        "mov.b   @%0,%3          \n"  /* read old value */
        "rotcl   r0              \n"
        "and     %4,%3           \n"  /* mask out unneeded bits */
        "or      r0,%3           \n"  /* set new bits */
        "mov.b   %3,@%0          \n"  /* store value to bitplane */
        "add     %2,%0           \n"  /* advance to next bitplane */
        "cmp/hi  %0,%1           \n"  /* last bitplane done? */
        "bt      .wa_floop       \n"  /* no: loop */

        "bra     .wa_end         \n"
        "nop                     \n"

    ".wa_sloop:                  \n"  /** short loop (nothing to keep) **/
        "shlr    r1              \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0              \n"  /* rotate t bit into r0 */
        "shlr    r2              \n"
        "rotcl   r0              \n"
        "shlr    r3              \n"
        "rotcl   r0              \n"
        "shlr    r8              \n"
        "rotcl   r0              \n"
        "shlr    r9              \n"
        "rotcl   r0              \n"
        "shlr    r10             \n"
        "rotcl   r0              \n"
        "shlr    r11             \n"
        "rotcl   r0              \n"
        "shlr    r12             \n"
        "rotcl   r0              \n"
        "mov.b   r0,@%0          \n"  /* store byte to bitplane */
        "add     %2,%0           \n"  /* advance to next bitplane */
        "cmp/hi  %0,%1           \n"  /* last bitplane done? */
        "bt      .wa_sloop       \n"  /* no: loop */

    ".wa_end:                    \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(address),
        /* %1 */ "r"(end_addr),
        /* %2 */ "r"(graybuf->plane_size),
        /* %3 */ "r"(pat_ptr),
        /* %4 */ "r"(mask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r8", "r9", "r10", "r11", "r12"
    );
}

/* Write an 8-pixel block, defined by foreground and background pattern.
 * Address is the byte in the first bitplane, mask determines which pixels to
 * set, and bits determines if the pixel is foreground or background */
static void _writeblock(unsigned char *address, unsigned mask, unsigned bits)
{
    unsigned long pat_stack[8];
    register unsigned char *end_addr;
    register unsigned long *pat_ptr = &pat_stack[8];

    /* precalculate the bit patterns with random shifts (same RNG as _writepixel,
     * see there for an explanation) for all 8 pixels and put them on an
     * extra stack */
    asm (
        "mov     #8,r3           \n"  /* loop count in r3: 8 pixels */
        "mov     %6,r2           \n"  /* copy mask */

    ".wb_loop:                   \n"  /** load pattern for pixel **/
        "shlr    r2              \n"  /* shift out lsb of mask */
        "bf      .wb_skip        \n"  /* skip this pixel */

        "mov     %2,r4           \n"  /* load foreground pattern */
        "shlr    %7              \n"  /* shift out lsb of bits */
        "bt      .wb_fg          \n"  /* foreground? -> skip next insn */
        "mov     %3,r4           \n"  /* load background pattern */
    ".wb_fg:                     \n"

        "mov     #75,r0          \n"
        "mulu    r0,%0           \n"  /* multiply by 75 */
        "sts     macl,%0         \n"
        "add     #74,%0          \n"  /* add another 74 */
        /* Since the lower bits are not very random: */
        "swap.b  %0,r1           \n"  /* get bits 8..15 (need max. 5) */
        "and     %5,r1           \n"  /* mask out unneeded bits */

        "cmp/hs  %4,r1           \n"  /* random >= depth ? */
        "bf      .wb_ntrim       \n"
        "sub     %4,r1           \n"  /* yes: random -= depth; */
    ".wb_ntrim:                  \n"

        "mov.l   .ashlsi3,r0     \n"  /** rotate pattern **/
        "jsr     @r0             \n"  /* r4 -> r0, shift left by r5 */
        "mov     r1,r5           \n"

        "mov     %4,r5           \n"
        "sub     r1,r5           \n"  /* r5 = depth - r1 */
        "mov.l   .lshrsi3,r1     \n"
        "jsr     @r1             \n"  /* r4 -> r0, shift right by r5 */
        "mov     r0,r1           \n"  /* store previous result in r1 */

        "bra     .wb_store       \n"
        "or      r1,r0           \n"  /* rotated_pattern = r0 | r1 */

    ".wb_skip:                   \n"
        "shlr    %7              \n"  /* shift out lsb of bits to keep in sync */
        "mov     #0,r0           \n"  /* pattern for skipped pixel must be 0 */

    ".wb_store:                  \n"
        "mov.l   r0,@-%1         \n"  /* push on pattern stack */

        "add     #-1,r3          \n"  /* decrease loop count */
        "cmp/pl  r3              \n"  /* loop count > 0? */
        "bt      .wb_loop        \n"  /* yes: loop */
        : /* outputs */
        /* %0, in & out */ "+r"(random_buffer),
        /* %1, in & out */ "+r"(pat_ptr)
        : /* inputs */
        /* %2 */ "r"(graybuf->fg_pattern),
        /* %3 */ "r"(graybuf->bg_pattern),
        /* %4 */ "r"(graybuf->depth),
        /* %5 */ "r"(graybuf->randmask),
        /* %6 */ "r"(mask),
        /* %7 */ "r"(bits)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "macl"
    );

    end_addr = address + MULU16(graybuf->depth, graybuf->plane_size);

    /* set the bits for all 8 pixels in all bytes according to the
     * precalculated patterns on the pattern stack */
    asm volatile (
        "mov.l   @%3+,r1         \n"  /* pop all 8 patterns */
        "mov.l   @%3+,r2         \n"
        "mov.l   @%3+,r3         \n"
        "mov.l   @%3+,r8         \n"
        "mov.l   @%3+,r9         \n"
        "mov.l   @%3+,r10        \n"
        "mov.l   @%3+,r11        \n"
        "mov.l   @%3+,r12        \n"

        "not     %4,%4           \n"  /* "set" mask -> "keep" mask */
        "extu.b  %4,%4           \n"  /* mask out high bits */
        "tst     %4,%4           \n"  /* nothing to keep? */
        "bt      .wb_sloop       \n"  /* yes: jump to short loop */

    ".wb_floop:                  \n"  /** full loop (there are bits to keep)**/
        "shlr    r1              \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0              \n"  /* rotate t bit into r0 */
        "shlr    r2              \n"
        "rotcl   r0              \n"
        "shlr    r3              \n"
        "rotcl   r0              \n"
        "shlr    r8              \n"
        "rotcl   r0              \n"
        "shlr    r9              \n"
        "rotcl   r0              \n"
        "shlr    r10             \n"
        "rotcl   r0              \n"
        "shlr    r11             \n"
        "rotcl   r0              \n"
        "shlr    r12             \n"
        "mov.b   @%0,%3          \n"  /* read old value */
        "rotcl   r0              \n"
        "and     %4,%3           \n"  /* mask out unneeded bits */
        "or      r0,%3           \n"  /* set new bits */
        "mov.b   %3,@%0          \n"  /* store value to bitplane */
        "add     %2,%0           \n"  /* advance to next bitplane */
        "cmp/hi  %0,%1           \n"  /* last bitplane done? */
        "bt      .wb_floop       \n"  /* no: loop */

        "bra     .wb_end         \n"
        "nop                     \n"

    ".wb_sloop:                  \n"  /** short loop (nothing to keep) **/
        "shlr    r1              \n"  /* rotate lsb of pattern 1 to t bit */
        "rotcl   r0              \n"  /* rotate t bit into r0 */
        "shlr    r2              \n"
        "rotcl   r0              \n"
        "shlr    r3              \n"
        "rotcl   r0              \n"
        "shlr    r8              \n"
        "rotcl   r0              \n"
        "shlr    r9              \n"
        "rotcl   r0              \n"
        "shlr    r10             \n"
        "rotcl   r0              \n"
        "shlr    r11             \n"
        "rotcl   r0              \n"
        "shlr    r12             \n"
        "rotcl   r0              \n"
        "mov.b   r0,@%0          \n"  /* store byte to bitplane */
        "add     %2,%0           \n"  /* advance to next bitplane */
        "cmp/hi  %0,%1           \n"  /* last bitplane done? */
        "bt      .wb_sloop       \n"  /* no: loop */

    ".wb_end:                    \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(address),
        /* %1 */ "r"(end_addr),
        /* %2 */ "r"(graybuf->plane_size),
        /* %3 */ "r"(pat_ptr),
        /* %4 */ "r"(mask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r8", "r9", "r10", "r11", "r12"
    );
}

/* References to C library routines used in _writearray and _writeblock */
asm (
    ".align  2               \n"
".ashlsi3:                   \n"  /* C library routine: */
    ".long   ___ashlsi3      \n"  /* shift r4 left by r5, return in r0 */
".lshrsi3:                   \n"  /* C library routine: */
    ".long   ___lshrsi3      \n"  /* shift r4 right by r5, return in r0 */
    /* both routines preserve r4, destroy r5 and take ~16 cycles */
);

/* Invert pixels within an 8-pixel block.
 * Address is the byte in the first bitplane, mask and bits determine which
 * pixels to invert ('And'ed together, for matching the parameters with
 * _writeblock) */
static void _invertblock(unsigned char *address, unsigned mask, unsigned bits)
{
    asm volatile (
        "mov     #0,r1           \n"  /* current_plane = 0 */

    ".im_loop:                   \n"
        "mov.b   @%1,r2          \n"  /* get data byte */
        "add     #1,r1           \n"  /* current_plane++; */
        "xor     %2,r2           \n"  /* invert bits */
        "mov.b   r2,@%1          \n"  /* store data byte */
        "add     %3,%1           \n"  /* advance address to next bitplane */
        "cmp/hi  r1,%0           \n"  /* current_plane < depth ? */
        "bt      .im_loop        \n"
        : /* outputs */
        : /* inputs */
        /* %0 */ "r"(graybuf->depth),
        /* %1 */ "r"(address),
        /* %2 */ "r"(mask & bits),
        /* %3 */ "r"(graybuf->plane_size)
        : /* clobbers */
        "r1", "r2", "macl"
    );
}

/* Call _writeblock with the mask modified to draw foreground pixels only */
static void _writeblockfg(unsigned char *address, unsigned mask, unsigned bits)
{
    _writeblock(address, mask & bits, bits);
}

/* Call _writeblock with the mask modified to draw background pixels only */
static void _writeblockbg(unsigned char *address, unsigned mask, unsigned bits)
{
    _writeblock(address, mask & ~bits, bits);
}

/**** public functions ****/

/* Initialize the framework
 *
 * Every framework needs such a function, and it has to be called as
 * the very first one */
void gray_init(struct plugin_api* newrb)
{
    rb = newrb;
}

/** General functions **/

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
 * If you need info about the memory taken by the grayscale buffer, supply an
 * int* as the last parameter. This int will then contain the number of bytes
 * used. The total memory needed can be calculated as follows:
 *   total_mem =
 *     sizeof(tGraybuf)      (= 64 bytes currently)
 *   + sizeof(long)          (=  4 bytes)
 *   + (width * bheight + sizeof(long)) * depth
 *   + 0..3                  (longword alignment of grayscale display buffer)
 */
int gray_init_buffer(unsigned char *gbuf, int gbuf_size, int width,
                     int bheight, int depth, int *buf_taken)
{
    int possible_depth, plane_size;
    int i, j, align;

    if ((unsigned) width > LCD_WIDTH
        || (unsigned) bheight > (LCD_HEIGHT/8)
        || depth < 1)
        return 0;

    /* the buffer has to be long aligned */
    align = 3 - (((unsigned long)gbuf + 3) & 3);
    gbuf += align;
    gbuf_size -= align;

    plane_size = MULU16(width, bheight);
    possible_depth = (gbuf_size - sizeof(tGraybuf) - sizeof(long))
                     / (plane_size + sizeof(long));

    if (possible_depth < 1)
        return 0;

    depth = MIN(depth, 32);
    depth = MIN(depth, possible_depth);

    graybuf = (tGraybuf *) gbuf;  /* global pointer to buffer structure */
                                  
    graybuf->x = 0;
    graybuf->by = 0;
    graybuf->width = width;
    graybuf->height = bheight << 3;
    graybuf->bheight = bheight;
    graybuf->plane_size = plane_size;
    graybuf->depth = depth;
    graybuf->cur_plane = 0;
    graybuf->flags = 0;
    graybuf->bitpattern = (unsigned long *) (gbuf + sizeof(tGraybuf));
    graybuf->data = (unsigned char *) (graybuf->bitpattern + depth + 1);

    i = depth - 1;
    j = 8;
    while (i != 0)
    {
        i >>= 1;
        j--;
    }
    graybuf->randmask = 0xFFu >> j;

    /* initial state is all white */
    rb->memset(graybuf->data, 0, MULU16(depth, plane_size));
    
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
    
    graybuf->fg_pattern = graybuf->bitpattern[0]; /* black */
    graybuf->bg_pattern = graybuf->bitpattern[depth]; /* white */
    graybuf->drawmode = GRAY_DRAW_SOLID;
    graybuf->curfont = FONT_SYSFIXED;

    if (buf_taken)  /* caller requested info about space taken */
    {
        *buf_taken = sizeof(tGraybuf) + sizeof(long)
                   + MULU16(plane_size + sizeof(long), depth) + align;
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
    graybuf->x = x;
    graybuf->by = by;
    
    if (graybuf->flags & GRAY_RUNNING)
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
}

/* Switch the grayscale overlay on or off
 *
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
    if (enable)
    {
        graybuf->flags |= GRAY_RUNNING;
        rb->plugin_register_timer(FREQ / 67, 1, _timer_isr);
    }
    else
    {
        rb->plugin_unregister_timer();
        graybuf->flags &= ~GRAY_RUNNING;
        rb->lcd_update(); /* restore whatever there was before */
    }
}

/* Set the draw mode for subsequent drawing operations
 *
 *   drawmode =
 *     GRAY_DRAW_INVERSE: Foreground pixels are inverted, background pixels are
 *                        left untouched
 *     GRAY_DRAW_FG:      Only foreground pixels are drawn
 *     GRAY_DRAW_BG:      Only background pixels are drawn
 *     GRAY_DRAW_SOLID:   Foreground and background pixels are drawn
 */
void gray_set_drawmode(int drawmode)
{
    if ((unsigned) drawmode <= GRAY_DRAW_SOLID)
        graybuf->drawmode = drawmode;
}

/* Set the foreground shade for subsequent drawing operations
 *
 * brightness = 0 (black) .. 255 (white)
 */
void gray_set_foreground(int brightness)
{
    if ((unsigned) brightness <= 255)
        graybuf->fg_pattern = graybuf->bitpattern[MULU16(brightness,
                              graybuf->depth + 1) >> 8];
}

/* Set the background shade for subsequent drawing operations
 *
 * brightness = 0 (black) .. 255 (white)
 */
void gray_set_background(int brightness)
{
    if ((unsigned) brightness <= 255)
        graybuf->bg_pattern = graybuf->bitpattern[MULU16(brightness,
                              graybuf->depth + 1) >> 8];
}

/* Set draw mode, foreground and background shades at once
 * 
 * If you hand it -1 (or in fact any other out-of-bounds value) for a
 * parameter, that particular setting won't be changed
 */
void gray_set_drawinfo(int drawmode, int fg_brightness, int bg_brightness)
{
    gray_set_drawmode(drawmode);
    gray_set_foreground(fg_brightness);
    gray_set_background(bg_brightness);
}

/** Functions affecting the whole dsplay **/

/* Clear the grayscale display (sets all pixels to white) */
void gray_clear_display(void)
{
    rb->memset(graybuf->data, 0, MULU16(graybuf->depth, graybuf->plane_size));
}

/* Set the grayscale display to all black */
void gray_black_display(void)
{
    rb->memset(graybuf->data, 0xFF, MULU16(graybuf->depth, graybuf->plane_size));
}

/* Do an lcd_update() to show changes done by rb->lcd_xxx() functions (in areas
 * of the screen not covered by the grayscale overlay).
 *
 * If the grayscale overlay is running, the update will be done in the next
 * call of the interrupt routine, otherwise it will be performed right away.
 * See also comment for the gray_show_display() function.
 */
void gray_deferred_update(void)
{
    if (graybuf->flags & GRAY_RUNNING)
        graybuf->flags |= GRAY_DEFERRED_UPDATE;
    else
        rb->lcd_update();
}

/** Scrolling functions **/

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
    unsigned filler;
    unsigned char *ptr;

    if ((unsigned) count >= (unsigned) graybuf->width)
        return;
        
    filler = black_border ? 0xFF : 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 0; by < graybuf->bheight; by++)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
        {   
            asm volatile (
                "mov     %0,r1            \n" /* check if both source... */
                "or      %2,r1            \n" /* ...and offset are even */
                "shlr    r1               \n" /* -> lsb = 0 */
                "bf      .sl_start2       \n" /* -> copy word-wise */

                "add     #-1,%2           \n" /* copy byte-wise */
            ".sl_loop1:                   \n"
                "mov.b   @%0+,r1          \n"
                "mov.b   r1,@(%2,%0)      \n"
                "cmp/hi  %0,%1            \n"
                "bt      .sl_loop1        \n"
                
                "bra     .sl_end          \n"
                "nop                      \n"

            ".sl_start2:                  \n" /* copy word-wise */
                "add     #-2,%2           \n"
            ".sl_loop2:                   \n"
                "mov.w   @%0+,r1          \n"
                "mov.w   r1,@(%2,%0)      \n"
                "cmp/hi  %0,%1            \n"
                "bt      .sl_loop2        \n"

            ".sl_end:                     \n"
                : /* outputs */
                : /* inputs */
                /* %0 */ "r"(ptr + count),
                /* %1 */ "r"(ptr + graybuf->width),
                /* %2 */ "z"(-count)
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
    unsigned filler;
    unsigned char *ptr;

    if ((unsigned) count >= (unsigned) graybuf->width)
        return;

    filler = black_border ? 0xFF : 0;

    /* Scroll row by row to minimize flicker (byte rows = 8 pixels each) */
    for (by = 0; by < graybuf->bheight; by++)
    {
        ptr = graybuf->data + MULU16(graybuf->width, by);
        for (d = 0; d < graybuf->depth; d++)
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
                /* %0 */ "r"(ptr + graybuf->width),
                /* %1 */ "r"(ptr + count),
                /* %2 */ "z"(-count)
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
    unsigned filler;
    unsigned char *ptr;

    filler = black_border ? 0xFF : 0;

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
    unsigned filler;
    unsigned char *ptr;

    filler = black_border ? 0xFF : 0;

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
    unsigned filler;

    if ((unsigned) count > 7)
        return;

    filler = black_border ? 0xFFu : 0;

    /* scroll column by column to minimize flicker */
    asm volatile (
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

/** Pixel and line functions **/

/* Set a pixel with the current drawinfo
 * 
 * If the drawmode is GRAY_DRAW_INVERSE, the pixel is inverted
 * GRAY_DRAW_FG and GRAY_DRAW_SOLID draw the pixel in the foreground shade
 * GRAY_DRAW_BG draws the pixel in the background shade
 */
void gray_drawpixel(int x, int y)
{
    unsigned long pattern;

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;

    pattern = (graybuf->drawmode == GRAY_DRAW_BG) ?
              graybuf->bg_pattern : graybuf->fg_pattern;

    _pixelfuncs[graybuf->drawmode](x, y, pattern);
}

/* Draw a line from (x1, y1) to (x2, y2) with the current drawinfo,
 * See gray_drawpixel() for details
 */
void gray_drawline(int x1, int y1, int x2, int y2)
{
    int numpixels;
    int i;
    int deltax, deltay;
    int d, dinc1, dinc2;
    int x, xinc1, xinc2;
    int y, yinc1, yinc2;
    unsigned long pattern;
    void (*pixelfunc)(int x, int y, unsigned long pattern);

    if ((unsigned) x1 >= (unsigned) graybuf->width
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
    
    pixelfunc = _pixelfuncs[graybuf->drawmode];
    pattern = (graybuf->drawmode == GRAY_DRAW_BG) ?
              graybuf->bg_pattern : graybuf->fg_pattern;

    for (i = 0; i < numpixels; i++)
    {
        pixelfunc(x, y, pattern);

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

/* Draw a horizontal line from (x1, y) to (x2, y) with the current drawinfo,
 * See gray_drawpixel() for details
 */
void gray_horline(int x1, int x2, int y)
{
    int x;
    unsigned long pattern;
    void (*pixelfunc)(int x, int y, unsigned long pattern);

    if ((unsigned) x1 >= (unsigned) graybuf->width
        || (unsigned) x2 >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;

    if (x1 > x2)
    {
        x = x1;
        x1 = x2;
        x2 = x;
    }
    pixelfunc = _pixelfuncs[graybuf->drawmode];
    pattern = (graybuf->drawmode == GRAY_DRAW_BG) ?
              graybuf->bg_pattern : graybuf->fg_pattern;
              
    for (x = x1; x <= x2; x++)
        pixelfunc(x, y, pattern);

}

/* Draw a vertical line from (x, y1) to (x, y2) with the current drawinfo,
 * See gray_drawpixel() for details
 *
 * This one uses the block drawing optimization, so it is rather fast.
 */
void gray_verline(int x, int y1, int y2)
{
    int shift, y, ny;
    unsigned bits, mask_top, mask_bottom;
    unsigned char *dst;
    void (*blockfunc)(unsigned char *address, unsigned mask, unsigned bits);

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y1 >= (unsigned) graybuf->height
        || (unsigned) y2 >= (unsigned) graybuf->height)
        return;
    
    if (y1 > y2)
    {
        y = y1;
        y1 = y2;
        y2 = y;
    }
    y = y1;
    ny = y2 - y1 + 1;
        
    dst = graybuf->data + x + MULU16(graybuf->width, y >> 3);
    shift = y & 7;
    ny += shift;

    mask_top = 0xFFu << (y & 7);
    mask_bottom = ~(0xFEu << ((ny - 1) & 7));
    if (ny <= 8)
        mask_bottom &= mask_top;
    
    blockfunc = _blockfuncs[graybuf->drawmode];
    bits = (graybuf->drawmode == GRAY_DRAW_BG) ? 0u : 0xFFu;

    if (ny > 8)
    {
        blockfunc(dst, mask_top, bits);
        dst += graybuf->width;

        for (y = 8; y < ny - 8; y += 8)
        {
            blockfunc(dst, 0xFFu, bits);
            dst += graybuf->width;
        }
    }

    blockfunc(dst, mask_bottom, bits);
}

/** Rectangle functions **/

/* Draw a (hollow) rectangle with the current drawinfo,
 * See gray_drawpixel() for details
 */
void gray_drawrect(int x, int y, int nx, int ny)
{
    int x2, y2;

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;

    if ((unsigned) (y + ny) >= (unsigned) graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) graybuf->width) /* clip right */
        nx = graybuf->width - x;
        
    x2 = x + nx - 1;
    y2 = y + ny - 1;

    gray_horline(x, x2, y);
    gray_horline(x, x2, y2);
    gray_verline(x, y, y2);
    gray_verline(x2, y, y2);
}

/* Draw a filled rectangle with the current drawinfo,
 * See gray_drawpixel() for details
 *
 * This one uses the block drawing optimization, so it is rather fast.
 */
void gray_fillrect(int x, int y, int nx, int ny)
{
    int shift;
    unsigned bits, mask_top, mask_bottom;
    unsigned char *dst, *dst_row;
    void (*blockfunc)(unsigned char *address, unsigned mask, unsigned bits);
    
    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;
        
    if ((unsigned) (y + ny) >= (unsigned) graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) graybuf->width) /* clip right */
        nx = graybuf->width - x;

    dst = graybuf->data + x + MULU16(graybuf->width, y >> 3);
    shift = y & 7;
    ny += shift;

    mask_top = 0xFFu << (y & 7);
    mask_bottom = ~(0xFEu << ((ny - 1) & 7));
    if (ny <= 8)
        mask_bottom &= mask_top;
    
    blockfunc = _blockfuncs[graybuf->drawmode];
    bits = (graybuf->drawmode == GRAY_DRAW_BG) ? 0u : 0xFFu;

    if (ny > 8)
    {
        dst_row = dst;
        for (x = 0; x < nx; x++)
            blockfunc(dst_row++, mask_top, bits);

        dst += graybuf->width;
            
        for (y = 8; y < ny - 8; y += 8)
        {
            dst_row = dst;
            for (x = 0; x < nx; x++)
                blockfunc(dst_row++, 0xFFu, bits);

            dst += graybuf->width;
        }
    }

    for (x = 0; x < nx; x++)
        blockfunc(dst++, mask_bottom, bits);
}

/** Bitmap functions **/

/* Copy a grayscale bitmap into the display
 * 
 * A grayscale bitmap contains one byte for every pixel that defines the
 * brightness of the pixel (0..255). Bytes are read in row-major order.
 * The <stride> parameter is useful if you want to show only a part of a
 * bitmap. It should always be set to the "row length" of the bitmap, so
 * for displaying the whole bitmap, nx == stride.
 *
 * This is the only drawing function NOT using the drawinfo.
 */
void gray_drawgraymap(unsigned char *src, int x, int y, int nx, int ny,
                      int stride)
{
    int shift;
    unsigned mask_top, mask_bottom;
    unsigned char *src_row, *dst, *dst_row;

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;
    
    if ((unsigned) (y + ny) >= (unsigned) graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) graybuf->width) /* clip right */
        nx = graybuf->width - x;
        
    dst = graybuf->data + x + MULU16(graybuf->width, y >> 3);
    shift = y & 7;
    ny += shift;

    mask_top = 0xFFu << (y & 7);
    mask_bottom = ~(0xFEu << ((ny - 1) & 7));
    if (ny <= 8)
        mask_bottom &= mask_top;

    if (ny > 8)
    {
        src_row = src;
        dst_row = dst;

        for (x = 0; x < nx; x++)
            _writearray(dst_row++, src_row++, stride, mask_top);
            
        src += MULU16(stride, 8 - shift);
        dst += graybuf->width;

        for (y = 8; y < ny - 8; y += 8)
        {
            src_row = src;
            dst_row = dst;
            
            for (x = 0; x < nx; x++)
                _writearray(dst_row++, src_row++, stride, 0xFFu);

            src += stride << 3;
            dst += graybuf->width;
        }
    }

    for (x = 0; x < nx; x++)
        _writearray(dst++, src++, stride, mask_bottom);
}

/* Display a bitmap with the current drawinfo
 *
 * The drawmode is used as described for gray_set_drawmode()
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
 */
void gray_drawbitmap(unsigned char *src, int x, int y, int nx, int ny,
                     int stride)
{
    int shift;
    unsigned bits, mask_top, mask_bottom;
    unsigned char *src_col, *dst, *dst_col;
    void (*blockfunc)(unsigned char *address, unsigned mask, unsigned bits);

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;
        
    if ((unsigned) (y + ny) >= (unsigned) graybuf->height) /* clip bottom */
        ny = graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) graybuf->width) /* clip right */
        nx = graybuf->width - x;

    dst = graybuf->data + x + MULU16(graybuf->width, y >> 3);
    shift = y & 7;
    ny += shift;

    mask_top = 0xFFu << (y & 7);
    mask_bottom = ~(0xFEu << ((ny - 1) & 7));
    if (ny <= 8)
        mask_bottom &= mask_top;

    blockfunc = _blockfuncs[graybuf->drawmode];

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
            dst_col += graybuf->width;
            bits >>= 8;

            for (y = 8; y < ny - 8; y += 8)
            {
                bits |= *src_col << shift;
                src_col += stride;
                
                blockfunc(dst_col, 0xFFu, bits);
                dst_col += graybuf->width;
                bits >>= 8;
            }
        }
        if (y + shift < ny)
            bits |= *src_col << shift;

        blockfunc(dst_col, mask_bottom, bits);
    }
}

/** Font support **/

/* Set font for the font routines
 * 
 * newfont can be FONT_SYSFIXED or FONT_UI the same way as with the Rockbox
 * core routines
 */
void gray_setfont(int newfont)
{
    graybuf->curfont = rb->font_get(newfont);
}

/* Calculate width and height of the given text in pixels when rendered with
 * the currently selected font.
 *
 * This works exactly the same way as the core lcd_getstringsize(), only that
 * it uses the selected font for grayscale.
 */
int gray_getstringsize(unsigned char *str, int *w, int *h)
{
    int ch;
    int width = 0;
    struct font *pf = graybuf->curfont;

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

/* Display text starting at (x, y) with the current font and drawinfo
 *
 * The drawmode is used as described for gray_set_drawmode()
 */
void gray_putsxy(int x, int y, unsigned char *str)
{
    int ch, width;
    bitmap_t *bits;
    struct font *pf = graybuf->curfont;

    if ((unsigned) x >= (unsigned) graybuf->width
        || (unsigned) y >= (unsigned) graybuf->height)
        return;

    while ((ch = *str++) != '\0' && x < graybuf->width)
    {
        /* check input range */
        if (ch < pf->firstchar || ch >= pf->firstchar + pf->size)
            ch = pf->defaultchar;
        ch -= pf->firstchar;

        /* get proportional width and glyph bits */
        width = pf->width ? pf->width[ch] : pf->maxwidth;
        bits = pf->bits + (pf->offset ? pf->offset[ch] 
                                      : MULU16(pf->height, ch));

        gray_drawbitmap((unsigned char*) bits, x, y, width, pf->height,
                        width);
        x += width;
    }
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


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
* Low level block drawing functions
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

/* Prototypes */
static void _writeblock(unsigned char *address, unsigned mask, unsigned bits);
static void _invertblock(unsigned char *address, unsigned mask, unsigned bits);
static void _writeblockfg(unsigned char *address, unsigned mask, unsigned bits);
static void _writeblockbg(unsigned char *address, unsigned mask, unsigned bits);

/* Block function pointer array */
void (* const _gray_blockfuncs[4])(unsigned char *address, unsigned mask,
                                   unsigned bits) = {
    _invertblock, _writeblockfg, _writeblockbg, _writeblock
};

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
        "sts.l   pr,@-r15        \n"  /* save pr (fix GCC331 build, cleaner) */
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
        "lds.l   @r15+,pr        \n"  /* retsore pr */
        : /* outputs */
        /* %0, in & out */ "+r"(_gray_random_buffer),
        /* %1, in & out */ "+r"(pat_ptr)
        : /* inputs */
        /* %2 */ "r"(_graybuf->fg_pattern),
        /* %3 */ "r"(_graybuf->bg_pattern),
        /* %4 */ "r"(_graybuf->depth),
        /* %5 */ "r"(_graybuf->randmask),
        /* %6 */ "r"(mask),
        /* %7 */ "r"(bits)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "macl"
    );

    end_addr = address + MULU16(_graybuf->depth, _graybuf->plane_size);

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
        /* %2 */ "r"(_graybuf->plane_size),
        /* %3 */ "r"(pat_ptr),
        /* %4 */ "r"(mask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r8", "r9", "r10", "r11", "r12"
    );
}

/* References to C library routines used in _writeblock */
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
    bits &= mask;
    if (!bits)
        return;

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
        /* %0 */ "r"(_graybuf->depth),
        /* %1 */ "r"(address),
        /* %2 */ "r"(bits),
        /* %3 */ "r"(_graybuf->plane_size)
        : /* clobbers */
        "r1", "r2", "macl"
    );
}

/* Call _writeblock with the mask modified to draw foreground pixels only */
static void _writeblockfg(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= bits;
    if (mask)
        _writeblock(address, mask, bits);
}

/* Call _writeblock with the mask modified to draw background pixels only */
static void _writeblockbg(unsigned char *address, unsigned mask, unsigned bits)
{
    mask &= ~bits;
    if (mask)
        _writeblock(address, mask, bits);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


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
* Low level pixel drawing functions
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
static void _writepixel(int x, int y, unsigned long pattern);
static void _invertpixel(int x, int y, unsigned long pattern);

/* function pointer array */
void (* const _gray_pixelfuncs[4])(int x, int y, unsigned long pattern) = {
    _invertpixel, _writepixel, _writepixel, _writepixel
};

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
        /* %1, in & out */ "+r"(_gray_random_buffer)
        : /* inputs */
        /* %2 */ "r"(_graybuf->randmask)
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
        /* %2 */ "r"(_graybuf->width),
        /* %3 = r0 */ "z"(y),
        /* %4 */ "r"(_graybuf->data + x)
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
        /* %1 */ "r"(_graybuf->depth),
        /* %2 */ "r"(address),
        /* %3 */ "r"(mask),
        /* %4 */ "r"(_graybuf->plane_size),
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
        /* %2 */ "r"(_graybuf->width),
        /* %3 = r0 */ "z"(y),
        /* %4 */ "r"(_graybuf->data + x)
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
        /* %0 */ "r"(_graybuf->depth),
        /* %1 */ "r"(address),
        /* %2 */ "r"(mask),
        /* %3 */ "r"(_graybuf->plane_size)
        : /* clobbers */
        "r1", "r2"
    );
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


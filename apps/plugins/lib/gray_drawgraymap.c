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
* gray_drawgraymap() function
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
static void _writearray(unsigned char *address, const unsigned char *src,
                        int stride, unsigned mask);

/* Write an 8-pixel block, defined by their brightnesses in a graymap.
 * Address is the byte in the first bitplane, src is the graymap start address,
 * stride is the increment for the graymap to get to the next pixel, mask
 * determines which pixels of the destination block are changed. For "0" bits,
 * the src address is not incremented! */
static void _writearray(unsigned char *address, const unsigned char *src,
                        int stride, unsigned mask)
{
    unsigned long pat_stack[8];
    register unsigned char *end_addr;
    register unsigned long *pat_ptr = &pat_stack[8];

    /* precalculate the bit patterns with random shifts (same RNG as
     * _writepixel, see there for an explanation) for all 8 pixels and put them
     * on an extra "stack" */
    asm (
        "sts.l   pr,@-r15        \n"  /* save pr (fix GCC331 build, cleaner) */
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
        "lds.l   @r15+,pr        \n"  /* restore pr */
        : /* outputs */
        /* %0, in & out */ "+r"(_gray_random_buffer),
        /* %1, in & out */ "+r"(pat_ptr)
        : /* inputs */
        /* %2 */ "r"(src),
        /* %3 */ "r"(stride),
        /* %4 */ "r"(_graybuf->depth),
        /* %5 */ "r"(_graybuf->bitpattern),
        /* %6 */ "r"(_graybuf->randmask),
        /* %7 */ "r"(mask)
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
        /* %2 */ "r"(_graybuf->plane_size),
        /* %3 */ "r"(pat_ptr),
        /* %4 */ "r"(mask)
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r8", "r9", "r10", "r11", "r12"
    );
}

/* References to C library routines used in _writearray */
asm (
    ".align  2               \n"
".ashlsi3:                   \n"  /* C library routine: */
    ".long   ___ashlsi3      \n"  /* shift r4 left by r5, return in r0 */
".lshrsi3:                   \n"  /* C library routine: */
    ".long   ___lshrsi3      \n"  /* shift r4 right by r5, return in r0 */
    /* both routines preserve r4, destroy r5 and take ~16 cycles */
);

/*---------------------------------------------------------------------------
 Copy a grayscale bitmap into the display
 ----------------------------------------------------------------------------
 A grayscale bitmap contains one byte for every pixel that defines the
 brightness of the pixel (0..255). Bytes are read in row-major order.
 The <stride> parameter is useful if you want to show only a part of a
 bitmap. It should always be set to the "row length" of the bitmap, so
 for displaying the whole bitmap, nx == stride.

 This is the only drawing function NOT using the drawinfo.
 */
void gray_drawgraymap(const unsigned char *src, int x, int y, int nx, int ny,
                      int stride)
{
    int shift;
    unsigned mask_top, mask_bottom;
    const unsigned char *src_row;
    unsigned char *dst, *dst_row;

    if ((unsigned) x >= (unsigned) _graybuf->width
        || (unsigned) y >= (unsigned) _graybuf->height)
        return;
    
    if ((unsigned) (y + ny) >= (unsigned) _graybuf->height) /* clip bottom */
        ny = _graybuf->height - y;

    if ((unsigned) (x + nx) >= (unsigned) _graybuf->width) /* clip right */
        nx = _graybuf->width - x;
        
    dst = _graybuf->data + x + MULU16(_graybuf->width, y >> 3);
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
        dst += _graybuf->width;

        for (y = 8; y < ny - 8; y += 8)
        {
            src_row = src;
            dst_row = dst;
            
            for (x = 0; x < nx; x++)
                _writearray(dst_row++, src_row++, stride, 0xFFu);

            src += stride << 3;
            dst += _graybuf->width;
        }
    }

    for (x = 0; x < nx; x++)
        _writearray(dst++, src++, stride, mask_bottom);
}

#endif // #ifdef HAVE_LCD_BITMAP
#endif // #ifndef SIMULATOR


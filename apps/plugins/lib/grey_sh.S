/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* New greyscale framework
* SH1 assembler routines
*
* This is a generic framework to display 129 shades of grey on low-depth
* bitmap LCDs (Archos b&w, Iriver & Ipod 4-grey) within plugins.
*
* Copyright (C) 2008 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "config.h"
/* Plugins should not normally do this, but we need to check a macro, and
 * plugin.h would confuse the assembler. */
 
    .text
    .global     __grey_line1
    .type       __grey_line1, @function
    
#if (LCD_PIXELFORMAT == VERTICAL_PACKING) && (LCD_DEPTH == 1)
    
/****************************************************************************
 * void _grey_line1(int width,                        r4
 *                  unsigned char *dst,               r5
 *                  const unsigned char *src,         r6
 *                  const unsigned char *lut);        r7
 */

__grey_line1:
    mov     #1, r0
    tst     r0, r6
    bt      .p1_h_end
    
    mov.b   @r6+, r0
    extu.b  r0, r0
    mov.b   @(r0, r7), r0
    add     #-1, r4
    mov.b   r0, @r5
    add     #8, r5
.p1_h_end:

    mov     #2, r0
    cmp/hs  r0, r4
    bf      .p2_t_end
    tst     r0, r6
    bt      .p2_h_end
    
    mov.w   @r6+, r1
    extu.b  r1, r0
    mov.b   @(r0, r7), r0
    shlr8   r1
    mov.b   r0, @(8, r5)
    extu.b  r1, r0
    mov.b   @(r0, r7), r0
    add     #-2, r4
    mov.b   r0, @r5
    add     #16, r5
.p2_h_end:

    add     #-4, r4
    cmp/pz  r4
    bf      .p4_end

    add     r6, r4

.p4_loop:
    mov.l   @r6+, r1
    swap.w  r1, r2
    extu.b  r2, r0
    mov.b   @(r0, r7), r0
    shlr8   r2
    mov.b   r0, @(8, r5)
    extu.b  r2, r0
    mov.b   @(r0, r7), r2
    extu.b  r1, r0
    mov.b   r2, @r5
    add     #16, r5
    mov.b   @(r0, r7), r0
    shlr8   r1
    mov.b   r0, @(8, r5)
    extu.b  r1, r0
    mov.b   @(r0, r7), r0
    cmp/hs  r6, r4
    mov.b   r0, @r5
    add     #16, r5
    bt      .p4_loop
    
    /* No need to correct the count, we're only testing bits from now on. */
    
.p4_end:
    mov     #2, r0
    tst     r0, r4
    bt      .p2_t_end
    
    mov.w   @r6+, r1
    extu.b  r1, r0
    mov.b   @(r0, r7), r0
    shlr8   r1
    mov.b   r0, @(8, r5)
    extu.b  r1, r0
    mov.b   @(r0, r7), r0
    mov.b   r0, @r5
    add     #16, r5
.p2_t_end:

    mov     #1, r0
    tst     r0, r4
    bt      .p1_t_end

    mov.b   @r6+, r0
    extu.b  r0, r0
    mov.b   @(r0, r7), r0
    rts
    mov.b   r0, @r5
.p1_t_end:

    rts
    nop
    .size   _grey_line1, . - _grey_line1

#endif

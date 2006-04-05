/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"

#ifdef CPU_ARM
    .section    .icode,"ax",%progbits
#else
    .section    .icode,"ax",@progbits
#endif

    .align      2
#if CONFIG_CPU == SH7034        
    .global     _memset
    .type       _memset,@function

/* Fills a memory region with specified byte value
 * This version is optimized for speed
 *
 * arguments:
 *  r4 - start address
 *  r5 - data
 *  r6 - length
 *
 * return value:
 *  r0 - start address (like ANSI version)
 *
 * register usage:
 *  r0 - temporary
 *  r1 - start address +11 for main loop
 *  r4 - start address
 *  r5 - data (spread to all 4 bytes when using long stores)
 *  r6 - current address (runs down from end to start)
 *
 * The instruction order below is devised in a way to utilize the pipelining
 * of the SH1 to the max. The routine fills memory from end to start in
 * order to utilize the auto-decrementing store instructions.
 */

_memset:
    neg     r4,r0
    and     #3,r0       /* r0 = (4 - align_offset) % 4 */
    add     #4,r0
    cmp/hs  r0,r6       /* at least one aligned longword to fill? */
    add     r4,r6       /* r6 = end_address */
    bf      .no_longs   /* no, jump directly to byte loop */

    extu.b  r5,r5       /* start: spread data to all 4 bytes */
    swap.b  r5,r0
    or      r0,r5       /* data now in 2 lower bytes of r5 */
    swap.w  r5,r0
    or      r0,r5       /* data now in all 4 bytes of r5 */
    
    mov     r6,r0
    tst     #3,r0       /* r0 already long aligned? */
    bt      .end_b1     /* yes: skip loop */

    /* leading byte loop: sets 0..3 bytes */
.loop_b1:
    mov.b   r5,@-r0     /* store byte */
    tst     #3,r0       /* r0 long aligned? */
    bf      .loop_b1    /* runs r0 down until long aligned */
    
    mov     r0,r6       /* r6 = last long bound */
    nop                 /* keep alignment */

.end_b1:
    mov     r4,r1       /* r1 = start_address... */
    add     #11,r1      /* ... + 11, combined for rounding and offset */
    xor     r1,r0
    tst     #4,r0       /* bit 2 tells whether an even or odd number of */
    bf      .loop_odd   /* longwords to set */

    /* main loop: set 2 longs per pass */
.loop_2l:
    mov.l   r5,@-r6     /* store first long */
.loop_odd:
    cmp/hi  r1,r6       /* runs r6 down to first long bound */
    mov.l   r5,@-r6     /* store second long */
    bt      .loop_2l

.no_longs:
    cmp/hi  r4,r6       /* any bytes left? */
    bf      .end_b2     /* no: skip loop */

    /* trailing byte loop */
.loop_b2:
    mov.b   r5,@-r6     /* store byte */
    cmp/hi  r4,r6       /* runs r6 down to the start address */
    bt      .loop_b2

.end_b2:
    rts
    mov     r4,r0       /* return start address */

.end:
    .size   _memset,.end-_memset
#elif defined(CPU_COLDFIRE)
    .global     memset
    .type       memset,@function

/* Fills a memory region with specified byte value
 * This version is optimized for speed
 *
 * arguments:
 *  (4,%sp)  - start address
 *  (8,%sp)  - data
 *  (12,%sp) - length
 *
 * return value:
 *  %d0 - start address (like ANSI version)
 *
 * register usage:
 *  %d0 - data (spread to all 4 bytes when using long stores)
 *  %d1 - temporary / data (for burst transfer)
 *  %d2 - data (for burst transfer)
 *  %d3 - data (for burst transfer)
 *  %a0 - start address
 *  %a1 - current address (runs down from end to start)
 *
 * For maximum speed this routine uses both long stores and burst mode,
 * storing whole lines with movem.l. The routine fills memory from end
 * to start in order to ease returning the start address.
 */
memset:
    move.l  (4,%sp),%a0     /* start address */
    move.l  (8,%sp),%d0     /* data */
    move.l  (12,%sp),%a1    /* length */
    add.l   %a0,%a1         /* %a1 = end address */

    move.l  %a0,%d1
    addq.l  #7,%d1
    and.l   #0xFFFFFFFC,%d1 /* %d1 = first long bound + 4 */
    cmp.l   %d1,%a1         /* at least one aligned longword to fill? */
    blo.b   .no_longs       /* no, jump directly to byte loop */

    and.l   #0xFF,%d0       /* start: spread data to all 4 bytes */
    move.l  %d0,%d1
    lsl.l   #8,%d1
    or.l    %d1,%d0         /* data now in 2 lower bytes of %d0 */
    move.l  %d0,%d1
    swap    %d0
    or.l    %d1,%d0         /* data now in all 4 bytes of %d0 */

    move.l  %a1,%d1
    and.l   #0xFFFFFFFC,%d1 /* %d1 = last long bound */
    cmp.l   %d1,%a1         /* any bytes to set? */
    bls.b   .end_b1         /* no: skip byte loop */

    /* leading byte loop: sets 0..3 bytes */
.loop_b1:
    move.b  %d0,-(%a1)      /* store byte */
    cmp.l   %d1,%a1         /* runs %a1 down to last long bound */
    bhi.b   .loop_b1
    
.end_b1:
    moveq.l #31,%d1
    add.l   %a0,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = first line bound + 16 */
    cmp.l   %d1,%a1         /* at least one full line to fill? */
    blo.b   .no_lines       /* no, jump to longword loop */

    mov.l   %a1,%d1
    and.l   #0xFFFFFFF0,%d1 /* %d1 = last line bound */
    cmp.l   %d1,%a1         /* any longwords to set? */
    bls.b   .end_l1         /* no: skip longword loop */

    /* leading longword loop: sets 0..3 longwords */
.loop_l1:
    move.l  %d0,-(%a1)      /* store longword */
    cmp.l   %d1,%a1         /* runs %a1 down to last line bound */
    bhi.b   .loop_l1

.end_l1:
    move.l  %d2,-(%sp)      /* free some registers */
    move.l  %d3,-(%sp)

    move.l  %d0,%d1         /* spread data to 4 data registers */
    move.l  %d0,%d2
    move.l  %d0,%d3
    lea.l   (15,%a0),%a0    /* start address += 15, acct. for trl. data */
    
    /* main loop: set whole lines utilising burst mode */
.loop_line:
    lea.l   (-16,%a1),%a1   /* pre-decrement */
    movem.l %d0-%d3,(%a1)   /* store line */
    cmp.l   %a0,%a1         /* runs %a1 down to first line bound */
    bhi.b   .loop_line

    lea.l   (-15,%a0),%a0   /* correct start address */
    move.l  (%sp)+,%d3      /* restore registers */
    move.l  (%sp)+,%d2

    move.l  %a0,%d1         /* %d1 = start address ... */
    addq.l  #3,%d1          /* ... +3, account for possible trailing bytes */
    cmp.l   %d1,%a1         /* any longwords left */
    bhi.b   .loop_l2        /* yes: jump to longword loop */
    bra.b   .no_longs       /* no: skip loop */

.no_lines:
    move.l  %a0,%d1         /* %d1 = start address ... */
    addq.l  #3,%d1          /* ... +3, account for possible trailing bytes */

    /* trailing longword loop */
.loop_l2:
    move.l  %d0,-(%a1)      /* store longword */
    cmp.l   %d1,%a1         /* runs %a1 down to first long bound */
    bhi.b   .loop_l2

.no_longs:
    cmp.l   %a0,%a1         /* any bytes left? */
    bls.b   .end_b2         /* no: skip loop */

    /* trailing byte loop */
.loop_b2:
    move.b  %d0,-(%a1)      /* store byte */
    cmp.l   %a0,%a1         /* runs %a1 down to start address */
    bhi.b   .loop_b2

.end_b2:
    move.l  %a0,%d0         /* return start address */
    rts

.end:
    .size   memset,.end-memset

#elif defined(CPU_ARM)

/*  The following code is based on code found in Linux kernel version 2.6.15.3
 *  linux/arch/arm/lib/memset.S
 *
 *  Copyright (C) 1995-2000 Russell King
 */

/* This code will align a pointer for memset, if needed */
1:      cmp     r2, #4                  @ 1 do we have enough
        blt     5f                      @ 1 bytes to align with?
        cmp     r3, #2                  @ 1
        strgtb  r1, [r0, #-1]!          @ 1
        strgeb  r1, [r0, #-1]!          @ 1
        strb    r1, [r0, #-1]!          @ 1
        sub     r2, r2, r3              @ 1 r2 = r2 - r3
        b 2f

        .global     memset
        .type       memset,%function
memset:
        add     r0, r0, r2              @ we'll write backwards in memory
        ands    r3, r0, #3              @ 1 unaligned?
        bne     1b                      @ 1
2:
/*
 * we know that the pointer in r0 is aligned to a word boundary.
 */
        orr     r1, r1, r1, lsl #8
        orr     r1, r1, r1, lsl #16
        mov     r3, r1
        cmp     r2, #16
        blt     5f
/*
 * We need an extra register for this loop - save the return address and
 * use the LR
 */
        str     lr, [sp, #-4]!
        mov     ip, r1
        mov     lr, r1

3:      subs    r2, r2, #64
        stmgedb r0!, {r1, r3, ip, lr}   @ 64 bytes at a time.
        stmgedb r0!, {r1, r3, ip, lr}
        stmgedb r0!, {r1, r3, ip, lr}
        stmgedb r0!, {r1, r3, ip, lr}
        bgt     3b
        ldmeqfd sp!, {pc}               @ Now <64 bytes to go.
/*
 * No need to correct the count; we're only testing bits from now on
 */
        tst     r2, #32
        stmnedb r0!, {r1, r3, ip, lr}
        stmnedb r0!, {r1, r3, ip, lr}
        tst     r2, #16
        stmnedb r0!, {r1, r3, ip, lr}
        ldr     lr, [sp], #4

5:      tst     r2, #8
        stmnedb r0!, {r1, r3}
        tst     r2, #4
        strne   r1, [r0, #-4]!
/*
 * When we get here, we've got less than 4 bytes to zero.  We
 * may have an unaligned pointer as well.
 */
6:      tst     r2, #2
        strneb  r1, [r0, #-1]!
        strneb  r1, [r0, #-1]!
        tst     r2, #1
        strneb  r1, [r0, #-1]!
        mov     pc, lr
end:
        .size   memset,.end-memset
#endif


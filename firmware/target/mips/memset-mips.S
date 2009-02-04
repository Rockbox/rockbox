/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * This file was originally part of the Linux/MIPS GNU C Library
 * Copyright (C) 1998 by Ralf Baechle
 * Adapted for Rockbox by Maurus Cuelenaere, 2009
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "mips.h"

#define FILL256(dst, offset, val)     \
    sw  val, (offset + 0x00)(dst);    \
    sw  val, (offset + 0x04)(dst);    \
    sw  val, (offset + 0x08)(dst);    \
    sw  val, (offset + 0x0c)(dst);    \
    sw  val, (offset + 0x10)(dst);    \
    sw  val, (offset + 0x14)(dst);    \
    sw  val, (offset + 0x18)(dst);    \
    sw  val, (offset + 0x1c)(dst);    \
    sw  val, (offset + 0x20)(dst);    \
    sw  val, (offset + 0x24)(dst);    \
    sw  val, (offset + 0x28)(dst);    \
    sw  val, (offset + 0x2c)(dst);    \
    sw  val, (offset + 0x30)(dst);    \
    sw  val, (offset + 0x34)(dst);    \
    sw  val, (offset + 0x38)(dst);    \
    sw  val, (offset + 0x3c)(dst);    \
    sw  val, (offset + 0x40)(dst);    \
    sw  val, (offset + 0x44)(dst);    \
    sw  val, (offset + 0x48)(dst);    \
    sw  val, (offset + 0x4c)(dst);    \
    sw  val, (offset + 0x50)(dst);    \
    sw  val, (offset + 0x54)(dst);    \
    sw  val, (offset + 0x58)(dst);    \
    sw  val, (offset + 0x5c)(dst);    \
    sw  val, (offset + 0x60)(dst);    \
    sw  val, (offset + 0x64)(dst);    \
    sw  val, (offset + 0x68)(dst);    \
    sw  val, (offset + 0x6c)(dst);    \
    sw  val, (offset + 0x70)(dst);    \
    sw  val, (offset + 0x74)(dst);    \
    sw  val, (offset + 0x78)(dst);    \
    sw  val, (offset + 0x7c)(dst);    \
    sw  val, (offset + 0x80)(dst);    \
    sw  val, (offset + 0x84)(dst);    \
    sw  val, (offset + 0x88)(dst);    \
    sw  val, (offset + 0x8c)(dst);    \
    sw  val, (offset + 0x90)(dst);    \
    sw  val, (offset + 0x94)(dst);    \
    sw  val, (offset + 0x98)(dst);    \
    sw  val, (offset + 0x9c)(dst);    \
    sw  val, (offset + 0xa0)(dst);    \
    sw  val, (offset + 0xa4)(dst);    \
    sw  val, (offset + 0xa8)(dst);    \
    sw  val, (offset + 0xac)(dst);    \
    sw  val, (offset + 0xb0)(dst);    \
    sw  val, (offset + 0xb4)(dst);    \
    sw  val, (offset + 0xb8)(dst);    \
    sw  val, (offset + 0xbc)(dst);    \
    sw  val, (offset + 0xc0)(dst);    \
    sw  val, (offset + 0xc4)(dst);    \
    sw  val, (offset + 0xc8)(dst);    \
    sw  val, (offset + 0xcc)(dst);    \
    sw  val, (offset + 0xd0)(dst);    \
    sw  val, (offset + 0xd4)(dst);    \
    sw  val, (offset + 0xd8)(dst);    \
    sw  val, (offset + 0xdc)(dst);    \
    sw  val, (offset + 0xe0)(dst);    \
    sw  val, (offset + 0xe4)(dst);    \
    sw  val, (offset + 0xe8)(dst);    \
    sw  val, (offset + 0xec)(dst);    \
    sw  val, (offset + 0xf0)(dst);    \
    sw  val, (offset + 0xf4)(dst);    \
    sw  val, (offset + 0xf8)(dst);    \
    sw  val, (offset + 0xfc)(dst);

#define FILL128(dst, offset, val)     \
    sw  val, (offset + 0x00)(dst);    \
    sw  val, (offset + 0x04)(dst);    \
    sw  val, (offset + 0x08)(dst);    \
    sw  val, (offset + 0x0c)(dst);    \
    sw  val, (offset + 0x10)(dst);    \
    sw  val, (offset + 0x14)(dst);    \
    sw  val, (offset + 0x18)(dst);    \
    sw  val, (offset + 0x1c)(dst);    \
    sw  val, (offset + 0x20)(dst);    \
    sw  val, (offset + 0x24)(dst);    \
    sw  val, (offset + 0x28)(dst);    \
    sw  val, (offset + 0x2c)(dst);    \
    sw  val, (offset + 0x30)(dst);    \
    sw  val, (offset + 0x34)(dst);    \
    sw  val, (offset + 0x38)(dst);    \
    sw  val, (offset + 0x3c)(dst);    \
    sw  val, (offset + 0x40)(dst);    \
    sw  val, (offset + 0x44)(dst);    \
    sw  val, (offset + 0x48)(dst);    \
    sw  val, (offset + 0x4c)(dst);    \
    sw  val, (offset + 0x50)(dst);    \
    sw  val, (offset + 0x54)(dst);    \
    sw  val, (offset + 0x58)(dst);    \
    sw  val, (offset + 0x5c)(dst);    \
    sw  val, (offset + 0x60)(dst);    \
    sw  val, (offset + 0x64)(dst);    \
    sw  val, (offset + 0x68)(dst);    \
    sw  val, (offset + 0x6c)(dst);    \
    sw  val, (offset + 0x70)(dst);    \
    sw  val, (offset + 0x74)(dst);    \
    sw  val, (offset + 0x78)(dst);    \
    sw  val, (offset + 0x7c)(dst);

#define FILL64(dst, offset, val)      \
    sw  val, (offset + 0x00)(dst);    \
    sw  val, (offset + 0x04)(dst);    \
    sw  val, (offset + 0x08)(dst);    \
    sw  val, (offset + 0x0c)(dst);    \
    sw  val, (offset + 0x10)(dst);    \
    sw  val, (offset + 0x14)(dst);    \
    sw  val, (offset + 0x18)(dst);    \
    sw  val, (offset + 0x1c)(dst);    \
    sw  val, (offset + 0x20)(dst);    \
    sw  val, (offset + 0x24)(dst);    \
    sw  val, (offset + 0x28)(dst);    \
    sw  val, (offset + 0x2c)(dst);    \
    sw  val, (offset + 0x30)(dst);    \
    sw  val, (offset + 0x34)(dst);    \
    sw  val, (offset + 0x38)(dst);    \
    sw  val, (offset + 0x3c)(dst);

#define FILL32(dst, offset, val)      \
    sw  val, (offset + 0x00)(dst);    \
    sw  val, (offset + 0x04)(dst);    \
    sw  val, (offset + 0x08)(dst);    \
    sw  val, (offset + 0x0c)(dst);    \
    sw  val, (offset + 0x10)(dst);    \
    sw  val, (offset + 0x14)(dst);    \
    sw  val, (offset + 0x18)(dst);    \
    sw  val, (offset + 0x1c)(dst);

#define FILL    64
#define F_FILL  FILL64


#ifdef ROCKBOX_BIG_ENDIAN
# define SWHI   swl                /* high part is left in big-endian        */
#else
# define SWHI   swr                /* high part is right in little-endian    */
#endif

/*
 * memset(void *s, int c, size_t n)
 *
 * a0: start of area to clear
 * a1: char to fill with
 * a2: size of area to clear
 */
    .section   .icode, "ax", %progbits

    .global    memset
    .type      memset, %function
    
    .set       noreorder
    .align     5
memset:
    beqz    a1, 1f
    move    v0, a0                /* result */

    andi    a1, 0xff              /* spread fillword */
    sll     t1, a1, 8
    or      a1, t1
    sll     t1, a1, 16
    or      a1, t1
1:

    sltiu   t0, a2, 4             /* very small region? */
    bnez    t0, small_memset
    andi    t0, a0, 3             /* aligned? */

    beqz    t0, 1f
    subu    t0, 4                 /* alignment in bytes */

    SWHI    a1, (a0)              /* make word aligned */
    subu    a0, t0                /* word align ptr */
    addu    a2, t0                /* correct size */

1:  ori     t1, a2, (FILL-1)      /* # of full blocks */
    xori    t1, (FILL-1)
    beqz    t1, memset_partial    /* no block to fill */
    andi    t0, a2, (FILL-4)

    addu    t1, a0                /* end address */
    .set    reorder
1:  addiu   a0, FILL
    F_FILL( a0, -FILL, a1 )
    bne     t1, a0, 1b
    .set    noreorder

memset_partial:
    la      t1, 2f                /* where to start */
    subu    t1, t0
    jr      t1
    addu    a0, t0                /* dest ptr */

    F_FILL( a0, -FILL, a1 )       /* ... but first do words ... */
2:  andi    a2, 3                 /* 0 <= n <= 3 to go */

    beqz    a2, 1f
    addu    a0, a2                /* What's left */
    SWHI    a1, -1(a0)
1:  jr      ra
    move    a2, zero

small_memset:
    beqz    a2, 2f
    addu    t1, a0, a2

1:  addiu   a0, 1                 /* fill bytewise */
    bne     t1, a0, 1b
    sb      a1, -1(a0)

2:  jr      ra                    /* done */
    move    a2, zero

    .set       reorder

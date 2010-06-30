/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2010 by Michael Sevakis
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

/***************************************************************************
 * void bitmod32(volatile uint32_t *addr, uint32_t bits, uint32_t mask)
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     bitmod32
    .type       bitmod32, %function
bitmod32:
    mrs    r12, cpsr
    cpsid  if
    ldr    r3, [r0]
    and    r1, r1, r2               @ Only allow mod of bits in 'mask'
    bic    r3, r3, r2               @ Clear mask bits
    orr    r3, r3, r1               @ Set according to 'bits'
    str    r3, [r0]
    msr    cpsr_c, r12
    bx     lr
    .size  bitmod32, .-bitmod32

/***************************************************************************
 * void bitset32(volatile uint32_t *addr, uint32_t mask)
 */
    .section    .text, "ax", %progbits
    .align      2
    .global     bitset32
    .type       bitset32, %function
bitset32:
    mrs    r12, cpsr
    cpsid  if
    ldr    r2, [r0]
    orr    r2, r2, r1
    str    r2, [r0]
    msr    cpsr_c, r12
    bx     lr
    .size  bitset32, .-bitset32


/***************************************************************************
 * void bitclr32(volatile uint32_t *addr, uint32_t mask)
 */
     .section    .text, "ax", %progbits
     .align      2
     .global     bitclr32
     .type       bitclr32, %function
bitclr32:
    mrs    r12, cpsr
    cpsid  if
    ldr    r2, [r0]
    bic    r2, r2, r1
    str    r2, [r0]
    msr    cpsr_c, r12
    bx     lr
    .size  bitclr32, .-bitclr32

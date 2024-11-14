/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
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
#ifndef SYSTEM_ARM_H
#define SYSTEM_ARM_H

#if ARM_PROFILE == ARM_PROFILE_CLASSIC
# include "system-arm-classic.h"
#elif ARM_PROFILE == ARM_PROFILE_MICRO
# include "system-arm-micro.h"
#else
# error "Unknown ARM architecture profile!"
#endif

/* Common to all ARM_ARCH */
#define nop \
  asm volatile ("nop")

#if defined(ARM_NEED_DIV0)
void __div0(void);
#endif

#if ARM_ARCH >= 6
static inline uint16_t swap16_hw(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    uint32_t retval;
    asm ("revsh %0, %1"                         /* xxAB */
        : "=r"(retval) : "r"((uint32_t)value)); /* xxBA */
    return retval;
}

static inline uint32_t swap32_hw(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t retval;
    asm ("rev %0, %1"                 /* ABCD */
        : "=r"(retval) : "r"(value)); /* DCBA */
    return retval;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t retval;
    asm ("rev16 %0, %1"               /* ABCD */
        : "=r"(retval) : "r"(value)); /* BADC */
    return retval;
}

#else /* ARM_ARCH < 6 */

static inline uint16_t swap16_hw(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32_hw(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
#ifdef __thumb__
    uint32_t mask = 0x00FF00FF;
    asm (                            /* val  = ABCD */
        "and %1, %0              \n" /* mask = .B.D */
        "eor %0, %1              \n" /* val  = A.C. */
        "lsl %1, #8              \n" /* mask = B.D. */
        "lsr %0, #8              \n" /* val  = .A.C */
        "orr %0, %1              \n" /* val  = BADC */
        "mov %1, #16             \n" /* mask = 16   */
        "ror %0, %1              \n" /* val  = DCBA */
        : "+l"(value), "+l"(mask));
#else
    uint32_t tmp;
    asm (
        "eor %1, %0, %0, ror #16 \n"
        "bic %1, %1, #0xff0000   \n"
        "mov %0, %0, ror #8      \n"
        "eor %0, %0, %1, lsr #8  \n"
        : "+r" (value), "=r" (tmp));
#endif
    return value;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
#ifdef __thumb__
    uint32_t mask = 0x00FF00FF;
    asm (                            /* val  = ABCD */
        "and %1, %0             \n"  /* mask = .B.D */
        "eor %0, %1             \n"  /* val  = A.C. */
        "lsl %1, #8             \n"  /* mask = B.D. */
        "lsr %0, #8             \n"  /* val  = .A.C */
        "orr %0, %1             \n"  /* val  = BADC */
        : "+l"(value), "+l"(mask));
#else
    uint32_t tmp;
    asm (                            /* ABCD      */
        "bic %1, %0, #0x00ff00  \n"  /* AB.D      */
        "bic %0, %0, #0xff0000  \n"  /* A.CD      */
        "mov %0, %0, lsr #8     \n"  /* .A.C      */
        "orr %0, %0, %1, lsl #8 \n"  /* B.D.|.A.C */
        : "+r" (value), "=r" (tmp)); /* BADC      */
#endif
    return value;
}

#endif /* ARM_ARCH */

static inline uint32_t swaw32_hw(uint32_t value)
{
    /*
      result[31..16] = value[15.. 0];
      result[15.. 0] = value[31..16];
    */
#ifdef __thumb__
    asm (
        "ror %0, %1"
        : "+l"(value) : "l"(16));
    return value;
#else
    uint32_t retval;
    asm (
        "mov %0, %1, ror #16"
        : "=r"(retval) : "r"(value));
    return retval;
#endif

}

#endif /* SYSTEM_ARM_H */

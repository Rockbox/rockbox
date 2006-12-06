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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#define or_l(mask, address) \
  asm                       \
    ("or.l %0,(%1)"         \
     :                      \
     : /* %0 */ "d"(mask),  \
       /* %1 */ "a"(address))

#define and_l(mask, address) \
  asm                        \
    ("and.l %0,(%1)"         \
     :                       \
     : /* %0 */ "d"(mask),   \
       /* %1 */ "a"(address))

#define eor_l(mask, address) \
  asm                        \
    ("eor.l %0,(%1)"         \
     :                       \
     : /* %0 */ "d"(mask),   \
       /* %1 */ "a"(address))

#define EMAC_ROUND      0x10
#define EMAC_FRACTIONAL 0x20
#define EMAC_SATURATE   0x80

static inline void coldfire_set_macsr(unsigned long flags)
{
    asm volatile ("move.l %0, %%macsr" : : "i,r" (flags));
}

static inline unsigned long coldfire_get_macsr(void)
{
    unsigned long m;

    asm volatile ("move.l %%macsr, %0" : "=r" (m));
    return m;
}

#define HIGHEST_IRQ_LEVEL (7<<8)
static inline int set_irq_level(int level)
{
    int oldlevel;
    /* Read the old level and set the new one */
    asm volatile ("move.w %%sr,%0\n"
                  "or.l #0x2000,%1\n"
                  "move.w %1,%%sr\n" : "=d" (oldlevel), "+d" (level) : );
    return oldlevel;
}

static inline unsigned short swap16(unsigned short value)
  /*
    result[15..8] = value[ 7..0];
    result[ 7..0] = value[15..8];
  */
{
    return (value >> 8) | (value << 8);
}

static inline unsigned long SWAW32(unsigned long value)
  /*
    result[31..16] = value[15.. 0];
    result[15.. 0] = value[31..16];
  */
{
    asm ("swap    %%0" : "+r"(value));
    return value;
}

static inline unsigned long swap32(unsigned long value)
  /*
    result[31..24] = value[ 7.. 0];
    result[23..16] = value[15.. 8];
    result[15.. 8] = value[23..16];
    result[ 7.. 0] = value[31..24];
  */
{
    unsigned long mask = 0x00FF00FF;
    asm (                             /* val  = ABCD */
        "and.l   %[val],%[mask]  \n"  /* mask = .B.D */
        "eor.l   %[mask],%[val]  \n"  /* val  = A.C. */
        "lsl.l   #8,%[mask]      \n"  /* mask = B.D. */
        "lsr.l   #8,%[val]       \n"  /* val  = .A.C */
        "or.l    %[mask],%[val]  \n"  /* val  = BADC */
        "swap    %[val]          \n"  /* val  = DCBA */
        : /* outputs */
        [val] "+d"(value),
        [mask]"+d"(mask)
    );
    return value;
}

static inline unsigned long swap_odd_even32(unsigned long value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    unsigned long mask = 0x00FF00FF;

    asm (                             /* val  = ABCD */
        "and.l   %[val],%[mask]  \n"  /* mask = .B.D */
        "eor.l   %[mask],%[val]  \n"  /* val  = A.C. */
        "lsl.l   #8,%[mask]      \n"  /* mask = B.D. */
        "lsr.l   #8,%[val]       \n"  /* val  = .A.C */
        "or.l    %[mask],%[val]  \n"  /* val  = BADC */
        : /* outputs */
        [val] "+d"(value),
        [mask]"+d"(mask)
    );

    return value;
}

static inline void invalidate_icache(void)
{
   asm volatile ("move.l #0x01000000,%d0\n"
                 "movec.l %d0,%cacr\n"
                 "move.l #0x80000000,%d0\n"
                 "movec.l %d0,%cacr");
}

#define DEFAULT_PLLCR_AUDIO_BITS 0x10400000
void coldfire_set_pllcr_audio_bits(long bits);

/* 11.2896 MHz */
#define CPUFREQ_DEFAULT_MULT 1
#define CPUFREQ_DEFAULT      (CPUFREQ_DEFAULT_MULT * CPU_FREQ)
/* 45.1584 MHz */
#define CPUFREQ_NORMAL_MULT  4
#define CPUFREQ_NORMAL       (CPUFREQ_NORMAL_MULT * CPU_FREQ)
/* 124.1856 MHz */
#define CPUFREQ_MAX_MULT     11
#define CPUFREQ_MAX          (CPUFREQ_MAX_MULT * CPU_FREQ)

#endif /* SYSTEM_TARGET_H */

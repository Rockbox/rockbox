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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "cpu.h"
#include "config.h"
#include "stdbool.h"

extern void system_reboot (void);
extern void system_init(void);

extern long cpu_frequency;

#if CONFIG_CPU==PP5020
#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))
#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))
#define inw(a) (*(volatile unsigned short *) (a))
#define outw(a,b) (*(volatile unsigned short *) (b) = (a))
static inline void udelay(unsigned usecs)
{
    unsigned start = inl(0x60005010);
    while ((inl(0x60005010) - start) < usecs);
}
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#define FREQ cpu_frequency
void set_cpu_frequency(long frequency);
void cpu_boost(bool on_off);
void cpu_idle_mode(bool on_off);
#else
#define FREQ CPU_FREQ
#define set_cpu_frequency(frequency)
#define cpu_boost(on_off)
#define cpu_idle_mode(on_off)
#endif

#define BAUDRATE 9600

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#ifdef ROCKBOX_LITTLE_ENDIAN
#define letoh16(x) (x)
#define letoh32(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#define betoh16(x) swap16(x)
#define betoh32(x) swap32(x)
#define htobe16(x) swap16(x)
#define htobe32(x) swap32(x)
#else 
#define letoh16(x) swap16(x)
#define letoh32(x) swap32(x)
#define htole16(x) swap16(x)
#define htole32(x) swap32(x)
#define betoh16(x) (x)
#define betoh32(x) (x)
#define htobe16(x) (x)
#define htobe32(x) (x)
#endif


#define nop \
  asm volatile ("nop")

/* gcc 3.4 changed the format of the constraints */
#if (__GNUC__ >= 3) && (__GNUC_MINOR__ > 3) || (__GNUC__ >= 4)
#define I_CONSTRAINT "I08"
#else
#define I_CONSTRAINT "I"
#endif

/* Utilize the user break controller to catch invalid memory accesses. */
int system_memory_guard(int newmode);

enum {
    MEMGUARD_KEEP = -1,    /* don't change the mode; for reading */
    MEMGUARD_NONE = 0,     /* catch nothing */
    MEMGUARD_FLASH_WRITES, /* catch writes to area 02 (flash ROM) */
    MEMGUARD_ZERO_AREA,    /* catch all accesses to areas 00 and 01 */
    MAXMEMGUARD
};


#if CONFIG_CPU == SH7034
#define or_b(mask, address) \
  asm                                       \
    ("or.b %0,@(r0,gbr)"                    \
     :                                      \
     : /* %0 */ I_CONSTRAINT((char)(mask)), \
       /* %1 */ "z"(address-GBR))

#define and_b(mask, address) \
  asm                                       \
    ("and.b %0,@(r0,gbr)"                   \
     :                                      \
     : /* %0 */ I_CONSTRAINT((char)(mask)), \
       /* %1 */ "z"(address-GBR))

#define xor_b(mask, address) \
  asm                                        \
    ("xor.b %0,@(r0,gbr)"                    \
     :                                       \
     : /* %0 */ I_CONSTRAINT((char)(mask)),  \
       /* %1 */ "z"(address-GBR))

#elif defined(CPU_COLDFIRE)
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

#endif

#ifndef SIMULATOR

/****************************************************************************
 * Interrupt level setting
 * The level is left shifted 4 bits
 ****************************************************************************/
#if CONFIG_CPU == SH7034
#define HIGHEST_IRQ_LEVEL (15<<4)
static inline int set_irq_level(int level)
{
    int i;
    /* Read the old level and set the new one */
    asm volatile ("stc sr, %0" : "=r" (i));
    asm volatile ("ldc %0, sr" : : "r" (level));
    return i;
}

static inline unsigned short swap16(unsigned short value)
  /*
    result[15..8] = value[ 7..0];
    result[ 7..0] = value[15..8];
  */
{
    unsigned short result;
    asm volatile ("swap.b\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline unsigned long SWAW32(unsigned long value)
  /*
    result[31..16] = value[15.. 0];
    result[15.. 0] = value[31..16];
  */
{
    unsigned long result;
    asm volatile ("swap.w\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline unsigned long swap32(unsigned long value)
  /*
    result[31..24] = value[ 7.. 0];
    result[23..16] = value[15.. 8];
    result[15.. 8] = value[23..16];
    result[ 7.. 0] = value[31..24];
  */
{
    asm volatile ("swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n"
                  "swap.b\t%0,%0\n" : "+r"(value));
    return value;
}

#define invalidate_icache()

#elif defined(CPU_COLDFIRE)
#define HIGHEST_IRQ_LEVEL (7<<8)
static inline int set_irq_level(int level)
{
    int oldlevel;
    /* Read the old level and set the new one */
    asm volatile ("move.w %%sr,%0\n"
                  "or.l #0x2000,%1\n"
                  "move.w %1,%%sr\n" : "=r" (oldlevel), "+r" (level) : );
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

static inline void invalidate_icache(void)
{
   asm volatile ("move.l #0x01000000,%d0\n"
                 "movec.l %d0,%cacr\n"
                 "move.l #0x80000000,%d0\n"
                 "movec.l %d0,%cacr");
}
            
#define CPUFREQ_DEFAULT_MULT 1
#define CPUFREQ_DEFAULT      (CPUFREQ_DEFAULT_MULT * CPU_FREQ)
#define CPUFREQ_NORMAL_MULT  4
#define CPUFREQ_NORMAL       (CPUFREQ_NORMAL_MULT * CPU_FREQ)
#define CPUFREQ_MAX_MULT     11
#define CPUFREQ_MAX          (CPUFREQ_MAX_MULT * CPU_FREQ)

#elif defined(CPU_ARM)

/* TODO: Implement set_irq_level and check CPU frequencies */

#define CPUFREQ_DEFAULT CPU_FREQ
#define CPUFREQ_NORMAL 37500000
#define CPUFREQ_MAX 75000000

static inline unsigned short swap16(unsigned short value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline unsigned long swap32(unsigned long value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    unsigned long hi = swap16(value >> 16);
    unsigned long lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

#define HIGHEST_IRQ_LEVEL (1)

static inline int set_irq_level(int level)
{
    unsigned long cpsr;
    /* Read the old level and set the new one */
    asm volatile ("mrs %0,cpsr" : "=r" (cpsr));
    asm volatile ("msr cpsr_c,%0"
                  : : "r" ((cpsr & ~0x80) | (level << 7)));
    return (cpsr >> 7) & 1;
}

#define invalidate_icache()

#if CONFIG_CPU == PNX0101
typedef void (*interrupt_handler_t)(void);

void irq_set_int_handler(int n, interrupt_handler_t handler);
void irq_enable_int(int n);
#endif

#elif CONFIG_CPU == TCC730

extern int smsc_version(void);

extern void smsc_delay(void);

extern void set_pll_freq(int pll_index, long freq_out);


extern void* volatile interrupt_vector[16]  __attribute__ ((section(".idata")));

extern void ddma_transfer(int dir, int mem, void* intAddr, long extAddr,
                          int num);


#define HIGHEST_IRQ_LEVEL (1)
static inline int set_irq_level(int level)
{
  int result;
  __asm__ ("ld %0, 0\n\t"
	   "tstsr ie\n\t"
	   "incc %0" : "=r"(result));
  if (level > 0)
    __asm__ volatile ("clrsr ie");
  else
    __asm__ volatile ("setsr ie");
    
  return result;
}

static inline unsigned short swap16(unsigned short value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline unsigned long swap32(unsigned long value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    unsigned long hi = swap16(value >> 16);
    unsigned long lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

/* Archos uses:

22MHz: busy wait on dma
32MHz: normal
80Mhz: heavy load

*/

#define CPUFREQ_DEFAULT CPU_FREQ
#define CPUFREQ_NORMAL (32000000)
#define CPUFREQ_MAX    (80000000)

#define invalidate_icache()

#endif
#else

static inline unsigned short swap16(unsigned short value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline unsigned long swap32(unsigned long value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    unsigned long hi = swap16(value >> 16);
    unsigned long lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}


#define invalidate_icache()

#endif

#endif

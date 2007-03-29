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
#include "stdbool.h"
#include "kernel.h"

extern void system_reboot (void);
extern void system_init(void);

extern long cpu_frequency;

#ifdef CPU_PP
#define inl(a) (*(volatile unsigned long *) (a))
#define outl(a,b) (*(volatile unsigned long *) (b) = (a))
#define inb(a) (*(volatile unsigned char *) (a))
#define outb(a,b) (*(volatile unsigned char *) (b) = (a))
#define inw(a) (*(volatile unsigned short *) (a))
#define outw(a,b) (*(volatile unsigned short *) (b) = (a))
extern unsigned int ipod_hw_rev;

static inline void udelay(unsigned usecs)
{
    unsigned stop = USEC_TIMER + usecs;
    while (TIME_BEFORE(USEC_TIMER, stop));
}

unsigned int current_core(void);
#endif

struct flash_header {
    unsigned long magic;
    unsigned long length;
    char version[32];
};

bool detect_flashed_romimage(void);
bool detect_flashed_ramimage(void);
bool detect_original_firmware(void);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ) \
        && defined(ROCKBOX_HAS_LOGF) && (NUM_CORES == 1)
#define CPU_BOOST_LOGGING
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#define FREQ cpu_frequency
void set_cpu_frequency(long frequency);
#ifdef CPU_BOOST_LOGGING
char * cpu_boost_log_getlog_first(void);
char * cpu_boost_log_getlog_next(void);
int cpu_boost_log_getcount(void);
void cpu_boost_(bool on_off, char* location, int line);
#else
void cpu_boost(bool on_off);
#endif
void cpu_idle_mode(bool on_off);
int get_cpu_boost_counter(void);
#else
#define FREQ CPU_FREQ
#define set_cpu_frequency(frequency)
#define cpu_boost(on_off)
#define cpu_boost_id(on_off, id)
#define cpu_idle_mode(on_off)
#define get_cpu_boost_counter()
#define get_cpu_boost_tracker()
#endif

#ifdef CPU_BOOST_LOGGING
#define cpu_boost(on_off) cpu_boost_(on_off,__FILE__,  __LINE__)
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

/* return number of elements in array a */
#define ARRAYLEN(a) (sizeof(a)/sizeof((a)[0]))

/* return p incremented by specified number of bytes */
#define SKIPBYTES(p, count) ((typeof (p))((char *)(p) + (count)))

#define P2_M1(p2)  ((1 << (p2))-1)

/* align up or down to nearest 2^p2 */
#define ALIGN_DOWN_P2(n, p2) ((n) & ~P2_M1(p2))
#define ALIGN_UP_P2(n, p2)   ALIGN_DOWN_P2((n) + P2_M1(p2),p2)

/* align up or down to nearest integer multiple of a */
#define ALIGN_DOWN(n, a)     ((n)/(a)*(a))
#define ALIGN_UP(n, a)       ALIGN_DOWN((n)+((a)-1),a)

/* live endianness conversion */
#ifdef ROCKBOX_LITTLE_ENDIAN
#define letoh16(x) (x)
#define letoh32(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#define betoh16(x) swap16(x)
#define betoh32(x) swap32(x)
#define htobe16(x) swap16(x)
#define htobe32(x) swap32(x)
#define swap_odd_even_be32(x) (x)
#define swap_odd_even_le32(x) swap_odd_even32(x)
#else
#define letoh16(x) swap16(x)
#define letoh32(x) swap32(x)
#define htole16(x) swap16(x)
#define htole32(x) swap32(x)
#define betoh16(x) (x)
#define betoh32(x) (x)
#define htobe16(x) (x)
#define htobe32(x) (x)
#define swap_odd_even_be32(x) swap_odd_even32(x)
#define swap_odd_even_le32(x) (x)
#endif

/* static endianness conversion */
#define SWAP_16(x) ((typeof(x))(unsigned short)(((unsigned short)(x) >> 8) | \
                                                ((unsigned short)(x) << 8)))

#define SWAP_32(x) ((typeof(x))(unsigned long)( ((unsigned long)(x) >> 24) | \
                                               (((unsigned long)(x) & 0xff0000ul) >> 8) | \
                                               (((unsigned long)(x) & 0xff00ul) << 8) | \
                                                ((unsigned long)(x) << 24)))

#ifdef ROCKBOX_LITTLE_ENDIAN
#define LE_TO_H16(x) (x)
#define LE_TO_H32(x) (x)
#define H_TO_LE16(x) (x)
#define H_TO_LE32(x) (x)
#define BE_TO_H16(x) SWAP_16(x)
#define BE_TO_H32(x) SWAP_32(x)
#define H_TO_BE16(x) SWAP_16(x)
#define H_TO_BE32(x) SWAP_32(x)
#else
#define LE_TO_H16(x) SWAP_16(x)
#define LE_TO_H32(x) SWAP_32(x)
#define H_TO_LE16(x) SWAP_16(x)
#define H_TO_LE32(x) SWAP_32(x)
#define BE_TO_H16(x) (x)
#define BE_TO_H32(x) (x)
#define H_TO_BE16(x) (x)
#define H_TO_BE32(x) (x)
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

#ifndef SIMULATOR
#ifdef CPU_COLDFIRE
#include "system-target.h"
#endif
#endif
#ifndef SIMULATOR
#if CONFIG_CPU == S3C2440
#include "system-target.h"
#endif
#endif

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


#endif /* CONFIG_CPU == SH7034 */

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

static inline uint16_t swap16(uint16_t value)
  /*
    result[15..8] = value[ 7..0];
    result[ 7..0] = value[15..8];
  */
{
    uint16_t result;
    asm volatile ("swap.b\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline uint32_t SWAW32(uint32_t value)
  /*
    result[31..16] = value[15.. 0];
    result[15.. 0] = value[31..16];
  */
{
    uint32_t result;
    asm volatile ("swap.w\t%1,%0" : "=r"(result) : "r"(value));
    return result;
}

static inline uint32_t swap32(uint32_t value)
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

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    asm volatile ("swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n"
                  "swap.b\t%0,%0\n"
                  "swap.w\t%0,%0\n" : "+r"(value));
    return value;
}

#define invalidate_icache()

#elif defined(CPU_ARM)

/* TODO: Implement set_irq_level and check CPU frequencies */

#if CONFIG_CPU == S3C2440

#define CPUFREQ_DEFAULT 98784000
#define CPUFREQ_NORMAL  98784000
#define CPUFREQ_MAX    296352000

#elif CONFIG_CPU == PNX0101

#define CPUFREQ_DEFAULT 12000000
#define CPUFREQ_NORMAL  48000000
#define CPUFREQ_MAX     60000000

#else

#define CPUFREQ_DEFAULT_MULT 8
#define CPUFREQ_DEFAULT 24000000
#define CPUFREQ_NORMAL_MULT 10
#define CPUFREQ_NORMAL 30000000
#define CPUFREQ_MAX_MULT 25
#define CPUFREQ_MAX 75000000

#endif

static inline uint16_t swap16(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t tmp;

    asm volatile (
        "eor %1, %0, %0, ror #16 \n\t"
        "bic %1, %1, #0xff0000   \n\t"
        "mov %0, %0, ror #8      \n\t"
        "eor %0, %0, %1, lsr #8  \n\t"
        : "+r" (value), "=r" (tmp)
    );
    return value;
}

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t tmp;

    asm volatile (                    /* ABCD      */
        "bic %1, %0, #0x00ff00  \n\t" /* AB.D      */
        "bic %0, %0, #0xff0000  \n\t" /* A.CD      */
        "mov %0, %0, lsr #8     \n\t" /* .A.C      */
        "orr %0, %0, %1, lsl #8 \n\t" /* B.D.|.A.C */
        : "+r" (value), "=r" (tmp)    /* BADC      */
    );
    return value;
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

static inline void set_fiq_handler(void(*fiq_handler)(void))
{
    /* Install the FIQ handler */
    *((unsigned int*)(15*4)) = (unsigned int)fiq_handler;
}

static inline void enable_fiq(void)
{
    /* Clear FIQ disable bit */
    asm volatile (
        "mrs     r0, cpsr         \n"\
        "bic     r0, r0, #0x40    \n"\
        "msr     cpsr_c, r0         "
        : : : "r0"
    );
}

static inline void disable_fiq(void)
{
    /* Set FIQ disable bit */
    asm volatile (
        "mrs     r0, cpsr         \n"\
        "orr     r0, r0, #0x40    \n"\
        "msr     cpsr_c, r0         "
        : : : "r0"
    );
}

#if CONFIG_CPU != S3C2440
#define invalidate_icache()
#endif

#if CONFIG_CPU == PNX0101
typedef void (*interrupt_handler_t)(void);

void irq_set_int_handler(int n, interrupt_handler_t handler);
void irq_enable_int(int n);
void irq_disable_int(int n);
#endif

#endif

#else /* SIMULATOR */

static inline uint16_t swap16(uint16_t value)
    /*
      result[15..8] = value[ 7..0];
      result[ 7..0] = value[15..8];
    */
{
    return (value >> 8) | (value << 8);
}

static inline uint32_t swap32(uint32_t value)
    /*
      result[31..24] = value[ 7.. 0];
      result[23..16] = value[15.. 8];
      result[15.. 8] = value[23..16];
      result[ 7.. 0] = value[31..24];
    */
{
    uint32_t hi = swap16(value >> 16);
    uint32_t lo = swap16(value & 0xffff);
    return (lo << 16) | hi;
}

static inline uint32_t swap_odd_even32(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t t = value & 0xff00ff00;
    return (t >> 8) | ((t ^ value) << 8);
}

#define invalidate_icache()

#endif /* !SIMULATOR */

#endif /* __SYSTEM_H__ */

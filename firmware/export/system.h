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
#if NUM_CORES > 1
extern struct spinlock boostctrl_spin;
#endif
void cpu_boost_init(void);
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
#include "system-target.h"
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

#endif /* !SIMULATOR */

/* Declare this as HIGHEST_IRQ_LEVEL if they don't differ */
#ifndef DISABLE_INTERRUPTS
#define DISABLE_INTERRUPTS  HIGHEST_IRQ_LEVEL
#endif

/* Just define these as empty if not declared */
#ifndef HAVE_INVALIDATE_ICACHE
#define invalidate_icache()
#endif

#ifndef HAVE_FLUSH_ICACHE
#define flush_icache()
#endif

#ifdef PROC_NEEDS_CACHEALIGN
/* Cache alignment attributes and sizes are enabled */

/* 2^CACHEALIGN_BITS = the byte size */
#define CACHEALIGN_SIZE (1u << CACHEALIGN_BITS)

#define CACHEALIGN_ATTR __attribute__((aligned(CACHEALIGN_SIZE)))
/* Aligns x up to a CACHEALIGN_SIZE boundary */
#define CACHEALIGN_UP(x) \
    ((typeof (x))ALIGN_UP_P2((uintptr_t)(x), CACHEALIGN_BITS))
/* Aligns x down to a CACHEALIGN_SIZE boundary */
#define CACHEALIGN_DOWN(x) \
    ((typeof (x))ALIGN_DOWN_P2((uintptr_t)(x), CACHEALIGN_BITS))
/* Aligns at least to the greater of size x or CACHEALIGN_SIZE */
#define CACHEALIGN_AT_LEAST_ATTR(x) \
    __attribute__((aligned(CACHEALIGN_UP(x))))
/* Aligns a buffer pointer and size to proper boundaries */
#define CACHEALIGN_BUFFER(start, size) \
    ({ align_buffer(PUN_PTR(void **, (start)), (size), CACHEALIGN_SIZE); })

#else /* ndef PROC_NEEDS_CACHEALIGN */

/* Cache alignment attributes and sizes are not enabled */
#define CACHEALIGN_ATTR
#define CACHEALIGN_AT_LEAST_ATTR(x) \
    __attribute__((aligned(x)))
#define CACHEALIGN_UP(x) (x)
#define CACHEALIGN_DOWN(x) (x)
/* Make no adjustments */
#define CACHEALIGN_BUFFER(start, size) \
    ({ (void)(start); (size); })

#endif /* PROC_NEEDS_CACHEALIGN */

/* Double-cast to avoid 'dereferencing type-punned pointer will
 * break strict aliasing rules' B.S. */
#define PUN_PTR(type, p) ((type)(intptr_t)(p))

#endif /* __SYSTEM_H__ */

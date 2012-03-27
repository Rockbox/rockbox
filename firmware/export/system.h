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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "cpu.h"
#include "stdbool.h"
#include "kernel.h"
#include "gcc_extensions.h" /* for LIKELY/UNLIKELY */

extern void system_reboot (void);
/* Called from any UIE handler and panicf - wait for a key and return
 * to reboot system. */
extern void system_exception_wait(void);
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
#else /* ndef HAVE_ADJUSTABLE_CPU_FREQ */
#ifndef FREQ
#define FREQ CPU_FREQ
#endif
#define set_cpu_frequency(frequency)
#define cpu_boost(on_off)
#define cpu_boost_id(on_off, id)
#define cpu_idle_mode(on_off)
#define get_cpu_boost_counter()
#define get_cpu_boost_tracker()
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

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
#define ALIGN_DOWN(n, a)     ((typeof(n))((typeof(a))(n)/(a)*(a)))
#define ALIGN_UP(n, a)       ALIGN_DOWN((n)+((a)-1),a)

/* align start and end of buffer to nearest integer multiple of a */
#define ALIGN_BUFFER(ptr,len,align) \
{\
    uintptr_t tmp_ptr1 = (uintptr_t)ptr; \
    uintptr_t tmp_ptr2 = tmp_ptr1 + len;\
    tmp_ptr1 = ALIGN_UP(tmp_ptr1,align); \
    tmp_ptr2 = ALIGN_DOWN(tmp_ptr2,align); \
    len = tmp_ptr2 - tmp_ptr1; \
    ptr = (typeof(ptr))tmp_ptr1; \
}

#define PTR_ADD(ptr, x) ((typeof(ptr))((char*)(ptr) + (x)))
#define PTR_SUB(ptr, x) ((typeof(ptr))((char*)(ptr) - (x)))

/* newer? SDL includes endian.h, So we ignore it */
#if (CONFIG_PLATFORM & PLATFORM_HOSTED) || defined(__PCTOOL__)
#undef letoh16
#undef letoh32
#undef htole16
#undef htole32
#undef betoh16
#undef betoh32
#undef htobe16
#undef htobe32
#endif

/* Android NDK contains swap16 and swap32, ignore them */
#if (CONFIG_PLATFORM & PLATFORM_ANDROID)
#undef swap16
#undef swap32
#endif

/* Get the byte offset of a type's member */
#define OFFSETOF(type, membername) ((off_t)&((type *)0)->membername)

/* Get the type pointer from one of its members */
#define TYPE_FROM_MEMBER(type, memberptr, membername) \
    ((type *)((intptr_t)(memberptr) - OFFSETOF(type, membername)))

/* returns index of first set bit or 32 if no bits are set */
int find_first_set_bit(uint32_t val);

static inline __attribute__((always_inline))
uint32_t isolate_first_bit(uint32_t val)
    { return val & -val; }

/* Functions to set and clear register or variable bits atomically */
void bitmod16(volatile uint16_t *addr, uint16_t bits, uint16_t mask);
void bitset16(volatile uint16_t *addr, uint16_t mask);
void bitclr16(volatile uint16_t *addr, uint16_t mask);

void bitmod32(volatile uint32_t *addr, uint32_t bits, uint32_t mask);
void bitset32(volatile uint32_t *addr, uint32_t mask);
void bitclr32(volatile uint32_t *addr, uint32_t mask);

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

#if !defined(SIMULATOR) && !defined(__PCTOOL__) 
#include "system-target.h"
#elif defined(HAVE_SDL) /* SDL build */
#include "system-sdl.h"
#define NEED_GENERIC_BYTESWAPS
#elif defined(__PCTOOL__)
#include "system-sdl.h"
#define NEED_GENERIC_BYTESWAPS
#endif
#include "bitswap.h"

#ifdef NEED_GENERIC_BYTESWAPS
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
    uint32_t hi = swap16_hw(value >> 16);
    uint32_t lo = swap16_hw(value & 0xffff);
    return (lo << 16) | hi;
}

static inline uint32_t swap_odd_even32_hw(uint32_t value)
{
    /*
      result[31..24],[15.. 8] = value[23..16],[ 7.. 0]
      result[23..16],[ 7.. 0] = value[31..24],[15.. 8]
    */
    uint32_t t = value & 0xff00ff00;
    return (t >> 8) | ((t ^ value) << 8);
}

static inline uint32_t swaw32_hw(uint32_t value)
{
    /*
      result[31..16] = value[15.. 0];
      result[15.. 0] = value[31..16];
    */
    return (value >> 16) | (value << 16);
}

#endif /* NEED_GENERIC_BYTESWAPS */

/* static endianness conversion */
#define SWAP16_CONST(x) \
    ((typeof(x))( ((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8) ))

#define SWAP32_CONST(x) \
    ((typeof(x))( ((uint32_t)(x) >> 24) | \
                 (((uint32_t)(x) & 0xff0000) >> 8) | \
                 (((uint32_t)(x) & 0xff00) << 8) | \
                  ((uint32_t)(x) << 24) ))

#define SWAP_ODD_EVEN32_CONST(x) \
    ((typeof(x))( ((uint32_t)SWAP16_CONST((uint32_t)(x) >> 16) << 16) | \
                             SWAP16_CONST((uint32_t)(x))) )

#define SWAW32_CONST(x) \
    ((typeof(x))( ((uint32_t)(x) << 16) | ((uint32_t)(x) >> 16) ))

/* Select best method based upon whether x is a constant expression */
#define swap16(x) \
    ( __builtin_constant_p(x) ? SWAP16_CONST(x) : (typeof(x))swap16_hw(x) )

#define swap32(x) \
    ( __builtin_constant_p(x) ? SWAP32_CONST(x) : (typeof(x))swap32_hw(x) )

#define swap_odd_even32(x) \
    ( __builtin_constant_p(x) ? SWAP_ODD_EVEN32_CONST(x) : (typeof(x))swap_odd_even32_hw(x) )

#define swaw32(x) \
    ( __builtin_constant_p(x) ? SWAW32_CONST(x) : (typeof(x))swaw32_hw(x) )


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

#ifndef BIT_N
#define BIT_N(n) (1U << (n))
#endif

#ifndef MASK_N
/* Make a mask of n contiguous bits, shifted left by 'shift' */
#define MASK_N(type, n, shift) \
    ((type)((((type)1 << (n)) - (type)1) << (shift)))
#endif

/* Declare this as HIGHEST_IRQ_LEVEL if they don't differ */
#ifndef DISABLE_INTERRUPTS
#define DISABLE_INTERRUPTS  HIGHEST_IRQ_LEVEL
#endif

/* Define this, if the CPU may take advantage of cache aligment. Is enabled
 * for all ARM CPUs. */
#ifdef CPU_ARM
    #define HAVE_CPU_CACHE_ALIGN
#endif

/* Calculate CACHEALIGN_SIZE from CACHEALIGN_BITS */
#ifdef CACHEALIGN_SIZE
    /* undefine, if defined. always calculate from CACHEALIGN_BITS */
    #undef CACHEALIGN_SIZE
#endif
#ifdef CACHEALIGN_BITS
    /* CACHEALIGN_SIZE = 2 ^ CACHEALIGN_BITS */
    #define CACHEALIGN_SIZE (1u << CACHEALIGN_BITS)
#else
    /* FIXME: set to maximum known cache alignment of supported CPUs */
    #define CACHEALIGN_BITS  5
    #define CACHEALIGN_SIZE 32
#endif

#ifdef HAVE_CPU_CACHE_ALIGN
    /* Cache alignment attributes and sizes are enabled */
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
        ALIGN_BUFFER((start), (size), CACHEALIGN_SIZE)
#else
    /* Cache alignment attributes and sizes are not enabled */
    #define CACHEALIGN_ATTR
    #define CACHEALIGN_AT_LEAST_ATTR(x) __attribute__((aligned(x)))
    #define CACHEALIGN_UP(x) (x)
    #define CACHEALIGN_DOWN(x) (x)
    /* Make no adjustments */
    #define CACHEALIGN_BUFFER(start, size)
#endif

/* Define MEM_ALIGN_ATTR which may be used to align e.g. buffers for faster
 * access. */
#if defined(CPU_ARM)
    /* Use ARMs cache alignment. */
    #define MEM_ALIGN_ATTR CACHEALIGN_ATTR
    #define MEM_ALIGN_SIZE CACHEALIGN_SIZE
#elif defined(CPU_COLDFIRE)
    /* Use fixed alignment of 16 bytes. Speed up only for 'movem' in DRAM. */
    #define MEM_ALIGN_ATTR __attribute__((aligned(16)))
    #define MEM_ALIGN_SIZE 16
#else
    /* Align pointer size */
    #define MEM_ALIGN_ATTR __attribute__((aligned(sizeof(intptr_t))))
    #define MEM_ALIGN_SIZE sizeof(intptr_t)
#endif

#define MEM_ALIGN_UP(x) \
    ((typeof (x))ALIGN_UP((uintptr_t)(x), MEM_ALIGN_SIZE))
#define MEM_ALIGN_DOWN(x) \
    ((typeof (x))ALIGN_DOWN((uintptr_t)(x), MEM_ALIGN_SIZE))

#ifdef STORAGE_WANTS_ALIGN
    #define STORAGE_ALIGN_ATTR __attribute__((aligned(CACHEALIGN_SIZE)))
    #define STORAGE_ALIGN_DOWN(x) \
        ((typeof (x))ALIGN_DOWN_P2((uintptr_t)(x), CACHEALIGN_BITS))
    /* Pad a size so the buffer can be aligned later */
    #define STORAGE_PAD(x) ((x) + CACHEALIGN_SIZE - 1)
    /* Number of bytes in the last cacheline assuming buffer of size x is aligned */
    #define STORAGE_OVERLAP(x) ((x) & (CACHEALIGN_SIZE - 1))
    #define STORAGE_ALIGN_BUFFER(start, size) \
        ALIGN_BUFFER((start), (size), CACHEALIGN_SIZE)
#else
    #define STORAGE_ALIGN_ATTR
    #define STORAGE_ALIGN_DOWN(x) (x)
    #define STORAGE_PAD(x) (x)
    #define STORAGE_OVERLAP(x) 0
    #define STORAGE_ALIGN_BUFFER(start, size)
#endif

/* Double-cast to avoid 'dereferencing type-punned pointer will
 * break strict aliasing rules' B.S. */
#define PUN_PTR(type, p) ((type)(intptr_t)(p))

#ifndef SIMULATOR
bool dbg_ports(void);
#endif
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
bool dbg_hw_info(void);
#endif

#endif /* __SYSTEM_H__ */

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

#include <stdbool.h>
#include <stdint.h>
#include "cpu.h"
#include "gcc_extensions.h" /* for LIKELY/UNLIKELY */

extern void system_reboot (void);
/* Called from any UIE handler and panicf - wait for a key and return
 * to reboot system. */
extern void system_exception_wait(void);
extern void system_init(void);

extern long cpu_frequency;

struct flash_header {
    uint32_t magic;
    uint32_t length;
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

/* wrap-safe macros for tick comparison */
#define TIME_AFTER(a,b)         ((long)(b) - (long)(a) < 0)
#define TIME_BEFORE(a,b)        TIME_AFTER(b,a)

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

#ifndef SGN
#define SGN(a) \
    ({ typeof (a) ___a = (a); (___a > 0) - (___a < 0); })
#endif

/* return number of elements in array a */
#define ARRAYLEN(a) (sizeof(a)/sizeof((a)[0]))

/* is the given pointer "p" inside the said bounds of array "a"? */
#define PTR_IN_ARRAY(a, p, numelem) \
    ((uintptr_t)(p) - (uintptr_t)(a) < (uintptr_t)(numelem)*sizeof ((a)[0]))

/* return p incremented by specified number of bytes */
#define SKIPBYTES(p, count) ((typeof (p))((char *)(p) + (count)))

#define P2_M1(p2)  ((1 << (p2))-1)

/* align up or down to nearest 2^p2 */
#define ALIGN_DOWN_P2(n, p2) ((n) & ~P2_M1(p2))
#define ALIGN_UP_P2(n, p2)   ALIGN_DOWN_P2((n) + P2_M1(p2),p2)

/* align up or down to nearest integer multiple of a */
#define ALIGN_DOWN(n, a)     ((typeof(n))((uintptr_t)(n)/(a)*(a)))
#define ALIGN_UP(n, a)       ALIGN_DOWN((n)+((a)-1),a)

/* align start and end of buffer to nearest integer multiple of a */
#define ALIGN_BUFFER(ptr, size, align) \
({                                           \
    size_t    __sz = (size);                 \
    size_t   __ali = (align);                \
    uintptr_t __a1 = (uintptr_t)(ptr);       \
    uintptr_t __a2 = __a1 + __sz;            \
    __a1 = ALIGN_UP(__a1, __ali);            \
    __a2 = ALIGN_DOWN(__a2, __ali);          \
    (ptr)  = (typeof (ptr))__a1;             \
    (size) = __a2 > __a1 ?  __a2 - __a1 : 0; \
})

#define IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

#define PTR_ADD(ptr, x) ((typeof(ptr))((char*)(ptr) + (x)))
#define PTR_SUB(ptr, x) ((typeof(ptr))((char*)(ptr) - (x)))

#ifndef alignof
#define alignof __alignof__
#endif

/* Get the byte offset of a type's member */
#ifndef offsetof
#define offsetof(type, member)  __builtin_offsetof(type, member)
#endif

/* Get the containing item of *ptr in type */
#ifndef container_of
#define container_of(ptr, type, member) ({              \
    const typeof (((type *)0)->member) *__mptr = (ptr); \
    (type *)((void *)(__mptr) - offsetof(type, member)); })
#endif

/* returns index of first set bit or 32 if no bits are set */
#if defined(CPU_ARM) && ARM_ARCH >= 5 && !defined(__thumb__)
static inline int find_first_set_bit(uint32_t val)
    { return LIKELY(val) ? __builtin_ctz(val) : 32; }
#else
int find_first_set_bit(uint32_t val);
#endif

static inline __attribute__((always_inline))
uint32_t isolate_first_bit(uint32_t val)
    { return val & -val; }

/* Functions to set and clear register or variable bits atomically;
 * return value is the previous value of *addr */
uint16_t bitmod16(volatile uint16_t *addr, uint16_t bits, uint16_t mask);
uint16_t bitset16(volatile uint16_t *addr, uint16_t mask);
uint16_t bitclr16(volatile uint16_t *addr, uint16_t mask);

uint32_t bitmod32(volatile uint32_t *addr, uint32_t bits, uint32_t mask);
uint32_t bitset32(volatile uint32_t *addr, uint32_t mask);
uint32_t bitclr32(volatile uint32_t *addr, uint32_t mask);

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
#ifdef SIMULATOR
#include "system-sim.h"
#endif
#elif defined(__PCTOOL__)
#include "system-hosted.h"
#endif

#include "bitswap.h"
#include "rbendian.h"

#ifndef ASSERT_CPU_MODE
/* Very useful to have defined properly for your architecture */
#define ASSERT_CPU_MODE(mode, rstatus...) \
    ({ (void)(mode); rstatus; })
#endif

#ifndef CPU_MODE_THREAD_CONTEXT
#define CPU_MODE_THREAD_CONTEXT 0
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
#ifndef CPU_BOOST_LOCK_DEFINED
#define CPU_BOOST_LOCK_DEFINED
/* Compatibility defauls */
static inline bool cpu_boost_lock(void)
    { return true; }
static inline void cpu_boost_unlock(void)
    { }
#endif /* CPU_BOOST_LOCK */
#endif /* HAVE_ADJUSTABLE_CPU_FREQ */

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
    #define MIN_STACK_ALIGN 8
#endif

/* Define this if target has support for generating backtraces */
#ifdef CPU_ARM
    #define HAVE_RB_BACKTRACE
#endif

#ifndef MIN_STACK_ALIGN
#define MIN_STACK_ALIGN (sizeof (uintptr_t))
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
#if (CONFIG_PLATFORM & PLATFORM_NATIVE) || defined(SONY_NWZ_LINUX) || defined(HIBY_LINUX) || defined(FIIO_M3K)
bool dbg_hw_info(void);
#endif

#endif /* __SYSTEM_H__ */

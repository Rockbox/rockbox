/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2019 Sebastian Leonhardt
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

/*
  Prototypes for the memory interface.
  */

#ifndef MEMORY_H
#define MEMORY_H

#define VERSION_MEM    "2.0"

/* not actually the size, but the shift value */
#define MEM_CHUNK_SIZE  7   /* actual size is 1<<7 = 0x80 */

extern unsigned (*mapped_read[0x10000>>MEM_CHUNK_SIZE])(unsigned a);
extern void (*mapped_write[0x10000>>MEM_CHUNK_SIZE])(unsigned a, unsigned b);

#  define decWrite(a, b)    (*mapped_write[(a)>>MEM_CHUNK_SIZE])((a), (b))
#  define decRead(a)        (*mapped_read[(a)>>MEM_CHUNK_SIZE])(a)


/*
 * Bank Switching
 */
enum bank_switch_scheme {
    BSW_NONE = 0,
    BSW_F8,
    BSW_F6,
    BSW_E0_PARKER,
    BSW_FA_CBA_RAMPLUS,
    BSW_F6SC_SUPERCHIP,
    /* TODO: 3F Tigervision (8k) */
    BSW_END_OF_LIST
};

/* Bank switching scheme name/description for direct use as opt_items */
#define BSW_SCHEME_LIST_NAMES  \
    { "No Bank Switching", -1 }, \
    { "F8 (8k Atari Style)", -1 }, \
    { "F6 (16k Atari Style)", -1 }, \
    { "E0 (Parker Brothers 8k)", -1 }, \
    { "FA (CBS Ram Plus)", -1 }, \
    { "F6SC (Atari 16k + super chip ram)", -1 },

/* maximum size in kilobytes; for checking if scheme/actual size match */
#define BANK_SW_ROM_SZ(i)      ((int[]){ \
     4, \
     8, \
    16, \
     8, \
    12, \
    16, \
    })[i]

#define BSW_FILE_EXTENSIONS             \
    "",     /* not used - no bank switching*/   \
    ".F8",  /* 8k Atari style F8 */     \
    ".F6",  /* 16k Atari style F6 */    \
    ".E0",  /* Parker Brothers E0*/     \
    ".FA",  /* CBS Ram Plus */          \
    ".F6S", /* 16k Atari + Super Chip ram F6SC */


#define CART_RAM_SIZE       1024
#define CART_SCRATCH_SIZE   4096
#define CART_MAX_SIZE      16384

extern int rom_size;
extern BYTE theCart[CART_MAX_SIZE];
extern BYTE cartScratch[CART_SCRATCH_SIZE];
extern BYTE cartRam[CART_RAM_SIZE];
extern BYTE *theRom;
extern BYTE theRam[128];

void init_memory(void);
void init_banking (void);
void init_memory_map(void);

/* -------------------------------------------------------------------------
 * memory access macros for CPU
 */

/* macros for all OPT_CPU settings */
#   define LOAD(a)              decRead(a)
#   define STORE(a,b)           decWrite((a),(b))

#ifdef OPT_CPU_PARANOID
    /* handles wraparound in the middle of a 2-byte access */
// TODO: only used for JMP ind and for reading reset/brk vector; maybe unneccessary
#   define LOAD_ADDR16(a)       ((LOAD(((a)+1)&0xffff)<<8)+LOAD(a))
    /* mostly used in the form LOADEXEC(PC+1), so we want to ensure that
       the parameter never exceeds it's 16 bit range. */
#   define LOADEXEC(a)          LOAD((a) & 0xffff)
#   define LOADEXEC16(a)            (LOADEXEC ((a)&0xffff) | (LOADEXEC (((a)+1)&0xffff) << 8))
    /* handle wraparound inside zeropage */
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
    /* no special handling for executable memory/stack/zeropage. LOAD/STORE will be used. */

#else /* use OPT_CPU setting */

    // assuming no wraparound on indexed addressing modes
# if OPT_CPU == 0
    /* for indexed indirect addressing modes */
#   define LOAD_ZPG_ADDR16(a)   (LOAD_ZPG(a) | (LOAD_ZPG(((a)+1) & 0xff) << 8))
#   define LOADEXEC(a)              ( ((a) & 0x1000) \
                                      ? theRom[(a) & 0xfff] : theRam[(a) & 0x7f] )
/* on the atari, the stack page is a mirror of page 0 */
#   define LOAD_STACK(a)        LOAD_ZPG((a))
#   define STORE_STACK(a, b)    STORE_ZPG((a), (b))

# else /* OPT_CPU >= 1 */
#   define LOAD_ZPG(a)          (((a) & 0x80) ? theRam[(a) & 0x7f] : read_tia(a))
#if defined(HANDLE_TIA_IN_RASTER) && !defined(HANDLE_TIA_CALLTABLE)
#   define STORE_ZPG(a,b)       (((a) & 0x80) ? (theRam[(a) & 0x7f]=(b)) : do_tia_write((a), (b)))
#else
#   define STORE_ZPG(a,b)       (((a) & 0x80) ? (theRam[(a) & 0x7f]=(b)) : write_tia((a), (b)))
#endif
// only JMP indirect or jump through reset/brk vector. assume always rom or ram.
#   define LOAD_ADDR16(a)       LOADEXEC16(a)
#  if OPT_CPU >= 2
    // assuming code is only run in rom, never in ram! (may break some games)
#   define LOADEXEC(a)              (theRom[(a) & 0xfff])
#  else
#   define LOADEXEC(a)            ( ((a) & 0x1000) ? theRom[(a) & 0xfff] : theRam[(a) & 0x7f] )
#  endif
#   define LOADEXEC16(a)            (LOADEXEC (a) | (LOADEXEC ((a)+1) << 8))
# endif /* OPT_CPU */
#  if OPT_CPU >= 2
    // assume 16 bit read only happens in RAM. Breaks smurf rescue.
#   define LOAD_ZPG_ADDR16(a)   (theRam[(a) & 0x7f] + (theRam[((a)+1) & 0x7f] << 8))
    // assuming push/pull operates only on ram. Some games are incompatible, e.g. Atlantis
#   define LOAD_STACK(a)        (theRam[(a) & 0x7f])
#   define STORE_STACK(a, b)    (theRam[(a) & 0x7f] = (b))
#  else
#   define LOAD_STACK(a)        LOAD_ZPG((a))
#   define STORE_STACK(a, b)    STORE_ZPG((a), (b))
#  endif
#endif /* OPT_CPU / OPT_CPU_PARANOID */


#endif /* MEMORY_H */

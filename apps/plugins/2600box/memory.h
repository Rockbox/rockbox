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

#endif /* MEMORY_H */

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
 * Bank switching functions are based on
 * x2600, the Atari Emulator, copyright 1996 by Alex Hornby
 */

/* =========================================================================
 * MEMORY.C
 * This part manages the cartrige ROM and handles CPU RAM and ROM access,
 * as well as bank switching stuff.
 * ========================================================================= */

#include "rbconfig.h"
#include "rb_test.h"

#include "address.h"
#include "options.h"
#include "cpu.h"

#include "memory.h"
#include "riot.h"
#include "tia_video.h"


/* ------------------------------------------------------------------------- */


/* The Rom define might need altering for paged carts */
int rom_size;

/* Used as a simple file buffer to store the data */
/* Enough for 16k carts */
BYTE theCart[CART_MAX_SIZE];

/* Pointer to start of ROM data */
BYTE *theRom;

/* Scratch area for those banking types that require memcpy() */
BYTE cartScratch[CART_SCRATCH_SIZE];

/* Area for those carts containing RAM */
BYTE cartRam[CART_RAM_SIZE];

BYTE theRam[128];

unsigned (*mapped_read[0x10000>>MEM_CHUNK_SIZE])(unsigned a);
void (*mapped_write[0x10000>>MEM_CHUNK_SIZE])(unsigned a, unsigned b);


/* =========================================================================
 * Functions to handle access to the different kinds of memory
 */


unsigned read_ram(unsigned a)
{
    a &= 0x7f;
    return theRam[a];
}

void write_ram(unsigned a, unsigned b)
{
    a &= 0x7f;
    theRam[a] = b;
}

unsigned read_rom(unsigned a)
{
    a &= 0xfff;
    return theRom[a];
}

void write_rom(unsigned a, unsigned b)
{
    /* dummy function (trying to write read-only-area) */
    (void) a;
    (void) b;
}

/* -------------------------------------------------------------------------
 * now for the bank switching and special ones ...
 */

/* Atari 8k F8 */
unsigned read_rom_bsw_f8(unsigned a)
{
    a &= 0xfff;
    if (a == 0xff8)
        theRom = &theCart[0];
    else if (a == 0xff9)
        theRom = &theCart[4096];
    return theRom[a];
}

/* Atari 8k F8 */
void write_rom_bsw_f8(unsigned a, unsigned b)
{
    (void) b;
    a &= 0xfff;
    if (a == 0xff8)
        theRom = &theCart[0];
    else if (a == 0xff9)
        theRom = &theCart[4096];
}

/* Atari 16k F6 */
unsigned read_rom_bsw_f6(unsigned a)
{
    a &= 0xfff;
    if (a == 0xff6)
        theRom = &theCart[0];
    else if (a == 0xff7)
        theRom = &theCart[4096];
    else if (a == 0xff8)
        theRom = &theCart[8192];
    else if (a == 0xff9)
        theRom = &theCart[12288];
    return theRom[a];
}

/* Atari 16k F6 */
void write_rom_bsw_f6(unsigned a, unsigned b)
{
    (void) b;
    a &= 0xfff;
    if (a == 0xff6)
        theRom = &theCart[0];
    else if (a == 0xff7)
        theRom = &theCart[4096];
    else if (a == 0xff8)
        theRom = &theCart[8192];
    else if (a == 0xff9)
        theRom = &theCart[12288];
}

/* Parker Brothers 8k E0 */
unsigned read_rom_bsw_e0(unsigned a)
{
    a &= 0xfff;
    if (a > 0xfdf && a < 0xfe8) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
    else if (a > 0xfe7 && a < 0xff0) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0x400],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
    else if (a > 0xfef && a < 0xff8) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0x800],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
    return theRom[a];
}

/* Parker Brothers 8k E0 */
void write_rom_bsw_e0(unsigned a, unsigned b)
{
    (void) b;
    a &= 0xfff;
    if (a > 0xfdf && a < 0xfe8) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
    else if (a > 0xfe7 && a < 0xff0) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0x400],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
    else if (a > 0xfef && a < 0xff8) {
        unsigned a1=(a&0x07)<<10;
        rb->memcpy(&cartScratch[0x800],&theCart[a1],0x400);
        TST_PROFILE(PRF_MEM_BSWCOPY, "Bsw: Copy cartScratch");
    }
}


/* CBS Ram Plus FA */
unsigned read_rom_bsw_fa(unsigned a)
{
    a &= 0xfff;
    if (a == 0xff8)
        theRom = &theCart[0];
    else if (a == 0xff9)
        theRom = &theCart[4096];
    else if (a == 0xffa)
        theRom = &theCart[8192];
    return theRom[a];
}

/* CBS Ram Plus FA */
void write_rom_bsw_fa(unsigned a, unsigned b)
{
    (void) b;
    a &= 0xfff;
    if (a == 0xff8)
        theRom = &theCart[0];
    else if (a == 0xff9)
        theRom = &theCart[4096];
    else if (a == 0xffa)
        theRom = &theCart[8192];
}


/* 128 byte extra cartrige ram (Atari F6 Super Chip) */
unsigned read_rom_cart_ram128(unsigned a)
{
    a &= 0x7f;
    return cartRam[a];
}

/* 128 byte extra cartrige ram (Atari F6 Super Chip) */
void write_rom_cart_ram128(unsigned a, unsigned b)
{
    a &= 0x7f;
    cartRam[a] = b;
}

/* 256 byte extra cartrige ram (CBS Ram Plus FA) */
unsigned read_rom_cart_ram256(unsigned a)
{
    a &= 0xff;
    return cartRam[a];
}

/* 256 byte extra cartrige ram (CBS Ram Plus FA) */
void write_rom_cart_ram256(unsigned a, unsigned b)
{
    a &= 0xff;
    cartRam[a] = b;
}

// --------------------------------------------------------------------

/*
 * we divide the address space in chunks of 128 bytes and use a call table
 * for memory access.
 */
void init_memory_map(void) COLD_ATTR;
void init_memory_map(void)
{
    unsigned i = 0;
    unsigned a = 0;

    for (i = a = 0; a < 0x10000; ++i, a += 1<<MEM_CHUNK_SIZE) {
        if ((a & 0x1080) == 0x0000) {                   /* tia */
            mapped_read[i] = &read_tia;
            mapped_write[i] = &write_tia;
        }
        else if ((a & 0x1280) == 0x0080) {              /* ram */
            mapped_read[i] = &read_ram;
            mapped_write[i] = &write_ram;
        }
        else if ((a & 0x1280) == 0x0280) {              /* riot i/o & timer */
            mapped_read[i] = &read_riot;
            mapped_write[i] = &write_riot;
        }
        else if (a & 0x1000) {                          /* rom */
            if ((a & 0xfff) >= 0xf80) {
                /* allmost all cartriges' bank switching addresses are located here */
                switch (base_opts.bank) {
                case BSW_F8:
                    mapped_read[i] = &read_rom_bsw_f8;
                    mapped_write[i] = &write_rom_bsw_f8;
                    break;
                case BSW_F6:
                case BSW_F6SC_SUPERCHIP:
                    mapped_read[i] = &read_rom_bsw_f6;
                    mapped_write[i] = &write_rom_bsw_f6;
                    break;
                case BSW_E0_PARKER:
                    mapped_read[i] = &read_rom_bsw_e0;
                    mapped_write[i] = &write_rom_bsw_e0;
                    break;
                case BSW_FA_CBA_RAMPLUS:
                    mapped_read[i] = &read_rom_bsw_fa;
                    mapped_write[i] = &write_rom_bsw_fa;
                    break;
                default:
                    /* normal read only rom */
                    mapped_read[i] = &read_rom;
                    mapped_write[i] = &write_rom;
                    break;
                }
            }
            else if (base_opts.bank == BSW_FA_CBA_RAMPLUS
                     && (a & 0xfff) < 0x200) {
                if ((a & 0xfff) < 0x100) { /* cartrige ram write address */
                    mapped_read[i] = &read_rom;
                    mapped_write[i] = &write_rom_cart_ram256;
                }
                else {  /* cartrige ram read address */
                    mapped_read[i] = &read_rom_cart_ram256;
                    mapped_write[i] = &write_rom;
                }
            }
            else if (base_opts.bank == BSW_F6SC_SUPERCHIP
                     && (a & 0xfff) < 0x100) {
                if ((a & 0xfff) < 0x80) { /* cartrige ram write address */
                    mapped_read[i] = &read_rom;
                    mapped_write[i] = &write_rom_cart_ram128;
                }
                else {  /* cartrige ram read address */
                    mapped_read[i] = &read_rom_cart_ram128;
                    mapped_write[i] = &write_rom;
                }
            }
            else {  /* normal read only rom */
                mapped_read[i] = &read_rom;
                mapped_write[i] = &write_rom;
            }
        } /* memory areas */
    }
}

// --------------------------------------------------------------------

void init_banking (void) COLD_ATTR;
void init_banking (void)
{
  /* Set to the first bank */
  if (rom_size == 2048)
    theRom = &theCart[rom_size - 2048];
  else
    theRom = &theCart[rom_size - 4096];

  switch(base_opts.bank)
    {
    case 3:
      /* Parker Brothers 8k E0 */
      rb->memcpy(&cartScratch[0xc00],&theCart[0x1c00],1024);
      rb->memcpy(&cartScratch[0],&theCart[0],3072);
      theRom=cartScratch;
      break;
    default:
      break;
    }
}


void init_memory(void) COLD_ATTR;
void init_memory(void)
{
    /* Wipe the RAM */
    for(unsigned i=0; i<ARRAYLEN(theRam); i++)
        theRam[i]=0;
    for(unsigned i=0; i<ARRAY_LEN(cartRam); i++)
        cartRam[i]=0;

    init_memory_map();
}


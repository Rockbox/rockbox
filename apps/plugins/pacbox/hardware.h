/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Pacbox - a Pacman Emulator for Rockbox
 *
 * Based on PIE - Pacman Instructional Emulator
 *
 * Copyright (c) 1997-2003,2004 Alessandro Scotti
 * http://www.ascotti.org/
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

#ifndef _HARDWARE_H
#define _HARDWARE_H

extern unsigned char   ram_[20*1024];          // ROM (16K) and RAM (4K)
extern unsigned char   charset_rom_[4*1024];   // Character set ROM (4K)
extern unsigned char   spriteset_rom_[4*1024]; // Sprite set ROM (4K)
extern unsigned char dirty_[1024];
extern unsigned char video_mem_[1024];       // Video memory (1K)
extern unsigned char color_mem_[1024];       // Color memory (1K)
extern unsigned char charmap_[256*8*8];    /* Character data for 256 8x8 characters */
extern unsigned char spritemap_[64*16*16]; /* Sprite data for 64 16x16 sprites */
extern unsigned char output_devices_;  /* Output flip-flops set by the game program */
extern unsigned char   interrupt_vector_;
extern unsigned        coin_counter_;
extern unsigned char port1_;
extern unsigned char port2_;
extern unsigned char dip_switches_;

/* Output flip-flops */
enum {
    FlipScreen          = 0x01,
    PlayerOneLight      = 0x02,
    PlayerTwoLight      = 0x04,
    InterruptEnabled    = 0x08,
    SoundEnabled        = 0x10,
    CoinLockout         = 0x20,
    CoinMeter           = 0x40,
    AuxBoardEnabled     = 0x80
};

struct PacmanSprite
{
    int mode; /** Display mode (normal or flipped) */
    int x; /** X coordinate */
    int y; /** Y coordinate */
    int n; /** Shape (from 0 to 63) */
    int color; /** Base color (0=not visible) */
};

/* Internal tables and structures for faster access to data */
extern struct PacmanSprite    sprites_[8];    /* Sprites */
extern short             vchar_to_i_[1024];

void writeByte(unsigned addr, unsigned char b);
unsigned char readPort( unsigned port );
void writePort( unsigned addr, unsigned char b );
void setOutputFlipFlop( unsigned char bit, unsigned char value );

/*
    For Z80 Environment: read a byte from high memory addresses (i.e. the
    memory-mapped registers)
*/
static inline unsigned char readByte( unsigned addr ) 
{
    addr &= 0xFFFF;

    if (addr < sizeof(ram_))
        return ram_[addr];

    // Address is not in RAM, check to see if it's a memory mapped register
    switch( addr & 0xFFC0 ) {
    // IN0
    case 0x5000: 
        // bit 0 : up
        // bit 1 : left
        // bit 2 : right
        // bit 3 : down
        // bit 4 : switch: advance to next level
        // bit 5 : coin 1
        // bit 6 : coin 2
        // bit 7 : credit (same as coin but coin counter is not incremented)
        return port1_;
    // IN1
    case 0x5040: 
        // bit 0 : up (2nd player)
        // bit 1 : left (2nd player)
        // bit 2 : right (2nd player)
        // bit 3 : down (2nd player)
        // bit 4 : switch: rack test -> 0x10=off, 0=on
        // bit 5 : start 1
        // bit 6 : start 2
        // bit 7 : cabinet -> 0x80=upright, 0x00=table
        return port2_;
    // DSW1
    case 0x5080:
        // bits 0,1 : coinage -> 0=free play, 1=1 coin/play, 2=1 coin/2 play, 3=2 coin/1 play
        // bits 2,3 : lives -> 0x0=1, 0x4=2, 0x8=3, 0xC=5
        // bits 4,5 : bonus life -> 0=10000, 0x10=15000, 0x20=20000, 0x30=none
        // bit  6   : jumper pad: difficulty -> 0x40=normal, 0=hard
        // bit  7   : jumper pad: ghost name -> 0x80=normal, 0=alternate
        return dip_switches_;
    // Watchdog reset
    case 0x50C0:
        break;
    }

    return 0xFF;
}

/* Reads a 16 bit word from memory at the specified address. */
static inline unsigned readWord( unsigned addr ) {
    addr &= 0xFFFF;

    if (addr < (sizeof(ram_)-1)) {
        return ram_[addr] | ((ram_[addr+1]) << 8);
    } else {
        return readByte(addr) | (((unsigned)readByte(addr+1)) << 8);
    }
}

/* Writes a 16 bit word to memory at the specified address. */
static inline void writeWord( unsigned addr, unsigned value ) {
    writeByte( addr,   value & 0xFF );
    writeByte( addr+1, (value >> 8) & 0xFF );
}

#endif

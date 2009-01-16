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

#include "plugin.h"
#include "hardware.h"

/* The main data for Pacman  */

unsigned char ram_[20*1024] IBSS_ATTR;          // ROM (16K) and RAM (4K)
unsigned char charset_rom_[4*1024] IBSS_ATTR;   // Character set ROM (4K)
unsigned char spriteset_rom_[4*1024] IBSS_ATTR; // Sprite set ROM (4K)
unsigned char dirty_[1024] IBSS_ATTR;
unsigned char video_mem_[1024] IBSS_ATTR;       // Video memory (1K)
unsigned char color_mem_[1024] IBSS_ATTR;       // Color memory (1K)
unsigned char charmap_[256*8*8] IBSS_ATTR;    /* Character data for 256 8x8 characters */
unsigned char spritemap_[64*16*16]; /* Sprite data for 64 16x16 sprites */
unsigned char output_devices_ IBSS_ATTR;  /* Output flip-flops set by the game program */
unsigned char interrupt_vector_ IBSS_ATTR;
unsigned      coin_counter_ IBSS_ATTR;
unsigned char port1_ IBSS_ATTR;
unsigned char port2_ IBSS_ATTR;
unsigned char dip_switches_ IBSS_ATTR;

/* Internal tables and structures for faster access to data */
struct PacmanSprite sprites_[8];    /* Sprites */
short vchar_to_i_[1024];


/*
    For Z80 Environment: write a byte to memory.
*/
void writeByte( unsigned addr, unsigned char b )
{
    addr &= 0x7FFF;

    if( addr < 0x4000 ) {
        // This is a ROM address, do not write into it!
    }
    else if( addr < 0x4400 ) {
        // Video memory
        if (ram_[addr] != b) {
            int x = vchar_to_i_[addr-0x4000];
            ram_[addr] = b;
            dirty_[x] = 1;
            video_mem_[x] = b;
        }
    }
    else if( addr < 0x4800 ) {
        // Color memory
        if (ram_[addr] != b) {
            int x = vchar_to_i_[addr-0x4400];
            ram_[addr] = b;
            dirty_[x] = 1;
            color_mem_[x] = b;
        }
    }
    else if( addr < 0x4FF0 ) {
        // Standard memory
        ram_[addr] = b;
    }
    else if( addr < 0x5000 ) {
        // Sprites
        ram_[addr] = b;

        unsigned idx = (addr - 0x4FF0) / 2;

        if( addr & 1 ) {
            sprites_[ idx ].color = b;
        }
        else {
            sprites_[ idx ].n = b >> 2;
            sprites_[ idx ].mode = b & 0x03;
        }
    }
    else {
    // Memory mapped ports
    switch( addr ) {
        case 0x5000:
            // Interrupt enable
            setOutputFlipFlop( InterruptEnabled, b & 0x01 );
            break;
        case 0x5001:
            // Sound enable
            setOutputFlipFlop( SoundEnabled, b & 0x01 );
            break;
        case 0x5002:
            // Aux board enable?
            break;
        case 0x5003:
            // Flip screen
            setOutputFlipFlop( FlipScreen, b & 0x01 );
            break;
        case 0x5004:
            // Player 1 start light
            setOutputFlipFlop( PlayerOneLight, b & 0x01 );
            break;
        case 0x5005:
            // Player 2 start light
            setOutputFlipFlop( PlayerTwoLight, b & 0x01 );
            break;
        case 0x5006:
            // Coin lockout: bit 0 is used to enable/disable the coin insert slots 
            // (0=disable).
            // The coin slot is enabled at startup and (temporarily) disabled when 
            // the maximum number of credits (99) is inserted.
            setOutputFlipFlop( CoinLockout, b & 0x01 );
            break;
        case 0x5007:
            // Coin meter (coin counter incremented on 0/1 edge)
            if( (output_devices_ & CoinMeter) == 0 && (b & 0x01) != 0 )
                coin_counter_++;
            setOutputFlipFlop( CoinMeter, b & 0x01 );
            break;
        case 0x50c0:
            // Watchdog reset
            break;
        default:
            if( addr >= 0x5040 && addr < 0x5060 ) {
                // Sound registers
                //SOUND sound_chip_.setRegister( addr-0x5040, b );
            }
            else if( addr >= 0x5060 && addr < 0x5070 ) {
                // Sprite coordinates, x/y pairs for 8 sprites
                unsigned idx = (addr-0x5060) / 2;

                if( addr & 1 ) {
                    sprites_[ idx ].y = 272 - b + 1;
                }
                else {
                    sprites_[ idx ].x = 240 - b - 1;

                    if( idx <= 2 ) {
                        // In Pacman the first few sprites must be further offset 
                        // to the left to get a correct display (is this a hack?)
                        sprites_[ idx ].x -= 1;
                    }
                }
            }
            break;
        }
    }
}

/*
    For Z80 Environment: read from a port.

    Note: all ports in Pacman are memory mapped so they are read with readByte().
*/
unsigned char readPort( unsigned port ) 
{
    (void)port;
    return 0;
}

/*
    For Z80 Environment: write to a port.
*/
void writePort( unsigned addr, unsigned char b )
{
    if( addr == 0 ) {
        // Sets the interrupt vector for the next CPU interrupt
        interrupt_vector_ = b;
    }
}

void setOutputFlipFlop( unsigned char bit, unsigned char value )
{
    if( value ) {
        output_devices_ |= bit;
    }
    else {
        output_devices_ &= ~bit;
    }
}

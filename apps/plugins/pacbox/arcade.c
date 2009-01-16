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

#include "arcade.h"
#include "hardware.h"
#include <string.h>
#include "plugin.h"

#ifndef HAVE_LCD_COLOR
/* Convert RGB888 to 2-bit greyscale - logic taken from bmp2rb.c */
static fb_data rgb_to_gray(unsigned int r, unsigned int g, unsigned int b)
{
    int brightness = ( 2*r + 4*g + b );
    if( r == 0 && g == 0 && b == 0 )
      return 3;

    brightness = (brightness/450);
    if( brightness > 2 ) return 0;
    else return 2-brightness;
}
#endif

unsigned char   color_data_[256] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0b, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0b, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0b, 0x05,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0b, 0x07,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x01, 0x09,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x00, 0x0e, 0x00, 0x01, 0x0c, 0x0f,
        0x00, 0x0e, 0x00, 0x0b, 0x00, 0x0c, 0x0b, 0x0e,
        0x00, 0x0c, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x0f, 0x00, 0x07, 0x0c, 0x02,
        0x00, 0x09, 0x06, 0x0f, 0x00, 0x0d, 0x0c, 0x0f,
        0x00, 0x05, 0x03, 0x09, 0x00, 0x0f, 0x0b, 0x00,
        0x00, 0x0e, 0x00, 0x0b, 0x00, 0x0e, 0x00, 0x0b,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0e, 0x01,
        0x00, 0x0f, 0x0b, 0x0e, 0x00, 0x0e, 0x00, 0x0f,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char palette_data_[0x20] = {
       0x00, 0x07, 0x66, 0xef, 0x00, 0xf8, 0xea, 0x6f,
       0x00, 0x3f, 0x00, 0xc9, 0x38, 0xaa, 0xaf, 0xf6,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

enum {
    Normal  = 0x00,
    FlipY   = 0x01,
    FlipX   = 0x02,
    FlipXY  = 0x03
};

/* Putting this in IRAM actually slows down the iPods, but is good for
   the Coldfire
*/
#ifdef CPU_COLDFIRE
fb_data  palette[256] IBSS_ATTR; /* Color palette */
#else
fb_data  palette[256];           /* Color palette */
#endif


void init_PacmanMachine(int dip)
{
    int i;

    /* Initialize the CPU and the RAM */
    z80_reset();
    rb->memset( &ram_[0x4000], 0xFF, 0x1000 );

    /* Initialize parameters */
    port1_ = 0xFF;
    port2_ = 0xFF;
    coin_counter_ = 0;

    /* Reset the machine */
    reset_PacmanMachine();

    /* Set the DIP switches to a default configuration */
    setDipSwitches( dip );

    /* Initialize the video character translation tables: video memory has a 
       very peculiar arrangement in Pacman so we precompute a few tables to 
       move around faster */

    for( i=0x000; i<0x400; i++ ) {
        int x, y;

        if( i < 0x040 ) {
          x = 29 - (i & 0x1F);
          y = 34 + (i >> 5);
        }
        else if( i >= 0x3C0 ) {
          x = 29 - (i & 0x1F);
          y = ((i-0x3C0) >> 5);
        }
        else {
          x = 27 - ((i-0x40) >> 5);
          y = 2 + ((i-0x40) & 0x1F);
        }
        if( (y >= 0) && (y < 36) && (x >= 0) && (x < 28) )
            vchar_to_i_[i] = y*28 + x;
        else
            vchar_to_i_[i] = 0x3FF;
    }
}

void reset_PacmanMachine(void)
{
    int i;

    z80_reset();
    output_devices_ = 0;
    interrupt_vector_ = 0;

    rb->memset( ram_+0x4000, 0, 0x1000 );
    rb->memset( color_mem_, 0, sizeof(color_mem_) );
    rb->memset( video_mem_, 0, sizeof(video_mem_) );
    rb->memset( dirty_, 0, sizeof(dirty_) );

    for( i=0; i<8; i++ ) {
        sprites_[i].color = 0;
        sprites_[i].x = ScreenWidth;
    }
}

/*
    Run the machine for one frame.
*/
int run(void)
{
    /* Run until the CPU has executed the number of cycles per frame
       (the function returns the number of "extra" cycles spent by the
       last instruction but that is not really important here) */

    unsigned extraCycles = z80_run( CpuCyclesPerFrame );

    /* Reset the CPU cycle counter to make sure it doesn't overflow,
       also take into account the extra cycles from the previous run */

    setCycles( extraCycles );

    /* If interrupts are enabled, force a CPU interrupt with the vector
       set by the program */

    if( output_devices_ & InterruptEnabled ) {
        z80_interrupt( interrupt_vector_ );
    }

    return 0;
}

/** Returns the status of the coin lockout door. */
unsigned char getCoinLockout(void) {
    return output_devices_ & CoinLockout ? 1 : 0;
}

static void decodeCharByte( unsigned char b, unsigned char * charbuf, int charx, int chary, int charwidth )
{
    int i;

    for( i=3; i>=0; i-- ) {
        charbuf[charx+(chary+i)*charwidth] = (b & 1) | ((b >> 3) & 2);
        b >>= 1;
    }
}

static void decodeCharLine( unsigned char * src, unsigned char * charbuf, int charx, int chary, int charwidth )
{
    int x;

    for( x=7; x>=0; x-- ) {
        decodeCharByte( *src++, charbuf, x+charx, chary, charwidth );
    }
}

static void decodeCharSet( unsigned char * mem, unsigned char * charset )
{
    int i;

    for( i=0; i<256; i++ ) {
        unsigned char * src = mem + 16*i;
        unsigned char * dst = charset + 64*i;

        decodeCharLine( src,   dst, 0, 4, 8 );
        decodeCharLine( src+8, dst, 0, 0, 8 );
    }
}

static void decodeSprites( unsigned char * mem, unsigned char * sprite_data )
{
    int i;

    for( i=0; i<64; i++ ) {
        unsigned char * src = mem + i*64;
        unsigned char * dst = sprite_data + 256*i;

        decodeCharLine( src   , dst, 8, 12, 16 );
        decodeCharLine( src+ 8, dst, 8,  0, 16 );
        decodeCharLine( src+16, dst, 8,  4, 16 );
        decodeCharLine( src+24, dst, 8,  8, 16 );
        decodeCharLine( src+32, dst, 0, 12, 16 );
        decodeCharLine( src+40, dst, 0,  0, 16 );
        decodeCharLine( src+48, dst, 0,  4, 16 );
        decodeCharLine( src+56, dst, 0,  8, 16 );
    }
}

/*
    Decode one byte from the encoded color palette.

    An encoded palette byte contains RGB information bit-packed as follows:
        
          bit: 7 6 5 4 3 2 1 0
        color: b b g g g r r r
*/
static unsigned decodePaletteByte( unsigned char value )
{
    unsigned    bit0, bit1, bit2;
    unsigned    red, green, blue;

    bit0 = (value >> 0) & 0x01;
    bit1 = (value >> 1) & 0x01;
    bit2 = (value >> 2) & 0x01;
    red = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

    bit0 = (value >> 3) & 0x01;
    bit1 = (value >> 4) & 0x01;
    bit2 = (value >> 5) & 0x01;
    green = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

    bit0 = 0;
    bit1 = (value >> 6) & 0x01;
    bit2 = (value >> 7) & 0x01;
    blue = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

    return (blue << 16 ) | (green << 8) | red;
}

void decodeROMs(void)
{
    unsigned decoded_palette[0x20];
    unsigned c;

    int i;

    decodeCharSet( charset_rom_, charmap_ );
    decodeSprites( spriteset_rom_, spritemap_ );

    for( i=0x00; i<0x20; i++ ) {
        decoded_palette[i] = decodePaletteByte( palette_data_[i] );
    }
    for( i=0; i<256; i++ ) {
        c = decoded_palette[ color_data_[i] & 0x0F ];
#ifdef HAVE_LCD_COLOR
        palette[i] = LCD_RGBPACK((unsigned char) (c),
                                 (unsigned char) (c >> 8),
                                 (unsigned char) (c >> 16));
#else
        palette[i] = rgb_to_gray((unsigned char) (c), 
                                 (unsigned char) (c >> 8), 
                                 (unsigned char) (c >> 16) );
#endif
    }
}

void getDeviceInfo( enum InputDevice device, unsigned char * mask, unsigned char ** port )
{
    static unsigned char MaskInfo[] = {
         0x01 , // Joy1_Up
         0x02 , // Joy1_Left
         0x04 , // Joy1_Right
         0x08 , // Joy1_Down
         0x10 , // Switch_RackAdvance
         0x20 , // CoinSlot_1
         0x40 , // CoinSlot_2
         0x80 , // Switch_AddCredit
         0x01 , // Joy2_Up
         0x02 , // Joy2_Left
         0x04 , // Joy2_Right
         0x08 , // Joy2_Down
         0x10 , // Switch_Test
         0x20 , // Key_OnePlayer
         0x40 , // Key_TwoPlayers
         0x80   // Switch_CocktailMode
    };

    *mask = MaskInfo[device];

    switch( device ) {
        case Joy1_Up:
        case Joy1_Left:
        case Joy1_Right:
        case Joy1_Down:
        case Switch_RackAdvance:
        case CoinSlot_1:
        case CoinSlot_2:
        case Switch_AddCredit:
            *port = &port1_;
            break;
        case Joy2_Up:
        case Joy2_Left:
        case Joy2_Right:
        case Joy2_Down:
        case Switch_Test:
        case Key_OnePlayer:
        case Key_TwoPlayers:
        case Switch_CocktailMode:
            *port = &port2_;
            break;
        default:
            *port = 0;
            break;
    }
}

enum InputDeviceMode getDeviceMode( enum InputDevice device )
{
    unsigned char mask;
    unsigned char * port;

    getDeviceInfo( device, &mask, &port );

    return (*port & mask) == 0 ? DeviceOn : DeviceOff;
}

/*
    Fire an input event, telling the emulator for example
    that the joystick has been released from the down position.
*/
void setDeviceMode( enum InputDevice device, enum InputDeviceMode mode )
{
    if( (getCoinLockout() == 0) && ((device == CoinSlot_1)||(device == CoinSlot_2)||(device == Switch_AddCredit)) ) {
        // Coin slots are locked, ignore command and exit
        return;
    }

    unsigned char mask;
    unsigned char * port;

    getDeviceInfo( device, &mask, &port );

    if( mode == DeviceOn )
        *port &= ~mask;
    else if( mode == DeviceOff )
        *port |= mask;
    else if( mode == DeviceToggle )
        *port ^= mask;
}

void setDipSwitches( unsigned value ) {
    dip_switches_ = (unsigned char) value;

    setDeviceMode( Switch_RackAdvance, value & DipRackAdvance_Auto ? DeviceOn : DeviceOff );
    setDeviceMode( Switch_Test, value & DipMode_Test ? DeviceOn : DeviceOff );
    setDeviceMode( Switch_CocktailMode, value & DipCabinet_Cocktail ? DeviceOn : DeviceOff );
}

unsigned getDipSwitches(void) {
    unsigned result = dip_switches_;

    if( getDeviceMode(Switch_RackAdvance) == DeviceOn ) result |= DipRackAdvance_Auto;
    if( getDeviceMode(Switch_Test) == DeviceOn ) result |= DipMode_Test;
    if( getDeviceMode(Switch_CocktailMode) == DeviceOn ) result |= DipCabinet_Cocktail;

    return result;
}

#if defined (CPU_COLDFIRE)
extern void drawChar( unsigned char * buffer, int index, int ox, int oy, int color );
#else
static inline void drawChar( unsigned char * buffer, int index, int ox, int oy, int color )
{
    int x,y;

    /* Make the index point to the character offset into the character table */
    unsigned char * chrmap = charmap_ + index*64;
    buffer += ox + oy*224; /* Make the buffer point to the character position*/
    color = (color & 0x3F)*4;

    if( color == 0 )
    {
        for( y=7; y>=0; y-- )
        {
            rb->memset( buffer, 0, 8 );
            buffer += ScreenWidth;
        };
        return;
    };

    if( output_devices_ & FlipScreen ) {
        // Flip character
        buffer += 7*ScreenWidth;
        for( y=7; y>=0; y-- ) {
            for( x=7; x>=0; x-- ) {
                *buffer++ = (*chrmap++) + color;
            }
            buffer -= ScreenWidth + 8; // Go to the next line
        }
    }
    else {
        for( y=7; y>=0; y-- ) {
            for( x=7; x>=0; x-- ) {
                *buffer++ = (*chrmap++) + color;
            }
            buffer += ScreenWidth - 8; // Go to the next line
        }
    }
}
#endif

inline void drawSprite( unsigned char * buffer, int index )
{
    struct PacmanSprite ps = sprites_[index];
    int x,y;
    char * s, * s2;

    // Exit now if sprite not visible at all
    if( (ps.color == 0) || (ps.x >= ScreenWidth) || (ps.y < 16) || (ps.y >= (ScreenHeight-32)) ) {
        return;    
    }
    
    // Clip the sprite coordinates to cut the parts that fall off the screen
    int start_x = (ps.x < 0) ? 0 : ps.x;
    int end_x = (ps.x < (ScreenWidth-16)) ? ps.x+15 : ScreenWidth-1;

    // Prepare variables for drawing
    int color = (ps.color & 0x3F)*4;
    unsigned char * spritemap_base = spritemap_ + ((ps.n & 0x3F)*256);
    
    buffer += ScreenWidth*ps.y;

    dirty_[(start_x >> 3) + (ps.y >> 3)*28] = 1;
    dirty_[(start_x >> 3) + 1 + (ps.y >> 3)*28] = 1;
    dirty_[(end_x >> 3) + (ps.y >> 3)*28] = 1;
    dirty_[(start_x >> 3) + ((ps.y >> 3)+1)*28] = 1;
    dirty_[(start_x >> 3) + 1 + ((ps.y >> 3)+1)*28] = 1;
    dirty_[(end_x >> 3) + ((ps.y >> 3)+1)*28] = 1;
    dirty_[(start_x >> 3) + ((ps.y+15) >> 3)*28] = 1;
    dirty_[(start_x >> 3) + 1 + ((ps.y+15) >> 3)*28] = 1;
    dirty_[(end_x >> 3) + ((ps.y+15) >> 3)*28] = 1;

    // Draw the 16x16 sprite
    if( ps.mode == 0 ) {                 // Normal
       s2 = spritemap_base + start_x-ps.x;
        // Draw the 16x16 sprite
        for( y=15; y>=0; y-- ) {
            s = s2;
            for( x=start_x; x<=end_x; x++, s++ ) {
                if( *s ) {
                    buffer[x] = color + *s;
                }
            }
            buffer += ScreenWidth;
            s2 += 16;
        }
    } else if( ps.mode == 1 ) {            // Flip Y
        s2 = spritemap_base + start_x-ps.x + 240;
        for( y=15; y>=0; y-- ) {
            s = s2;
            for( x=start_x; x<=end_x; x++, s++ ) {
                if( *s ) {
                    buffer[x] = color + *s;
                }
            }
            buffer += ScreenWidth;
            s2 -= 16;
        }
    } else if( ps.mode == 2 ) {            // Flip X
        s2 = spritemap_base + 15 + ps.x-start_x;
        for( y=15; y>=-0; y-- ) {
            s = s2;
            for( x=start_x; x<=end_x; x++, s-- ) {
                if( *s ) {
                    buffer[x] = color + *s;
                }
            }
            buffer += ScreenWidth;
            s2 += 16;
        }
    } else {                           // Flip X and Y
        s2 = spritemap_base + 255 + ps.x-start_x;
        for( y=15; y>=0; y-- ) {
            s = s2;
            for( x=start_x; x<=end_x; x++, s-- ) {
                if( *s ) {
                    buffer[x] = color + *s;
                }
            }
            buffer += ScreenWidth;
            s2 -= 16;
        }
    }
}

/*
    Draw the video into the specified buffer.
*/
bool renderBackground( unsigned char * buffer )
{
    unsigned char * video = video_mem_;
    unsigned char * color = color_mem_;
    unsigned char * dirty = dirty_;
    int x,y;
    bool changed=false;

    // Draw the background first...
    if( output_devices_ & FlipScreen ) {
        for( y=ScreenHeight-CharHeight; y>=0; y-=CharHeight ) {
            for( x=ScreenWidth-CharWidth; x>=0; x-=CharWidth ) {
                if (*dirty) {
                    drawChar( buffer, *video++, x, y, *color++ );
                    *(dirty++)=0;
                    changed=true;
                } else {
                    dirty++;
                    video++;
                   color++;
                }
            }
        }
    }
    else {
        for( y=0; y<ScreenHeight; y+=CharHeight ) {
            for( x=0; x<ScreenWidth; x+=CharWidth ) {
                if (*dirty) {
                    drawChar( buffer, *video++, x, y, *color++ );
                    *(dirty++)=0;
                    changed=true;
                } else {
                    dirty++;
                    video++;
                    color++;
                }
            }
        }
    }

    return changed;
}

void renderSprites( unsigned char * buffer )
{
    int i;

    // ...then add the sprites
    for( i=7; i>=0; i-- ) {
        drawSprite( buffer, i );
    }
}

/* Enables/disables the speed hack. */
int setSpeedHack( int enabled )
{
    int result = 0;

    if( enabled ) {
        if( (ram_[0x180B] == 0xBE) && (ram_[0x1FFD] == 0x00) ) {
            // Patch the ROM to activate the speed hack
            ram_[0x180B] = 0x01; // Activate speed hack
            ram_[0x1FFD] = 0xBD; // Fix ROM checksum

            result = 1;
        }
    }
    else {
        if( (ram_[0x180B] == 0x01) && (ram_[0x1FFD] == 0xBD) ) {
            // Restore the patched ROM locations
            ram_[0x180B] = 0xBE;
            ram_[0x1FFD] = 0x00;

            result = 1;
        }
    }

    return result;
}

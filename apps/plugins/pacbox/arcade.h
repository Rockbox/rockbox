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

#ifndef ARCADE_H_
#define ARCADE_H_

#include "plugin.h"
#include "z80.h"
#include "hardware.h"

extern fb_data  palette[256];  /* Color palette in native Rockbox format */

/**
    Pacman sprite properties.

    This information is only needed by applications that want to do their own
    sprite rendering, as the renderVideo() function already draws the sprites.

    @see PacmanMachine::renderVideo
*/

/** Machine hardware data */
enum {
    ScreenWidth         = 224,
    ScreenHeight        = 288,
    ScreenWidthChars    = 28,
    ScreenHeightChars   = 36,
    CharWidth           = 8,
    CharHeight          = 8,
    VideoFrequency      = 60,
    CpuClock            = 3072000,
    SoundClock          = 96000,    // CPU clock divided by 32
    CpuCyclesPerFrame   = CpuClock / VideoFrequency
};

/** Input devices and switches */
enum InputDevice {
    Joy1_Up = 0,
    Joy1_Left,
    Joy1_Right,
    Joy1_Down,
    Switch_RackAdvance,
    CoinSlot_1,
    CoinSlot_2,
    Switch_AddCredit,
    Joy2_Up,
    Joy2_Left,
    Joy2_Right,
    Joy2_Down,
    Switch_Test,
    Key_OnePlayer,
    Key_TwoPlayers,
    Switch_CocktailMode
};

/** Input device mode */
enum InputDeviceMode {
    DeviceOn,
    DevicePushed = DeviceOn,
    DeviceOff,
    DeviceReleased = DeviceOff,
    DeviceToggle
};

/** DIP switches */
enum {
    DipPlay_Free            =   0x00, // Coins per play
    DipPlay_OneCoinOneGame  =   0x01,
    DipPlay_OneCoinTwoGames =   0x02,
    DipPlay_TwoCoinsOneGame =   0x03,
    DipPlay_Mask            =   0x03,
    DipLives_1              =   0x00, // Lives per game
    DipLives_2              =   0x04,
    DipLives_3              =   0x08,
    DipLives_5              =   0x0C,
    DipLives_Mask           =   0x0C,
    DipBonus_10000          =   0x00, // Bonus life
    DipBonus_15000          =   0x10,
    DipBonus_20000          =   0x20,
    DipBonus_None           =   0x30,
    DipBonus_Mask           =   0x30,
    DipDifficulty_Normal    =   0x40, // Difficulty
    DipDifficulty_Hard      =   0x00,
    DipDifficulty_Mask      =   0x40,
    DipGhostNames_Normal    =   0x80, // Ghost names
    DipGhostNames_Alternate =   0x00,
    DipGhostNames_Mask      =   0x80,
    DipMode_Play            = 0x0000, // Play/test mode
    DipMode_Test            = 0x0100,
    DipMode_Mask            = 0x0100,
    DipCabinet_Upright      = 0x0000, // Cabinet upright/cocktail
    DipCabinet_Cocktail     = 0x0200,
    DipCabinet_Mask         = 0x0200,
    DipRackAdvance_Off      = 0x0000, // Automatic level advance
    DipRackAdvance_Auto     = 0x0400,
    DipRackAdvance_Mask     = 0x0400
};

void init_PacmanMachine(int dip);
int run(void);
void reset_PacmanMachine(void);
void decodeROMs(void);

/**
    Tells the emulator that the status of an input device has changed.
*/
void setDeviceMode( enum InputDevice device, enum InputDeviceMode mode );

/**
    Returns the value of the DIP switches.
*/
unsigned getDipSwitches(void);

/**
    Sets the value of the DIP switches that control several game settings
    (see the Dip... constants above). 
    
    Most of the DIP switches are read at program startup and take effect 
    only after a machine reset.
*/
void setDipSwitches( unsigned value );

/**
    Draws the current video into the specified buffer.

    The buffer must be at least 224*288 bytes long. Pixels are stored in
    left-to-right/top-to-bottom order starting from the upper left corner.
    There is one byte per pixel, containing an index into the color palette 
    returned by getPalette().

    It's up to the application to display the buffer to the user. The 
    code might look like this:
    <BLOCKQUOTE>
    <PRE>
    @@    unsigned char video_buffer[ PacmanMachine::ScreenWidth * PacmanMachine::ScreenHeight ];
    @@    unsigned char * vbuf = video_buffer;
    @@    const unsigned * palette = arcade.getPalette();
    @@
    @@    arcade.renderVideo( vbuf );
    @@
    @@    for( int y=0; y<PacmanMachine::ScreenHeight; y++ ) {
    @@        for( int x=0; x<PacmanMachine::ScreenWidth; x++ ) {
    @@            unsigned color = palette[ *vbuf++ ];
    @@            unsigned char red = color & 0xFF;
    @@            unsigned char green = (color >> 8) & 0xFF;
    @@            unsigned char blue = (color >> 16) & 0xFF;
    @@
    @@            setPixel( x, y, red, green, blue );
    @@        }
    @@    }
    </PRE>
    </BLOCKQUOTE>

*/
bool renderBackground( unsigned char * buffer );
void renderSprites( unsigned char * buffer );

/**
    Enables/disables a common speed hack that allows Pacman to
    move four times faster than the ghosts.

    @param enabled true to enabled the hack, false to disable

    @return 0 if successful, otherwise the patch could not be applied
            (probably because the loaded ROM set does not support it)
*/
int setSpeedHack( int enabled );

/* Implementation of the Z80 Environment interface */
unsigned char readByteHigh( unsigned addr );

void writeByte( unsigned, unsigned char );

unsigned char readPort( unsigned port );

void writePort( unsigned, unsigned char );

#endif // ARCADE_H_

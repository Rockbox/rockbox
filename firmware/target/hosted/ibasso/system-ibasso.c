/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <stdint.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "config.h"
#include "debug.h"
#include "panic.h"

#include "button-ibasso.h"
#include "debug-ibasso.h"
#include "pcm-ibasso.h"


/* Fake stack. */
uintptr_t* stackbegin;
uintptr_t* stackend;


void system_init( void )
{
    TRACE;

    /* Fake stack. */
    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    /* Prevent device from deep sleeping, which will interrupt playback. */
    FILE* f = fopen( "/sys/power/wake_lock", "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /sys/power/wake_lock for writing.", __func__ );
        panicf( "ERROR %s: Can not open /sys/power/wake_lock for writing.", __func__ );
        return;
    }

    if( fputs( "rockbox", f ) == EOF )
    {
        DEBUGF( "ERROR %s: Write failed for /sys/power/wake_lock.", __func__ );
        fclose( f );
        panicf( "ERROR %s: Write failed for /sys/power/wake_lock.", __func__ );
        return;
    }

    fclose( f );

    /* Prevent device to mute, which will cause tinyalsa pcm_writes to fail. */
    f = fopen( "/sys/class/codec/wm8740_mute", "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /sys/class/codec/wm8740_mute for writing.", __func__ );
        panicf( "ERROR %s: Can not open /sys/class/codec/wm8740_mute for writing.", __func__ );
        return;
    }

    if( fputc( 0, f ) == EOF )
    {
        DEBUGF( "ERROR %s: Write failed for /sys/class/codec/wm8740_mute.", __func__ );
        fclose( f );
        panicf( "ERROR %s: Write failed for /sys/class/codec/wm8740_mute.", __func__ );
        return;
    }

    fclose( f );

    /*
        Here would be the correct place for 'system( "/system/bin/muteopen" );' (headphone-out
        relay) but in pcm-ibasso.c, pcm_play_dma_init() the output capacitors are charged already a
        bit and the click of the headphone-connection-relay is softer.
    */
}


void system_reboot( void )
{
    TRACE;

    button_close_device();

    reboot( RB_AUTOBOOT );
}


void system_exception_wait( void )
{
    TRACE;

    while( 1 ) {};
}

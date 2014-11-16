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


#include <stdio.h>

#include "config.h"
#include "debug.h"
#include "panic.h"

#include "debug-ibasso.h"
#include "pcm-ibasso.h"


void audiohw_set_volume( int vol_l, int vol_r )
{
    TRACE;

    int volume = vol_l > vol_r ? vol_l : vol_r;

    /*
        See codec-dx50.h.
        -128db        -> -1 (adjusted to 0, mute)
        -127dB to 0dB ->  1 to 255 in steps of 2
        volume is in centibels (tenth-decibels).
    */
    int volume_adjusted = ( volume / 10 ) * 2 + 255;

    DEBUGF( "DEBUG %s: vol_l: %d, vol_r: %d, volume: %d, volume_adjusted: %d.",
            __func__,
            vol_l,
            vol_r,
            volume,
            volume_adjusted );

    if( volume_adjusted > 255 )
    {
        DEBUGF( "DEBUG %s: Adjusting volume from %d to 255.", __func__, volume );
        volume_adjusted = 255;
    }
    if( volume_adjusted < 0 )
    {
        DEBUGF( "DEBUG %s: Adjusting volume from %d to 0.", __func__, volume );
        volume_adjusted = 0;
    }

    FILE *f = fopen( "/dev/codec_volume", "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open /dev/codec_volume for writing.", __func__ );
        panicf( "ERROR %s: Can not open /dev/codec_volume for writing.", __func__ );
        return;
    }

    if( fprintf( f, "%d", volume_adjusted ) < 1 )
    {
        fclose( f );
        DEBUGF( "ERROR %s: Write failed for /dev/codec_volume.", __func__ );
        panicf( "ERROR %s: Write failed for /dev/codec_volume.", __func__ );
        return;
    }

    fclose( f );
}

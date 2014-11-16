/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by headwhacker: iBasso DX90 port
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


#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "debug.h"
#include "lcd.h"
#include "panic.h"

#include "debug-ibasso.h"


/*
    0: backlight on
    1: backlight off
*/
static const char BACKLIGHT_POWER_INTERFACE[] = "/sys/devices/platform/rk29_backlight/backlight/rk28_bl/bl_power";


/*
    Prevent excessive _backlight_on usage.
    Required for proper seeking.
*/
bool _backlight_enabled = false;


bool _backlight_init( void )
{
    TRACE;

    FILE* f = fopen( BACKLIGHT_POWER_INTERFACE, "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        panicf( "ERROR %s: Can not open %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        return false;
    }

    if( fprintf( f, "%d", 0 ) < 1 )
    {
        fclose( f );
        DEBUGF( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        panicf( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        return false;
    }

    fclose( f );

    _backlight_enabled = true;

    return true;
}


void _backlight_on( void )
{
    if( ! _backlight_enabled )
    {
        _backlight_init();
        lcd_enable( true );
    }
}


void _backlight_off( void )
{
    TRACE;

    FILE* f = fopen( BACKLIGHT_POWER_INTERFACE, "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        panicf( "ERROR %s: Can not open %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        return;
    }

    if( fprintf( f, "%d", 1 ) < 1 )
    {
        fclose( f );
        DEBUGF( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        panicf( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_POWER_INTERFACE );
        return;
    }

    fclose( f );

    lcd_enable( false );

    _backlight_enabled = false;
}


/*
    Prevent excessive _backlight_set_brightness usage.
    Required for proper seeking.
*/
int _current_brightness = -1;


void _backlight_set_brightness( int brightness )
{
    if( brightness > MAX_BRIGHTNESS_SETTING )
    {
        DEBUGF( "DEBUG %s: Adjusting brightness from %d to MAX.", __func__, brightness );
        brightness = MAX_BRIGHTNESS_SETTING;
    }
    if( brightness < MIN_BRIGHTNESS_SETTING )
    {
        DEBUGF( "DEBUG %s: Adjusting brightness from %d to MIN.", __func__, brightness );
        brightness = MIN_BRIGHTNESS_SETTING;
    }

    if( _current_brightness == brightness )
    {
        return;
    }

    TRACE;

    _current_brightness = brightness;

    /*
        /sys/devices/platform/rk29_backlight/backlight/rk28_bl/max_brightness
        0 ... 255
    */
    static const char BACKLIGHT_BRIGHTNESS_INTERFACE[] = "/sys/devices/platform/rk29_backlight/backlight/rk28_bl/brightness";

    FILE *f = fopen( BACKLIGHT_BRIGHTNESS_INTERFACE, "w" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open %s for writing.", __func__, BACKLIGHT_BRIGHTNESS_INTERFACE );
        panicf( "ERROR %s: Can not open %s for writing.", __func__, BACKLIGHT_BRIGHTNESS_INTERFACE );
        return;
    }

    if( fprintf( f, "%d", brightness ) < 1 )
    {
        fclose( f );
        DEBUGF( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_BRIGHTNESS_INTERFACE );
        panicf( "ERROR %s: Write failed for %s.", __func__, BACKLIGHT_BRIGHTNESS_INTERFACE );
        return;
    }

    fclose( f );
}

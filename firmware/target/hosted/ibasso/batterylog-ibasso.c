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
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "debug.h"
#include "rbpaths.h"
#include "tick.h"

#include "batterylog-ibasso.h"
#include "debug-ibasso.h"


static int batterylog_read_int( const char* file_name )
{
    FILE *f = fopen( file_name, "r" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open %s for reading.", __func__, file_name );
        return 0;
    }

    int val;
    if( fscanf( f, "%d", &val ) == EOF )
    {
        DEBUGF( "ERROR %s: Read failed for %s.", __func__, file_name );
        val = 0;
    }

    fclose( f );

    return val;
}


static char* batterylog_read_char( const char* file_name, char* buffer, int buffer_size )
{
    buffer[0] = '\0';

    FILE *f = fopen( file_name, "r" );
    if( f == NULL )
    {
        DEBUGF( "ERROR %s: Can not open %s for reading.", __func__, file_name );
        return buffer;
    }

    if( fgets( buffer, buffer_size, f ) == NULL )
    {
        DEBUGF( "ERROR %s: Read failed for %s.", __func__, file_name );
    }

    if( strlen( buffer ) > 0 )
    {
        buffer[strlen( buffer ) - 1] = '\0';
    }

    fclose( f );

    return buffer;
}


static const char LOG_FILE[] = HOME_DIR"/batterylog.csv";


static time_t _last_tick = 0;


static void batterylog_tick( void )
{
    time_t current_tick = time( NULL );

    if( ( _last_tick != 0 ) && ( ( current_tick - _last_tick ) < 60 ) )
    {
        return;
    }

    _last_tick = current_tick;

    FILE* log_file = fopen( LOG_FILE, "a" );
    if( log_file == NULL )
    {
        batterylog_stop();
        DEBUGF( "%s: Can not open %s for appendig.", __func__, LOG_FILE );
        return;
    }

    char buffer[30];
    int buffer_size = sizeof( buffer );

    if( fprintf( log_file, "%ld;", _last_tick ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: _last_tick.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/capacity" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: capacity.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/chgmilliclscur" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: chgmilliclscur.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/chgmillicur" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: chgmillicur.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/chgmilliearcur" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: chgmilliearcur.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/chgmillisuscur" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: chgmillisuscur.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/current_now" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: current_now.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/energy_full_design" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: energy_full_design.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%s;", batterylog_read_char( "/sys/class/power_supply/battery/health", buffer, buffer_size ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: health.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%s;", batterylog_read_char( "/sys/class/power_supply/battery/model_name", buffer, buffer_size ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: model_name.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/online" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: online.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/present" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: present.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%s;", batterylog_read_char( "/sys/class/power_supply/battery/status", buffer, buffer_size ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: status.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%s;", batterylog_read_char( "/sys/class/power_supply/battery/technology", buffer, buffer_size ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: technology.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/temp" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: temp.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%s;", batterylog_read_char( "/sys/class/power_supply/battery/type", buffer, buffer_size ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: type.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/voltage_max_design" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: voltage_max_design.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d;", batterylog_read_int( "/sys/class/power_supply/battery/voltage_min_design" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: voltage_min_design.", __func__, LOG_FILE );
        goto on_error;
    }

    if( fprintf( log_file, "%d\n", batterylog_read_int( "/sys/class/power_supply/battery/voltage_now" ) ) < 0 )
    {
        DEBUGF( "%s: Write failed for %s: voltage_now.", __func__, LOG_FILE );
        goto on_error;
    }

    fclose( log_file );

    return;

on_error:
    batterylog_stop();
    fclose( log_file );
}


void batterylog_start( void )
{
    TRACE;

    FILE* log_file = fopen( LOG_FILE, "a" );
    if( log_file == NULL )
    {
        DEBUGF( "%s: Can not open %s for appendig.", __func__, LOG_FILE );
        return;
    }

    if( fputs( "time;capacity;chgmilliclscur;chgmillicur;chgmilliearcur;chgmillisuscur;current_now;energy_full_design;health;model_name;online;present;status;technology;temp;type;voltage_max_design;voltage_min_design;voltage_now\n",
               log_file ) == EOF )
    {
        DEBUGF( "%s: Write failed for %s.", __func__, LOG_FILE );
        fclose( log_file );
        return;
    }

    fclose( log_file );

    tick_add_task( batterylog_tick );
}


void batterylog_stop( void )
{
    TRACE;

    tick_remove_task( batterylog_tick );
}

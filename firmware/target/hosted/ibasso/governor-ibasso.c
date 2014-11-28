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

#include "debug-ibasso.h"
#include "governor-ibasso.h"
#include "sysfs-ibasso.h"


void set_governor( enum ibasso_governors governor )
{
    TRACE;

    char _governor[15];
    switch( governor )
    {
        case GOVERNOR_CONSERVATIVE:
        {
            snprintf( _governor, sizeof( _governor ), "conservative" );
            break;
        }

        case GOVERNOR_ONDEMAND:
        {
            snprintf( _governor, sizeof( _governor ), "ondemand" );
            break;
        }

        case GOVERNOR_POWERSAVE:
        {
            snprintf( _governor, sizeof( _governor ), "powersave" );
            break;
        }

        case GOVERNOR_INTERACTIVE:
        {
            snprintf( _governor, sizeof( _governor ), "interactive" );
            break;
        }

        case GOVERNOR_PERFORMANCE:
        {
            snprintf( _governor, sizeof( _governor ), "performance" );
            break;
        }

        default:
        {
            DEBUGF( "ERROR %s: Unknown governor: %d.", __func__, governor );
            return;
        }
    }

    DEBUGF( "DEBUG %s: governor: %s.", __func__, _governor );

    if( ! sysfs_set_string( SYSFS_SCALING_GOVERNOR, _governor ) )
    {
        DEBUGF( "ERROR %s: Can not set freq scaling governor.", __func__);
    }
}

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
#include "cpufreq-linux.h"
#include "debug.h"

#include "debug-ibasso.h"
#include "governor-ibasso.h"
#include "sysfs-ibasso.h"


void set_governor( enum ibasso_governors governor )
{
    TRACE;

    switch( governor )
    {
        case GOVERNOR_CONSERVATIVE:
        {
            cpufreq_set_governor( "conservative", CPUFREQ_ALL_CPUS );
            break;
        }

        case GOVERNOR_ONDEMAND:
        {
            cpufreq_set_governor( "ondemand", CPUFREQ_ALL_CPUS );
            break;
        }

        case GOVERNOR_POWERSAVE:
        {
            cpufreq_set_governor( "powersave", CPUFREQ_ALL_CPUS );
            break;
        }

        case GOVERNOR_INTERACTIVE:
        {
            cpufreq_set_governor( "interactive", CPUFREQ_ALL_CPUS );
            break;
        }

        case GOVERNOR_PERFORMANCE:
        {
            cpufreq_set_governor( "performance", CPUFREQ_ALL_CPUS );
            break;
        }

        default:
        {
            DEBUGF( "ERROR %s: Unknown governor: %d.", __func__, governor );
            break;
        }
    }
}

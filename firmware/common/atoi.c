/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Gary Czvitkovicz
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

#include <stdlib.h>
#include "ctype.h"

int atoi (const char *str)
{
    int value = 0;
    int sign = 1;
    
    while (isspace(*str))
    {
        str++;
    }
    
    if ('-' == *str)
    {
        sign = -1;
        str++;
    }
    else if ('+' == *str)
    {
        str++;
    }
    
    while ('0' == *str)
    {
        str++;
    }

    while (isdigit(*str))
    {
        value = (value * 10) + (*str - '0');
        str++;
    }
    
    return value * sign;
}

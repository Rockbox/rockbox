 /***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Port of xrick, a Rick Dangerous clone, to Rockbox.
 * See http://www.bigorno.net/xrick/
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza
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

#include "xrick/system/system.h"

/*
 * globals
 */
int sysarg_args_period = 0; /* time between each frame, in milliseconds. The default is 40. */
int sysarg_args_map = 0;
int sysarg_args_submap = 0;
bool sysarg_args_nosound = false;
const char *sysarg_args_data = NULL;

/*
 * Read and process arguments
 */
bool sysarg_init(UNUSED(int argc), char **argv)
{
    /* note: "*argv" is truly a "const *" */
    sysarg_args_data = *argv;

    return true;
}

/* eof */

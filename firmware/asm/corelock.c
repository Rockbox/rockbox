/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Daniel Ankers
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

#include "config.h"
#include "corelock.h"
/* Core locks using Peterson's mutual exclusion algorithm. */

#ifdef CPU_ARM
#include "arm/corelock.c"
#else

void corelock_lock(struct corelock *cl)
{
    const unsigned int core = CURRENT_CORE;
    const unsigned int othercore = 1 - core;

    cl->myl[core] = core;
    cl->turn = othercore;

    for (;;)
    {
        if (cl->myl[othercore] == 0 || cl->turn == core)
            break;
    }
}

int corelock_try_lock(struct corelock *cl)
{
    const unsigned int core = CURRENT_CORE;
    const unsigned int othercore = 1 - core;

    cl->myl[core] = core;
    cl->turn = othercore;

    if (cl->myl[othercore] == 0 || cl->turn == core)
    {
        return 1;
    }

    cl->myl[core] = 0;
    return 0;
}

void corelock_unlock(struct corelock *cl)
{
    cl->myl[CURRENT_CORE] = 0;
}

#endif

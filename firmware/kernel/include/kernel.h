/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg
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
#ifndef KERNEL_H
#define KERNEL_H

#include "config.h"

#include "system.h"
#include "queue.h"
#include "mutex.h"
#include "mrsw_lock.h"
#include "tick.h"

#ifdef INCLUDE_TIMEOUT_API
#include "timeout.h"
#endif

#ifdef HAVE_SEMAPHORE_OBJECTS
#include "semaphore.h"
#endif

#ifdef HAVE_CORELOCK_OBJECT
#include "corelock.h"
#endif

#define OBJ_WAIT_TIMEDOUT     (-1)
#define OBJ_WAIT_FAILED       0
#define OBJ_WAIT_SUCCEEDED    1

#define TIMEOUT_BLOCK   -1
#define TIMEOUT_NOBLOCK  0

#endif /* KERNEL_H */

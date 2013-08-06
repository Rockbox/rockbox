/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#define RB_FILESYSTEM_OS
#include "config.h"
#include "system.h"
#include "thread-sdl.h"
#include "mutex.h"
#include "file.h"

#ifdef HAVE_SDL_THREADS
#define YIELD_THRESHOLD 512
static bool initialized = false;
static struct mutex readwrite_mtx;

/* Allow other threads to run while performing I/O */
ssize_t os_sdl_readwrite(int osfd, void *buf, size_t nbyte, bool dowrite)
{
    if (!initialized)
    {
        mutex_init(&readwrite_mtx);
        initialized = true;
    }

    mutex_lock(&readwrite_mtx);

    void *mythread = nbyte > YIELD_THRESHOLD ? sim_thread_unlock() : NULL;

    ssize_t rc = dowrite ? write(osfd, buf, nbyte) : read(osfd, buf, nbyte);

    if (mythread)
        sim_thread_lock(mythread);

    mutex_unlock(&readwrite_mtx);
    return rc;
}

#endif /* HAVE_SDL_THREADS */

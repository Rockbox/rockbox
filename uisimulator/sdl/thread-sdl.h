/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dan Everton
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

#ifndef __THREADSDL_H__
#define __THREADSDL_H__

#include "SDL_thread.h"

extern SDL_Thread *gui_thread;   /* The "main" thread */
void thread_sdl_thread_lock(void *me);
void * thread_sdl_thread_unlock(void);
void thread_sdl_exception_wait(void);
bool thread_sdl_init(void *param); /* Init the sim threading API - thread created calls app_main */
void thread_sdl_shutdown(void); /* Shut down all kernel threads gracefully */
void thread_sdl_lock(void); /* Sync with SDL threads */
void thread_sdl_unlock(void); /* Sync with SDL threads */

#endif /* #ifndef __THREADSDL_H__ */


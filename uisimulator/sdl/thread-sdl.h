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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
bool thread_sdl_init(void *param); /* Init the sim threading API - thread created calls app_main */
void thread_sdl_shutdown(void); /* Shut down all kernel threads gracefully */
void thread_sdl_lock(void); /* Sync with SDL threads */
void thread_sdl_unlock(void); /* Sync with SDL threads */

#endif /* #ifndef __THREADSDL_H__ */


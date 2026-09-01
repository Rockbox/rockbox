/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 Mauricio G.
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

#ifndef __SYSTYPES_H__
#define __SYSTYPES_H__

#ifdef N3DS
#include <3ds/svc.h>
#include <3ds/types.h>
#include <3ds/thread.h>
#include <3ds/result.h>
#include <3ds/services/apt.h>
#include <3ds/services/fs.h>
#include <3ds/synchronization.h>

typedef Thread         sys_thread_type;
typedef RecursiveLock  sys_mutex_type;
typedef LightSemaphore sys_sem_type;
typedef Handle         sys_handle_type;
#endif

#endif /* __SYSTYPES_H__ */


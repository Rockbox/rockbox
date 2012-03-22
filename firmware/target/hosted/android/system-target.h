/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Martitz
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

#ifndef __SYSTEM_TARGET_H__
#define __SYSTEM_TARGET_H__

#include "kernel-unix.h"
#include "system-hosted.h"

 /* don't pull in jni.h for every user of this file, it should be only needed
  * within the target tree (if at all)
  * define this before #including system.h or system-target.h */
#ifdef _SYSTEM_WITH_JNI
#include <jni.h>
/*
 * discover the JNIEnv for this the calling thread in case it's not known */
extern JNIEnv* getJavaEnvironment(void);
#endif /* _SYSTEM_WITH_JNI */

#endif /* __SYSTEM_TARGET_H__ */

/* facility function to check/wait for rockbox being ready, to be used
 * by java calls into native that depend on Rockbox structures such as
 * initialized kernel. */
bool is_rockbox_ready(void);
void wait_rockbox_ready(void);
void set_rockbox_ready(void);

#define NEED_GENERIC_BYTESWAPS

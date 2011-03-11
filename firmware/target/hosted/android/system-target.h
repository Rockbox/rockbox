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

#include <jni.h>

#define disable_irq()
#define enable_irq()
#define disable_irq_save() 0
#define restore_irq(level) (void)level

void power_off(void);
void wait_for_interrupt(void);
void interrupt(void);

/* A JNI environment is specific to its thread, so use the correct way to
 * obtain it: share a pointer to the JavaVM structure and ask that the JNI
 * environment attached to the current thread. */
static inline JNIEnv* getJavaEnvironment(void)
{
    extern JavaVM *vm_ptr;
    JNIEnv *env = NULL;

    if (vm_ptr)
        (*vm_ptr)->GetEnv(vm_ptr, (void**) &env, JNI_VERSION_1_2);

    return env;
}

#endif /* __SYSTEM_TARGET_H__ */

#define NEED_GENERIC_BYTESWAPS


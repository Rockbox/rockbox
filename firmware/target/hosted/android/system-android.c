/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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


#include <jni.h>
#include "config.h"
#include "system.h"



/* global fields for use with various JNI calls */
JavaVM *vm_ptr;
jobject RockboxService_instance;
jclass  RockboxService_class;

uintptr_t *stackbegin;
uintptr_t *stackend;

extern int main(void);
extern void telephony_init_device(void);

void system_exception_wait(void) { }
void system_reboot(void) { }
void power_off(void) { }

void system_init(void)
{
    /* no better place yet */
    telephony_init_device();
}

/* this is the entry point of the android app initially called by jni */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxService_main(JNIEnv *env, jobject this)
{
    /* hack!!! we can't have a valid stack pointer otherwise.
     * but we don't really need it anyway, thread.c only needs it
     * for overflow detection which doesn't apply for the main thread
     * (it's managed by the OS) */

    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;

    (*env)->GetJavaVM(env, &vm_ptr);
    RockboxService_instance = this;
    RockboxService_class = (*env)->GetObjectClass(env, this);

    main();
}

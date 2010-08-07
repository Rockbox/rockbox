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
#include "button.h"
#include "audio.h"

extern JNIEnv *env_ptr;

extern jobject RockboxService_instance;

static jclass  RockboxTimer_class;
static jobject RockboxTimer_instance;
static jmethodID java_wait_for_interrupt;
static bool    initialized = false;
/*
 * This is called from the separate Timer java thread. I have not added any
 * interrupt simulation to it (like the sdl counterpart does),
 * I think this is probably not needed, unless code calls disable_interrupt()
 * in order to be protected from tick tasks, but I can't remember a place right
 * now.
 *
 * No synchronisation mechanism either. This could possibly be problematic,
 * but we'll see :)
 */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxTimer_timerTask(JNIEnv *env, jobject this)
{
    (void)env;
    (void)this;
    call_tick_tasks();
}

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxTimer_postCallIncoming(JNIEnv *env, jobject this)
{
    (void)env;
    (void)this;
    queue_broadcast(SYS_CALL_INCOMING, 0);
}

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxTimer_postCallHungUp(JNIEnv *env, jobject this)
{
    (void)env;
    (void)this;
    queue_broadcast(SYS_CALL_HUNG_UP, 0);
}

void tick_start(unsigned int interval_in_ms)
{
    JNIEnv e = *env_ptr;
    /* first, create a new Timer instance */
    RockboxTimer_class  = e->FindClass(env_ptr, "org/rockbox/RockboxTimer");
    jmethodID constructor = e->GetMethodID(env_ptr,
                                           RockboxTimer_class,
                                           "<init>",
                                           "(Lorg/rockbox/RockboxService;J)V");
    /* the constructor will do the tick_start */
    RockboxTimer_instance = e->NewObject(env_ptr,
                                         RockboxTimer_class,
                                         constructor,
                                         RockboxService_instance,
                                         (jlong)interval_in_ms);

    /* get our wfi func also */
    java_wait_for_interrupt = e->GetMethodID(env_ptr,
                                             RockboxTimer_class,
                                             "java_wait_for_interrupt",
                                             "()V");
    /* it's now safe to call java_wait_for_interrupt */
    initialized = true;
}

void wait_for_interrupt(void)
{
    if (LIKELY(initialized))
    {
        (*env_ptr)->CallVoidMethod(env_ptr,
                                   RockboxTimer_instance,
                                   java_wait_for_interrupt);
    }
}
 
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, void (*timer_callback)(void))
{
    (void)reg_prio;
    (void)unregister_callback;
    (void)cycles;
    (void)timer_callback;
    return false;
}

bool timer_set_period(long cycles)
{
    (void)cycles;
    return false;
}

void timer_unregister(void)
{
}

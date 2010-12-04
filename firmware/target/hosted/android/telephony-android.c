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


#include <jni.h>
#include "kernel.h"

extern JNIEnv *env_ptr;
extern jobject RockboxService_instance;


void telephony_init_device(void)
{
    JNIEnv e = *env_ptr;
    jclass class = e->FindClass(env_ptr, "org/rockbox/RockboxTelephony");
    jmethodID constructor = e->GetMethodID(env_ptr, class, "<init>", "(Landroid/content/Context;)V");

    e->NewObject(env_ptr, class, constructor, RockboxService_instance);
}


JNIEXPORT void JNICALL
Java_org_rockbox_RockboxTelephony_postCallIncoming(JNIEnv *env, jobject this)
{
    (void)env;
    (void)this;
    queue_broadcast(SYS_CALL_INCOMING, 0);
}

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxTelephony_postCallHungUp(JNIEnv *env, jobject this)
{
    (void)env;
    (void)this;
    queue_broadcast(SYS_CALL_HUNG_UP, 0);
}

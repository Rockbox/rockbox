/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Jonathan Gordon
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
#include "config.h"

#if (CONFIG_PLATFORM&PLATFORM_ANDROID)
#include <jni.h>
#include <stdbool.h>
#include <stdio.h>
#include <system.h>

extern JNIEnv   *env_ptr;
static jclass    RockboxKeyboardInput_class;
static jobject   RockboxKeyboardInput_instance;
static jmethodID kbd_inputfunc, kbd_result;

static void kdb_init(void)
{
    JNIEnv e = *env_ptr;
    jmethodID kbd_is_usable;
    if (RockboxKeyboardInput_class == NULL)
    {
        /* get the class and its constructor */
        RockboxKeyboardInput_class = e->FindClass(env_ptr,
                                            "org/rockbox/RockboxKeyboardInput");
        jmethodID constructor = e->GetMethodID(env_ptr,
                                               RockboxKeyboardInput_class,
                                               "<init>", "()V");
        RockboxKeyboardInput_instance = e->NewObject(env_ptr,
                                                     RockboxKeyboardInput_class,
                                                     constructor);
        kbd_inputfunc = e->GetMethodID(env_ptr, RockboxKeyboardInput_class,
                                       "kbd_input", "(Ljava/lang/String;)V");
        kbd_result =    e->GetMethodID(env_ptr, RockboxKeyboardInput_class,
                                       "get_result", "()Ljava/lang/String;");
    }
    /* need to get it every time incase the activity died/restarted */
    kbd_is_usable = e->GetMethodID(env_ptr, RockboxKeyboardInput_class,
                                   "is_usable", "()Z");
    while (!e->CallBooleanMethod(env_ptr, RockboxKeyboardInput_instance,
                                                            kbd_is_usable))
        sleep(HZ/10);
}

int kbd_input(char* text, int buflen)
{
    JNIEnv e = *env_ptr;
    jstring str = e->NewStringUTF(env_ptr, text);
    jobject ret;
    const char* retchars;
    kdb_init();

    e->CallVoidMethod(env_ptr, RockboxKeyboardInput_instance,kbd_inputfunc,str);

    do {
        sleep(HZ/10);
        ret = e->CallObjectMethod(env_ptr, RockboxKeyboardInput_instance,
                                                                    kbd_result);
    } while (!ret);
    
    e->ReleaseStringUTFChars(env_ptr, str, NULL);
    retchars = e->GetStringUTFChars(env_ptr, ret, 0);
    if (retchars[0])
        snprintf(text, buflen, retchars);
    e->ReleaseStringUTFChars(env_ptr, ret, retchars);
    
    return retchars[0]?0:1; /* return 0 on success */
}

int load_kbd(unsigned char* filename)
{
    (void)filename;
    return 1;
}

#endif

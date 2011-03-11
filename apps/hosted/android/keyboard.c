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
#include "string-extra.h"
#include "kernel.h"
#include "lang.h"
#include "system.h"

static jclass           RockboxKeyboardInput_class;
static jobject          RockboxKeyboardInput_instance;
static jmethodID        kbd_inputfunc;
static struct semaphore kbd_wakeup;
static bool             accepted;
static jstring          new_string;

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxKeyboardInput_put_1result(JNIEnv *env, jobject this,
                                                  jboolean _accepted,
                                                  jstring _new_string)
{
    (void)env;(void)this;

    accepted = (bool)_accepted;
    if (accepted)
    {
        new_string = _new_string;
        (*env)->NewGlobalRef(env, new_string); /* prevet GC'ing */
    }
    semaphore_release(&kbd_wakeup);
}

static void kdb_init(void)
{
    JNIEnv *env_ptr = getJavaEnvironment();
    JNIEnv e = *env_ptr;

    static jmethodID kbd_is_usable;
    if (RockboxKeyboardInput_class == NULL)
    {
        semaphore_init(&kbd_wakeup, 1, 0);
        /* get the class and its constructor */
        jclass kbInput_class = e->FindClass(env_ptr,
                                            "org/rockbox/RockboxKeyboardInput");
        RockboxKeyboardInput_class = e->NewGlobalRef(env_ptr, kbInput_class);
        jmethodID constructor = e->GetMethodID(env_ptr,
                                               RockboxKeyboardInput_class,
                                               "<init>", "()V");
        jobject kbInput_instance = e->NewObject(env_ptr,
                                                RockboxKeyboardInput_class,
                                                constructor);
        RockboxKeyboardInput_instance = e->NewGlobalRef(env_ptr,
                                                        kbInput_instance);
        kbd_inputfunc = e->GetMethodID(env_ptr, RockboxKeyboardInput_class,
                                       "kbd_input",
                                       "(Ljava/lang/String;"
                                       "Ljava/lang/String;"
                                       "Ljava/lang/String;)V");
        kbd_is_usable = e->GetMethodID(env_ptr, RockboxKeyboardInput_class,
                                       "is_usable", "()Z");
    }

    /* need to get it every time incase the activity died/restarted */
    while (!e->CallBooleanMethod(env_ptr, RockboxKeyboardInput_instance,
                                                            kbd_is_usable))
        sleep(HZ/10);
}

int kbd_input(char* text, int buflen)
{
    JNIEnv *env_ptr     = getJavaEnvironment();
    JNIEnv e            = *env_ptr;
    jstring str         = e->NewStringUTF(env_ptr, text);
    jstring ok_text     = e->NewStringUTF(env_ptr, str(LANG_KBD_OK));
    jstring cancel_text = e->NewStringUTF(env_ptr, str(LANG_KBD_CANCEL));
    const char *utf8_string;
    kdb_init();

    e->CallVoidMethod(env_ptr, RockboxKeyboardInput_instance,kbd_inputfunc,
                      str, ok_text, cancel_text);

    semaphore_wait(&kbd_wakeup, TIMEOUT_BLOCK);

    if (accepted)
    {
        utf8_string = e->GetStringUTFChars(env_ptr, new_string, 0);
        strlcpy(text, utf8_string, buflen);
        e->ReleaseStringUTFChars(env_ptr, new_string, utf8_string);
        e->DeleteGlobalRef(env_ptr, new_string);
    }
    e->DeleteLocalRef(env_ptr, str);
    e->DeleteLocalRef(env_ptr, ok_text);
    e->DeleteLocalRef(env_ptr, cancel_text);
    
    return !accepted; /* return 0 on success */
}

int load_kbd(unsigned char* filename)
{
    (void)filename;
    return 1;
}

#endif

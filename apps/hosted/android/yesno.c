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
#include "yesno.h"
#include "settings.h"
#include "lang.h"
#include "kernel.h"
#include "system.h"

static jobject   RockboxYesno_instance = NULL;
static jmethodID yesno_func;
static struct semaphore    yesno_done;
static bool      ret;

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxYesno_put_1result(JNIEnv *env, jobject this, jboolean result)
{
    (void)env;
    (void)this;
    ret = (bool)result;
    semaphore_release(&yesno_done);
}

static void yesno_init(JNIEnv *env_ptr)
{
    JNIEnv e = *env_ptr;
    static jmethodID yesno_is_usable;
    if (RockboxYesno_instance == NULL)
    {
        semaphore_init(&yesno_done, 1, 0);
        /* get the class and its constructor */
        jclass RockboxYesno_class = e->FindClass(env_ptr,
                                                 "org/rockbox/RockboxYesno");
        jmethodID constructor = e->GetMethodID(env_ptr,
                                               RockboxYesno_class,
                                               "<init>", "()V");
        RockboxYesno_instance = e->NewObject(env_ptr,
                                                     RockboxYesno_class,
                                                     constructor);
        yesno_func = e->GetMethodID(env_ptr, RockboxYesno_class,
                                       "yesno_display",
                                       "(Ljava/lang/String;"
                                       "Ljava/lang/String;"
                                       "Ljava/lang/String;)V");
        yesno_is_usable = e->GetMethodID(env_ptr, RockboxYesno_class,
                                       "is_usable", "()Z");
    }
    /* need to get it every time incase the activity died/restarted */
    while (!e->CallBooleanMethod(env_ptr, RockboxYesno_instance,
                                 yesno_is_usable))
        sleep(HZ/10);
}

static jstring build_message(JNIEnv *env_ptr, const struct text_message *message)
{
    char msg[1024] = "";
    JNIEnv e = *env_ptr;
    int i;
    for(i=0; i<message->nb_lines; i++)
    {
        char* text = P2STR((unsigned char *)message->message_lines[i]);
        if (i>0)
            strlcat(msg, "\n", 1024);
        strlcat(msg, text, 1024);
    }
    /* make sure the questions end in a ?, for some reason they dont! */
    if (!strrchr(msg, '?'))
        strlcat(msg, "?", 1024);
    return e->NewStringUTF(env_ptr, msg);
}

enum yesno_res gui_syncyesno_run(const struct text_message * main_message,
                                 const struct text_message * yes_message,
                                 const struct text_message * no_message)
{
    (void)yes_message;
    (void)no_message;
    JNIEnv *env_ptr = getJavaEnvironment();

    yesno_init(env_ptr);

    JNIEnv e = *env_ptr;
    jstring message = build_message(env_ptr, main_message);
    jstring yes = (*env_ptr)->NewStringUTF(env_ptr, str(LANG_SET_BOOL_YES));
    jstring no  = (*env_ptr)->NewStringUTF(env_ptr, str(LANG_SET_BOOL_NO));

    e->CallVoidMethod(env_ptr, RockboxYesno_instance, yesno_func,
                      message, yes, no);
    
    semaphore_wait(&yesno_done, TIMEOUT_BLOCK);

    e->DeleteLocalRef(env_ptr, message);
    e->DeleteLocalRef(env_ptr, yes);
    e->DeleteLocalRef(env_ptr, no);

    return ret ? YESNO_YES : YESNO_NO;
}

#endif

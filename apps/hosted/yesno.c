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
#include "yesno.h"
#include "settings.h"
#include "lang.h"

extern JNIEnv   *env_ptr;
static jclass    RockboxYesno_class = NULL;
static jobject   RockboxYesno_instance = NULL;
static jmethodID yesno_func, result_ready, yesno_result;

static void yesno_init(void)
{
    JNIEnv e = *env_ptr;
    jmethodID yesno_is_usable;
    if (RockboxYesno_class == NULL)
    {
        /* get the class and its constructor */
        RockboxYesno_class = e->FindClass(env_ptr,
                                            "org/rockbox/RockboxYesno");
        jmethodID constructor = e->GetMethodID(env_ptr,
                                               RockboxYesno_class,
                                               "<init>", "()V");
        RockboxYesno_instance = e->NewObject(env_ptr,
                                                     RockboxYesno_class,
                                                     constructor);
        yesno_func = e->GetMethodID(env_ptr, RockboxYesno_class,
                                       "yesno_display", "(Ljava/lang/String;)V");
        yesno_result =    e->GetMethodID(env_ptr, RockboxYesno_class,
                                       "get_result", "()Z");
        result_ready =    e->GetMethodID(env_ptr, RockboxYesno_class,
                                       "result_ready", "()Z");
    }
    /* need to get it every time incase the activity died/restarted */
    yesno_is_usable = e->GetMethodID(env_ptr, RockboxYesno_class,
                                   "is_usable", "()Z");
    while (!e->CallBooleanMethod(env_ptr, RockboxYesno_instance,
                                 yesno_is_usable))
        sleep(HZ/10);
}

jstring build_message(const struct text_message *message)
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
    yesno_init();
    
    JNIEnv e = *env_ptr;
    jstring message = build_message(main_message);
    jboolean ret;
    
    e->CallVoidMethod(env_ptr, RockboxYesno_instance, yesno_func, message);
    e->ReleaseStringUTFChars(env_ptr, message, NULL);
    

    do {
        sleep(HZ/10);
        ret = e->CallBooleanMethod(env_ptr, RockboxYesno_instance, result_ready);
    } while (!ret);
    
    ret = e->CallBooleanMethod(env_ptr, RockboxYesno_instance, yesno_result);
    return ret ? YESNO_YES : YESNO_NO;
}

#endif

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
#include <string.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "lcd.h"
#include "button.h"

extern JNIEnv *env_ptr;
extern jobject RockboxService_instance;

static jobject RockboxFramebuffer_instance;
static jmethodID java_lcd_update;
static jmethodID java_lcd_update_rect;
static jmethodID java_lcd_init;

static jclass    AndroidRect_class;
static jmethodID AndroidRect_constructor;

static int dpi;
static int scroll_threshold;
static bool display_on;

/* this might actually be called before lcd_init_device() or even main(), so
 * be sure to only access static storage initalized at library loading,
 * and not more */
static void connect_with_java(JNIEnv* env, jobject fb_instance)
{
    JNIEnv e = *env;
    static bool have_class;

    if (!have_class)
    {
        jclass fb_class = e->GetObjectClass(env, fb_instance);
        /* cache update functions */
        java_lcd_update      = e->GetMethodID(env, fb_class,
                                             "update",
                                             "(Ljava/nio/ByteBuffer;)V");
        java_lcd_update_rect = e->GetMethodID(env, fb_class,
                                             "update",
                                             "(Ljava/nio/ByteBuffer;"
                                              "Landroid/graphics/Rect;)V");
        jmethodID get_dpi    = e->GetMethodID(env, fb_class,
                                             "getDpi", "()I");
        jmethodID thresh     = e->GetMethodID(env, fb_class,
                                             "getScrollThreshold", "()I");
        /* these don't change with new instances so call them now */
        dpi                  = e->CallIntMethod(env, fb_instance, get_dpi);
        scroll_threshold     = e->CallIntMethod(env, fb_instance, thresh);

        java_lcd_init        = e->GetMethodID(env, fb_class,
                                             "initialize", "(II)V");
        AndroidRect_class    = e->FindClass(env, "android/graphics/Rect");
        AndroidRect_constructor = e->GetMethodID(env, AndroidRect_class,
                                             "<init>", "(IIII)V");
        have_class           = true;
    }

    /* we need to setup parts for the java object every time */
    (*env)->CallVoidMethod(env, fb_instance, java_lcd_init,
                          (jint)LCD_WIDTH, (jint)LCD_HEIGHT);
}

/*
 * Do nothing here and connect with the java object later (if it isn't already)
 */
void lcd_init_device(void)
{
}

void lcd_update(void)
{
    if (display_on)
    {
        JNIEnv e = *env_ptr;
        jobject buffer = e->NewDirectByteBuffer(env_ptr, lcd_framebuffer,
                                               (jlong) FRAMEBUFFER_SIZE);

        e->CallVoidMethod(env_ptr, RockboxFramebuffer_instance,
                                   java_lcd_update, buffer);
        e->DeleteLocalRef(env_ptr, buffer);
    }
}

void lcd_update_rect(int x, int y, int width, int height)
{
    if (display_on)
    {
        JNIEnv e = *env_ptr;
        jobject buffer = e->NewDirectByteBuffer(env_ptr, lcd_framebuffer,
                                                   (jlong) FRAMEBUFFER_SIZE);
        jobject rect = e->NewObject(env_ptr, AndroidRect_class, AndroidRect_constructor,
                                        x, y, x + width, y + height);
        e->CallVoidMethod(env_ptr, RockboxFramebuffer_instance,
                                   java_lcd_update_rect, buffer, rect);

        e->DeleteLocalRef(env_ptr, buffer);
        e->DeleteLocalRef(env_ptr, rect);
    }
}

/*
 * this is called when the surface is created, which called is everytime
 * the activity is brought in front and the RockboxFramebuffer gains focus
 *
 * Note this is considered interrupt context
 */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_surfaceCreated(JNIEnv *env, jobject this,
                                                     jobject surfaceholder)
{
    (void)surfaceholder;

    /* Update RockboxFramebuffer_instance */
    RockboxFramebuffer_instance = (*env)->NewGlobalRef(env, this);

    /* possibly a new instance - reconnect */
    connect_with_java(env, this);
    display_on = true;

    /* need to wait for button_queue to be valid to post to */
    wait_rockbox_ready();

    send_event(LCD_EVENT_ACTIVATION, NULL);
    /* Force an update, since the newly created surface is initially black
     * waiting for the next normal update results in a longish black screen */
    queue_post(&button_queue, BUTTON_REDRAW, 0);
}

/*
 * the surface is destroyed everytime the RockboxFramebuffer loses focus and
 * goes invisible
 */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_surfaceDestroyed(JNIEnv *e, jobject this,
                                                    jobject surfaceholder)
{
    (void)this; (void)surfaceholder;

    display_on = false;

    (*e)->DeleteGlobalRef(e, RockboxFramebuffer_instance);
    RockboxFramebuffer_instance = NULL;
}

bool lcd_active(void)
{
    return display_on;
}

int lcd_get_dpi(void)
{
    return dpi;
}

int touchscreen_get_scroll_threshold(void)
{
    return scroll_threshold;
}

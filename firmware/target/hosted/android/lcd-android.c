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

extern jobject RockboxService_instance;

static jobject RockboxFramebuffer_instance;
static jmethodID java_lcd_update;
static jmethodID java_lcd_update_rect;
static jmethodID java_lcd_init;
static jobject native_buffer;

static int dpi;
static int scroll_threshold;
static bool display_on;

/* this might actually be called before lcd_init_device() or even main(), so
 * be sure to only access static storage initalized at library loading,
 * and not more */
void connect_with_java(JNIEnv* env, jobject fb_instance)
{
    JNIEnv e = *env;

    /* Update RockboxFramebuffer_instance */
    if (!e->IsSameObject(env, RockboxFramebuffer_instance, fb_instance)) {
        if (RockboxFramebuffer_instance != NULL)
            e->DeleteGlobalRef(env, RockboxFramebuffer_instance);

        RockboxFramebuffer_instance = e->NewGlobalRef(env, fb_instance);
    }

    static bool have_class = false;
    if (!have_class)
    {
        jclass fb_class = e->GetObjectClass(env, fb_instance);
        /* cache update functions */
        java_lcd_update      = e->GetMethodID(env, fb_class,
                                             "java_lcd_update",
                                             "()V");
        java_lcd_update_rect = e->GetMethodID(env, fb_class,
                                             "java_lcd_update_rect",
                                             "(IIII)V");
        jmethodID get_dpi    = e->GetMethodID(env, fb_class,
                                             "getDpi", "()I");
        jmethodID thresh     = e->GetMethodID(env, fb_class,
                                             "getScrollThreshold", "()I");
        /* these don't change with new instances so call them now */
        dpi                  = e->CallIntMethod(env, fb_instance, get_dpi);
        scroll_threshold     = e->CallIntMethod(env, fb_instance, thresh);

        java_lcd_init        = e->GetMethodID(env, fb_class,
                                             "java_lcd_init",
                                             "(IILjava/nio/ByteBuffer;)V");

        jobject buffer       = e->NewDirectByteBuffer(env,
                                                  lcd_framebuffer,
                                                  (jlong)sizeof(lcd_framebuffer));

        native_buffer        = e->NewGlobalRef(env, buffer);

        have_class           = true;
    }
    /* we need to setup parts for the java object every time */
    (*env)->CallVoidMethod(env, fb_instance, java_lcd_init,
                          (jint)LCD_WIDTH, (jint)LCD_HEIGHT, native_buffer);
}

void lcd_deinit(void)
{
    JNIEnv *env_ptr = getJavaEnvironment();

    (*env_ptr)->DeleteGlobalRef(env_ptr, RockboxFramebuffer_instance);
    (*env_ptr)->DeleteGlobalRef(env_ptr, native_buffer);
}

/*
 * Do nothing here and connect with the java object later (if it isn't already)
 */
void lcd_init_device(void)
{
}

void lcd_update(void)
{
    JNIEnv *env_ptr = getJavaEnvironment();

    if (display_on)
        (*env_ptr)->CallVoidMethod(env_ptr, RockboxFramebuffer_instance,
                                   java_lcd_update);
}

void lcd_update_rect(int x, int y, int width, int height)
{
    JNIEnv *env_ptr = getJavaEnvironment();

    if (display_on)
        (*env_ptr)->CallVoidMethod(env_ptr, RockboxFramebuffer_instance,
                                   java_lcd_update_rect, x, y, width, height);
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
    /* possibly a new instance - reconnect */
    connect_with_java(env, this);
    display_on = true;

    send_event(LCD_EVENT_ACTIVATION, NULL);
    /* Force an update, since the newly created surface is initially black
     * waiting for the next normal update results in a longish black screen */
    queue_post(&button_queue, BUTTON_FORCE_REDRAW, 0);
}

/*
 * the surface is destroyed everytime the RockboxFramebuffer loses focus and
 * goes invisible
 */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_surfaceDestroyed(JNIEnv *e, jobject this,
                                                    jobject surfaceholder)
{
    (void)e; (void)this; (void)surfaceholder;

    display_on = false;
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

/* below is a plain copy from lcd-sdl.c */

/**
 * |R|   |1.000000 -0.000001  1.402000| |Y'|
 * |G| = |1.000000 -0.334136 -0.714136| |Pb|
 * |B|   |1.000000  1.772000  0.000000| |Pr|
 * Scaled, normalized, rounded and tweaked to yield RGB 565:
 * |R|   |74   0 101| |Y' -  16| >> 9
 * |G| = |74 -24 -51| |Cb - 128| >> 8
 * |B|   |74 128   0| |Cr - 128| >> 9
 */
#define YFAC    (74)
#define RVFAC   (101)
#define GUFAC   (-24)
#define GVFAC   (-51)
#define BUFAC   (128)

static inline int clamp(int val, int min, int max)
{
    if (val < min)
        val = min;
    else if (val > max)
        val = max;
    return val;
}

void lcd_yuv_set_options(unsigned options)
{
    (void)options;
}

/* Draw a partial YUV colour bitmap - similiar behavior to lcd_blit_yuv
   in the core */
void lcd_blit_yuv(unsigned char * const src[3],
                  int src_x, int src_y, int stride,
                  int x, int y, int width, int height)
{
    const unsigned char *ysrc, *usrc, *vsrc;
    int linecounter;
    fb_data *dst, *row_end;
    long z;

    /* width and height must be >= 2 and an even number */
    width &= ~1;
    linecounter = height >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
    dst     = &lcd_framebuffer[y][x];
    row_end = dst + width;
#else
    dst     = &lcd_framebuffer[x][LCD_WIDTH - y - 1];
    row_end = dst + LCD_WIDTH * width;
#endif

    z    = stride * src_y;
    ysrc = src[0] + z + src_x;
    usrc = src[1] + (z >> 2) + (src_x >> 1);
    vsrc = src[2] + (usrc - src[1]);

    /* stride => amount to jump from end of last row to start of next */
    stride -= width;

    /* upsampling, YUV->RGB conversion and reduction to RGB565 in one go */

    do
    {
        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    -= width >> 1;
        vsrc    -= width >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif

        do
        {
            int y, cb, cr, rv, guv, bu, r, g, b;

            y  = YFAC*(*ysrc++ - 16);
            cb = *usrc++ - 128;
            cr = *vsrc++ - 128;

            rv  =            RVFAC*cr;
            guv = GUFAC*cb + GVFAC*cr;
            bu  = BUFAC*cb;

            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif

            y = YFAC*(*ysrc++ - 16);
            r = y + rv;
            g = y + guv;
            b = y + bu;

            if ((unsigned)(r | g | b) > 64*256-1)
            {
                r = clamp(r, 0, 64*256-1);
                g = clamp(g, 0, 64*256-1);
                b = clamp(b, 0, 64*256-1);
            }

            *dst = LCD_RGBPACK_LCD(r >> 9, g >> 8, b >> 9);

#if LCD_WIDTH >= LCD_HEIGHT
            dst++;
#else
            dst += LCD_WIDTH;
#endif
        }
        while (dst < row_end);

        ysrc    += stride;
        usrc    += stride >> 1;
        vsrc    += stride >> 1;

#if LCD_WIDTH >= LCD_HEIGHT
        row_end += LCD_WIDTH;
        dst     += LCD_WIDTH - width;
#else
        row_end -= 1;
        dst     -= LCD_WIDTH*width + 1;
#endif
    }
    while (--linecounter > 0);

#if LCD_WIDTH >= LCD_HEIGHT
    lcd_update_rect(x, y, width, height);
#else
    lcd_update_rect(LCD_WIDTH - y - height, x, height, width);
#endif
}

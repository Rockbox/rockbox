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
#include "lcd.h"

extern JNIEnv *env_ptr;
extern jclass  RockboxService_class;
extern jobject RockboxService_instance;

static jobject Framebuffer_instance;
static jmethodID java_lcd_update;
static jmethodID java_lcd_update_rect;

static bool display_on;

void lcd_init_device(void)
{
    /* get the RockboxFramebuffer instance allocated by the activity */
    jfieldID id = (*env_ptr)->GetStaticFieldID(env_ptr,
                                         RockboxService_class,
                                         "fb",
                                         "Lorg/rockbox/RockboxFramebuffer;");

    Framebuffer_instance = (*env_ptr)->GetStaticObjectField(env_ptr,
                                                      RockboxService_class,
                                                      id);

    jclass Framebuffer_class = (*env_ptr)->GetObjectClass(env_ptr,
                                                          Framebuffer_instance);

    /* get the java init function and call it. it'll set up a bitmap
     * based on LCD_WIDTH, LCD_HEIGHT and the ByteBuffer which directly maps
     * our framebuffer */

    jmethodID java_init_lcd = (*env_ptr)->GetMethodID(env_ptr,
                                                   Framebuffer_class,
                                                   "java_lcd_init",
                                                   "(IILjava/nio/ByteBuffer;)V");
    java_lcd_update      = (*env_ptr)->GetMethodID(env_ptr,
                                                   Framebuffer_class,
                                                   "java_lcd_update",
                                                   "()V");
    java_lcd_update_rect = (*env_ptr)->GetMethodID(env_ptr,
                                                   Framebuffer_class,
                                                   "java_lcd_update_rect",
                                                   "(IIII)V");

    /* map the framebuffer to a ByteBuffer, this way lcd updates will
     * be directly feched from the framebuffer */
    jobject buf = (*env_ptr)->NewDirectByteBuffer(env_ptr,
                                                  lcd_framebuffer,
                                                  sizeof(lcd_framebuffer));
    
    (*env_ptr)->CallVoidMethod(env_ptr,
                               Framebuffer_instance,
                               java_init_lcd,
                               LCD_WIDTH, LCD_HEIGHT, buf);
    display_on = true;
}

void lcd_update(void)
{
    /* tell the system we're ready for drawing */
    if (display_on)
        (*env_ptr)->CallVoidMethod(env_ptr, Framebuffer_instance, java_lcd_update);
}

void lcd_update_rect(int x, int y, int height, int width)
{
    if (display_on)
    {
        (*env_ptr)->CallVoidMethod(env_ptr, Framebuffer_instance, java_lcd_update_rect,
                                   x, y, height, width);
    }
}

bool lcd_active(void)
{
    return display_on;
}

/*
 * (un)block lcd updates.
 *
 * Notice: This is called from the activity thread, so take it
 * as interrupt context and take care what the event callback does
 * (it shouldn't block in particular
 *
 * the 1s are needed due to strange naming conventions...
 **/
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxFramebuffer_set_1lcd_1active(JNIEnv *e,
                                                     jobject this,
                                                     jint active)
{
    (void)e;
    (void)this;
    display_on = active != 0;
    if (active)
        send_event(LCD_EVENT_ACTIVATION, NULL);
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

/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Plasma demo plugin
*
* My crack at making a 80's style retro plasma effect for the fantastic
* rockbox! 
* Okay, I could've hard-coded the sine wave values, I just wanted the 
* challange of calculating them! silly: maybe, fun: yes!
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

#include "plugin.h"
#include "fixedpoint.h"
#include "lib/helper.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"

#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif


/******************************* Globals ***********************************/
static fb_data *lcd_fb;


static unsigned char wave_array[256];  /* Pre calculated wave array */
#ifdef HAVE_LCD_COLOR
static fb_data colours[256]; /* Smooth transition of shades */
static int redfactor = 1, greenfactor = 2, bluefactor = 3;
static int redphase = 0, greenphase = 50, bluephase = 100;
           /* lower chance of gray at regular intervals */
#else
GREY_INFO_STRUCT
static unsigned char colours[256]; /* Smooth transition of shades */
static unsigned char greybuffer[LCD_HEIGHT*LCD_WIDTH]; /* off screen buffer */
static unsigned char *gbuf;
static size_t        gbuf_size = 0;
#endif
static unsigned char sp1, sp2, sp3, sp4; /* Speed of plasma */
static int plasma_frequency;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
static bool boosted = false;
#endif

static const struct button_mapping* plugin_contexts[]= {
    pla_main_ctx,
#if defined(HAVE_REMOTE_LCD)
    pla_remote_ctx,
#endif
};

#define WAV_AMP 90

/*
 * Main wave function so we don't have to re-calc the sine 
 * curve every time. Mess around WAV_AMP and FREQ to make slighlty
 * weirder looking plasmas!
 */

static void wave_table_generate(void)
{
    int i;
    for (i=0;i<256;++i)
    {
        wave_array[i] = (unsigned char)((WAV_AMP
                      * (fp14_sin((i * 360 * plasma_frequency) / 256))) / 16384);
    }
}

#ifdef HAVE_LCD_COLOR
/* Make a smooth colour cycle. */
static void shades_generate(int time)
{
    int i;
    unsigned red, green, blue;
    unsigned r = time * redfactor + redphase;
    unsigned g = time * greenfactor + greenphase;
    unsigned b = time * bluefactor + bluephase;

    for(i=0; i < 256; ++i)
    {
        r &= 0xFF; g &= 0xFF; b &= 0xFF;

        red = 2 * r;
        if (red > 255)
            red = 510 - red;
        green = 2 * g;
        if (green > 255)
            green = 510 - green;
        blue = 2 * b;
        if (blue > 255)
            blue= 510 - blue;

        colours[i] = FB_RGBPACK(red, green, blue);

        r++; g++; b++;
    }
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_pal256_update_pal(colours);
#endif
}
#else
/* Make a smooth shade from black into white and back into black again. */
static void shades_generate(void)
{
    int i, y;

    for(i=0; i < 256; ++i)
    {
        y = 2 * i;
        if (y > 255)
            y = 510 - y;
        colours[i] = y;
    }
}
#endif

static void cleanup(void)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if (boosted)
        rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
#ifdef HAVE_BACKLIGHT
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
#endif
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif
}

/*
 * Main function that also contain the main plasma
 * algorithm.
 */

int main(void)
{
    plasma_frequency = 1;
    int action, x, y;
    unsigned char p1,p2,p3,p4,t1,t2,t3,t4, z,z0;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    long last_tick = *rb->current_tick;
    int delay;
    int cumulated_lag = 0;
#endif
#ifdef HAVE_LCD_COLOR
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    unsigned char *ptr;
#else
    fb_data *ptr;
#endif
    int time=0;
#else
    unsigned char *ptr;
#endif

    /*Generate the neccesary pre calced stuff*/
    wave_table_generate();

#ifndef HAVE_LCD_COLOR
    shades_generate();  /* statically */

    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    if (!grey_init(gbuf, gbuf_size, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return PLUGIN_ERROR;
    }
    /* switch on greyscale overlay */
    grey_show(true);
#endif
    atexit(cleanup);
    sp1 = 4;
    sp2 = 2;
    sp3 = 4;
    sp4 = 2;
    p1=p2=p3=p4=0;
    while (true)
    {
#ifdef HAVE_LCD_COLOR
        shades_generate(time++); /* dynamically */
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
        ptr = (unsigned char*)lcd_fb;
#else
        ptr = lcd_fb;
#endif

#else
        ptr = greybuffer;
#endif
        t1=p1;
        t2=p2;
        for(y = 0; y < LCD_HEIGHT; ++y)
        {
            t3=p3;
            t4=p4;
            z0 = wave_array[t1] + wave_array[t2];
            for(x = 0; x < LCD_WIDTH; ++x)
            {
                z = z0 + wave_array[t3] + wave_array[t4];
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
                *ptr++ = z;
#else
                *ptr++ = colours[z];
#endif
                t3+=1;
                t4+=2;
            }
            t1+=2;
            t2+=1;
            rb->yield();
        }
        p1+=sp1;
        p2-=sp2;
        p3+=sp3;
        p4-=sp4;
#ifdef HAVE_LCD_COLOR
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
        rb->lcd_blit_pal256(    (unsigned char*)lcd_fb,
                                0,0,0,0,LCD_WIDTH,LCD_HEIGHT);
#else
        rb->lcd_update();
#endif
#else
        grey_ub_gray_bitmap(greybuffer, 0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        delay = last_tick - *rb->current_tick + HZ/33;
        if (!boosted && delay < 0)
        {
            cumulated_lag -= delay;     /* proportional increase */
            if (cumulated_lag >= HZ)
                rb->cpu_boost(boosted = true);
        }
        else if (boosted && delay > 1)  /* account for jitter */
        {
            if (--cumulated_lag <= 0)   /* slow decrease */
                rb->cpu_boost(boosted = false);
        }
        last_tick = *rb->current_tick;
#endif
        action = pluginlib_getaction(0, plugin_contexts,
                        ARRAYLEN(plugin_contexts));

        switch(action)
        {
            case PLA_EXIT:
            case PLA_CANCEL:
                return PLUGIN_OK;
                break;

#ifdef HAVE_SCROLLWHEEL
            case PLA_SCROLL_FWD:
            case PLA_SCROLL_FWD_REPEAT:
#endif
            case PLA_UP:
            case PLA_UP_REPEAT:
                ++plasma_frequency;
                wave_table_generate();
                break;

#ifdef HAVE_SCROLLWHEEL
            case PLA_SCROLL_BACK:
            case PLA_SCROLL_BACK_REPEAT:
#endif
            case PLA_DOWN:
            case PLA_DOWN_REPEAT:
                if(plasma_frequency>1)
                {
                    --plasma_frequency;
                    wave_table_generate();
                }
                break;
#ifdef HAVE_LCD_COLOR
            case PLA_SELECT:
                redfactor=rb->rand()%4;
                greenfactor=rb->rand()%4;
                bluefactor=rb->rand()%4;
                redphase=rb->rand()%256;
                greenphase=rb->rand()%256;
                bluephase=rb->rand()%256;
                break;
#endif

            default:
                exit_on_usb(action);
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
#ifdef HAVE_BACKLIGHT
    /* Turn off backlight timeout */
    backlight_ignore_timeout();
#endif
#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif
    struct viewport *vp_main = rb->lcd_set_viewport(NULL);
    lcd_fb = vp_main->buffer->fb_ptr;

    return main();
}

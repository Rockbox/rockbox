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
#include "lib/helper.h"

#ifdef HAVE_LCD_BITMAP

#ifndef HAVE_LCD_COLOR
#include "lib/grey.h"
#endif
#include "lib/fixedpoint.h"

PLUGIN_HEADER

/******************************* Globals ***********************************/

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

/* Key assignement, all bitmapped models */
#if (CONFIG_KEYPAD == RECORDER_PAD) || (CONFIG_KEYPAD == ONDIO_PAD)
#define PLASMA_QUIT               BUTTON_OFF
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define PLASMA_QUIT               BUTTON_OFF
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT
#define PLASMA_RC_QUIT            BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define PLASMA_QUIT               BUTTON_MENU
#define PLASMA_INCREASE_FREQUENCY BUTTON_SCROLL_FWD
#define PLASMA_DECREASE_FREQUENCY BUTTON_SCROLL_BACK
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
      (CONFIG_KEYPAD == SANSA_C200_PAD) || \
      (CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
      (CONFIG_KEYPAD == SANSA_M200_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define PLASMA_QUIT               (BUTTON_HOME|BUTTON_REPEAT)
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_PLAY

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_SCROLL_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_SCROLL_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_PLAY

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)
#define PLASMA_QUIT               BUTTON_BACK
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#define PLASMA_QUIT               BUTTON_RC_REC
#define PLASMA_INCREASE_FREQUENCY BUTTON_RC_VOL_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_RC_VOL_DOWN
#define PLASMA_RC_QUIT            BUTTON_REC

#elif (CONFIG_KEYPAD == COWON_D2_PAD)
#define PLASMA_QUIT               BUTTON_POWER

#elif (CONFIG_KEYPAD == IAUDIO67_PAD)
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_RIGHT
#define PLASMA_DECREASE_FREQUENCY BUTTON_LEFT
#define PLASMA_REGEN_COLORS       BUTTON_PLAY
#define PLASMA_RC_QUIT            BUTTON_STOP

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define PLASMA_QUIT               BUTTON_BACK
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_SELECT

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define PLASMA_QUIT               BUTTON_POWER
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_PLAY

#elif (CONFIG_KEYPAD == ONDAVX747_PAD) || (CONFIG_KEYPAD == ONDAVX777_PAD)
#define PLASMA_QUIT BUTTON_POWER

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define PLASMA_QUIT               BUTTON_PLAY
#define PLASMA_INCREASE_FREQUENCY BUTTON_UP
#define PLASMA_DECREASE_FREQUENCY BUTTON_DOWN
#define PLASMA_REGEN_COLORS       BUTTON_LEFT

#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef PLASMA_QUIT
#define PLASMA_QUIT               BUTTON_TOPLEFT
#endif
#ifndef PLASMA_INCREASE_FREQUENCY
#define PLASMA_INCREASE_FREQUENCY BUTTON_MIDRIGHT
#endif
#ifndef PLASMA_DECREASE_FREQUENCY
#define PLASMA_DECREASE_FREQUENCY BUTTON_MIDLEFT
#endif
#ifdef HAVE_LCD_COLOR
#ifndef PLASMA_REGEN_COLORS
#define PLASMA_REGEN_COLORS       BUTTON_CENTER
#endif
#endif /* HAVE_LCD_COLOR */
#endif /* HAVE_TOUCHSCREEN */

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
void shades_generate(int time)
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

        colours[i] = LCD_RGBPACK(red, green, blue);

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

void cleanup(void *parameter)
{
    (void)parameter;

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    if (boosted)
        rb->cpu_boost(false);
#endif
#ifndef HAVE_LCD_COLOR
    grey_release();
#endif
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(); /* backlight control in lib/helper.c */
}

/*
 * Main function that also contain the main plasma
 * algorithm.
 */

int main(void)
{
    plasma_frequency = 1;
    int button, delay, x, y;
    unsigned char p1,p2,p3,p4,t1,t2,t3,t4, z,z0;
    long last_tick = *rb->current_tick;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
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

    grey_init(gbuf, gbuf_size, GREY_ON_COP, LCD_WIDTH, LCD_HEIGHT, NULL);
    /* switch on greyscale overlay */
    grey_show(true);
#endif
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
        ptr = (unsigned char*)rb->lcd_framebuffer;
#else
        ptr = rb->lcd_framebuffer;
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
        rb->lcd_blit_pal256(    (unsigned char*)rb->lcd_framebuffer,
                                0,0,0,0,LCD_WIDTH,LCD_HEIGHT);
#else
        rb->lcd_update();
#endif
#else
        grey_ub_gray_bitmap(greybuffer, 0, 0, LCD_WIDTH, LCD_HEIGHT);
#endif

        delay = last_tick - *rb->current_tick + HZ/33;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
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
#endif
        button = rb->button_get_w_tmo(MAX(0, delay));
        last_tick = *rb->current_tick;

        switch(button)
        {
#ifdef PLASMA_RC_QUIT
            case PLASMA_RC_QUIT:
#endif
            case(PLASMA_QUIT):
                cleanup(NULL);
                return PLUGIN_OK;
                break;

            case (PLASMA_INCREASE_FREQUENCY):
                ++plasma_frequency;
                wave_table_generate();
                break;

            case (PLASMA_DECREASE_FREQUENCY):
                if(plasma_frequency>1)
                {
                    --plasma_frequency;
                    wave_table_generate();
                }
                break;
#ifdef HAVE_LCD_COLOR
            case (PLASMA_REGEN_COLORS):
                redfactor=rb->rand()%4;
                greenfactor=rb->rand()%4;
                bluefactor=rb->rand()%4;
                redphase=rb->rand()%256;
                greenphase=rb->rand()%256;
                bluephase=rb->rand()%256;
                break;
#endif

            default:
                if (rb->default_event_handler_ex(button, cleanup, NULL)
                    == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

/*************************** Plugin entry point ****************************/

enum plugin_status plugin_start(const void* parameter)
{
    int ret;

    (void)parameter;
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    /* Turn off backlight timeout */
    backlight_force_on(); /* backlight control in lib/helper.c */

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_PAL256);
#endif

    ret = main();

#if defined(HAVE_LCD_MODES) && (HAVE_LCD_MODES & LCD_MODE_PAL256)
    rb->lcd_set_mode(LCD_MODE_RGB565);
#endif

    return ret;
}

#endif /* HAVE_LCD_BITMAP */

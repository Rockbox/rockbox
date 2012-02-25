/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Itai Shaked
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/
#include "plugin.h"
#include "lib/playergfx.h"
#include "lib/mylcd.h"
#include "lib/pluginlib_actions.h"

#ifdef HAVE_LCD_BITMAP
#define NUM_PARTICLES (LCD_WIDTH * LCD_HEIGHT / 72)
#define SNOW_HEIGHT LCD_HEIGHT
#define SNOW_WIDTH LCD_WIDTH
#else
#define NUM_PARTICLES 10
#define SNOW_HEIGHT 14
#define SNOW_WIDTH 20
#endif

static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

/* PLA definitions */
#define SNOW_QUIT    PLA_EXIT
#define SNOW_QUIT2   PLA_CANCEL

static short particles[NUM_PARTICLES][2];

#ifdef HAVE_LCD_BITMAP
#if LCD_WIDTH >= 160
#define FLAKE_WIDTH 5
static const unsigned char flake[] = {0x0a,0x04,0x1f,0x04,0x0a};
#else
#define FLAKE_WIDTH 3
static const unsigned char flake[] = {0x02,0x07,0x02};
#endif
#endif

static bool particle_exists(int particle)
{
    if (particles[particle][0]>=0 && particles[particle][1]>=0 &&
        particles[particle][0]<SNOW_WIDTH && particles[particle][1]<SNOW_HEIGHT)
        return true;
    else
        return false;
}

static int create_particle(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        if (!particle_exists(i)) {
            particles[i][0]=(rb->rand()%SNOW_WIDTH);
            particles[i][1]=0;
            return i;
        }
    }
    return -1;
}

static void snow_move(void)
{
    int i;

    if (!(rb->rand()%2))
        create_particle();

    for (i=0; i<NUM_PARTICLES; i++) {
        if (particle_exists(i)) {
            mylcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
#ifdef HAVE_LCD_BITMAP
            rb->lcd_fillrect(particles[i][0],particles[i][1],
                             FLAKE_WIDTH,FLAKE_WIDTH);
#else
            pgfx_drawpixel(particles[i][0],particles[i][1]);
#endif
            mylcd_set_drawmode(DRMODE_SOLID);
#ifdef HAVE_REMOTE_LCD
            if (particles[i][0] <= LCD_REMOTE_WIDTH 
                    && particles[i][1] <= LCD_REMOTE_HEIGHT) {
                rb->lcd_remote_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_remote_fillrect(particles[i][0],particles[i][1],
                                        FLAKE_WIDTH,FLAKE_WIDTH);
                rb->lcd_remote_set_drawmode(DRMODE_SOLID);
            }
#endif
            switch ((rb->rand()%7)) {
                case 0:
                    particles[i][0]++;
                    break;

                case 1:
                    particles[i][0]--;
                    break;

                case 2:
                    break;

                default:
                    particles[i][1]++;
                    break;
            }
            if (particle_exists(i))
#ifdef HAVE_LCD_BITMAP
                rb->lcd_mono_bitmap(flake,particles[i][0],particles[i][1],
                                    FLAKE_WIDTH,FLAKE_WIDTH);
#else
                pgfx_drawpixel(particles[i][0],particles[i][1]);
#endif
#ifdef HAVE_REMOTE_LCD
            if (particles[i][0] <= LCD_REMOTE_WIDTH 
                    && particles[i][1] <= LCD_REMOTE_HEIGHT) {
                rb->lcd_remote_mono_bitmap(flake,particles[i][0],particles[i][1],
                                           FLAKE_WIDTH,FLAKE_WIDTH);
            }
#endif
        }
    }
}


static void snow_init(void)
{
    int i;

    for (i=0; i<NUM_PARTICLES; i++) {
        particles[i][0]=-1;
        particles[i][1]=-1;
    }        
#ifdef HAVE_LCD_CHARCELLS
    pgfx_display(0, 0); /* display three times */
    pgfx_display(4, 0);
    pgfx_display(8, 0);
#endif
    mylcd_clear_display();
#ifdef HAVE_REMOTE_LCD
    rb->lcd_remote_clear_display();
#endif
}

enum plugin_status plugin_start(const void* parameter)
{
    int button;
    (void)(parameter);

#ifdef HAVE_LCD_CHARCELLS
    if (!pgfx_init(4, 2))
    {
        rb->splash(HZ*2, "Old LCD :(");
        return PLUGIN_OK;
    }
#endif
#ifdef HAVE_LCD_COLOR
    rb->lcd_clear_display();
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_DEFAULT_BG);
#endif
    snow_init();
    while (1) {
        snow_move();
        mylcd_update();
#ifdef HAVE_REMOTE_LCD
        rb->lcd_remote_update();
#endif
        rb->sleep(HZ/20);
        
                /*We get button from PLA this way */
        button = pluginlib_getaction(TIMEOUT_NOBLOCK, plugin_contexts,
                               ARRAYLEN(plugin_contexts));

        if ((button == SNOW_QUIT) || (button == SNOW_QUIT2))
        {
#ifdef HAVE_LCD_CHARCELLS
            pgfx_release();
#endif
            return PLUGIN_OK;
        }
        else
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
#ifdef HAVE_LCD_CHARCELLS
                pgfx_release();
#endif
                return PLUGIN_USB_CONNECTED;
            }
    }
}

